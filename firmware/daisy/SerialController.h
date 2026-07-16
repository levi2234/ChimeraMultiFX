#pragma once
#include "daisy_seed.h"
#include "util/CpuLoadMeter.h"
#include "Router.h"
#include "include/core/SerialTransport.h"
#include "include/core/EffectRegistry.h"
#include "include/core/SerialResponseBuilder.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cmath>

// =============================================================================
// SerialController — Human-readable UART command interface
// =============================================================================
//
// Commands:
//   add <lane> <effect>                      → append effect to lane
//   insert <lane> <slot> <effect>            → insert at position
//   remove <lane> <slot>                     → delete effect
//   swap <lane> <slotA> <slotB>              → swap two effects
//   move <from_lane> <from_slot> <to_lane> <to_slot>
//   set <lane> <slot> <param> <value>        → set effect parameter
//   get <lane> <slot> <param>                → read effect parameter
//   bypass <lane> <slot> <0|1>               → enable/disable effect
//   clear <lane>                             → remove all effects
//   route <lane> <input> <output>            → set lane I/O
//   level <lane> <value>                     → set lane level
//   params <lane> <slot>                     → list effect's param names
//   status                                   → full router state as JSON
//   info                                     → system capabilities as JSON
//   effect <effect>                          → effect parameter metadata as JSON
//   ping                                     → protocol/link test
//   dfu                                      → reboot into STM32 DFU bootloader
//   reset                                    → software-reset the Daisy Seed
//   setpin <pin> <0|1>                       → set GPIO pin high/low
//
// Effect names: distortion, bitcrusher, overdrive, chorus, tremolo, delay, compressor, lowpass
// Input names:  in1, in2, mix, lane0, lane1, lane2, lane3
// Output names: out1, out2, both, none
//
// =============================================================================

class SerialController {
public:
    static constexpr size_t UART_RX_DMA_BUFFER_LEN = 256;
    static constexpr size_t USB_RX_QUEUE_LEN = 512;

    void Init(Router* router,
              float sample_rate,
              daisy::UsbHandle* usb = nullptr,
              daisy::UartHandler* uart = nullptr) {
        router_ = router;
        sample_rate_ = sample_rate;
        usb_ = usb;
        uart_ = uart;
        transport_.Init(usb, uart);
        buf_pos_ = 0;
    }

    // Feed one character at a time (call from main loop)
    void Feed(char c) {
        if (c == '\n' || c == '\r') {
            if (discarding_frame_) {
                discarding_frame_ = false;
                buf_pos_ = 0;
            } else if (buf_pos_ > 0) {
                buf_[buf_pos_] = '\0';
                Execute(buf_);
                buf_pos_ = 0;
            }
        } else if (discarding_frame_) {
            return;
        } else if (buf_pos_ < MAX_CMD_LEN - 1) {
            buf_[buf_pos_++] = c;
        } else {
            framing_error_count_++;
            discarding_frame_ = true;
            buf_pos_ = 0;
        }
    }

    void ProcessPendingActions() {
        if (dfu_requested_) {
            dfu_requested_ = false;
            daisy::System::Delay(250);
            daisy::System::ResetToBootloader(
                daisy::System::BootloaderMode::DAISY_INFINITE_TIMEOUT);
        }
    }

    void ProcessPendingUart() {
        const bool receive_restarted = transport_.MaintainUartReceive();
        const bool receive_overflowed = transport_.TakeRxOverflow();
        if (receive_restarted || receive_overflowed) {
            buf_pos_ = 0;
            discarding_frame_ = false;
        }
        if (receive_overflowed) framing_error_count_++;
        transport_.ProcessPending(HandleTransportByte, this);
    }

    void QueueUartBytes(const uint8_t* data, size_t size) {
        transport_.QueueUartBytes(data, size);
    }

    void QueueUsbBytes(const uint8_t* data, size_t size) {
        for (size_t i = 0; i < size; i++) {
            const uint16_t next = static_cast<uint16_t>((usb_rx_head_ + 1) % USB_RX_QUEUE_LEN);
            if (next == usb_rx_tail_) {
                usb_rx_overflowed_ = true;
                continue;
            }
            usb_rx_queue_[usb_rx_head_] = data[i];
            usb_rx_head_ = next;
        }
    }

    void ProcessPendingUsb() {
        if (usb_rx_overflowed_) {
            usb_rx_tail_ = usb_rx_head_;
            usb_rx_overflowed_ = false;
            framing_error_count_++;
            buf_pos_ = 0;
            discarding_frame_ = false;
        }
        while (usb_rx_tail_ != usb_rx_head_) {
            const char byte = static_cast<char>(usb_rx_queue_[usb_rx_tail_]);
            usb_rx_tail_ = static_cast<uint16_t>((usb_rx_tail_ + 1) % USB_RX_QUEUE_LEN);
            Feed(byte);
        }
    }

    void StartUartDmaReceive(uint8_t* buffer,
                             size_t size,
                             daisy::UartHandler::CircularRxCallbackFunctionPtr callback,
                             void* context) {
        transport_.StartUartDmaReceive(buffer, size, callback, context);
    }

    void BeginAudioCallback(size_t samples) {
        if (samples == 0 || sample_rate_ <= 0.0f) return;
        if (!cpu_load_meter_initialized_) {
            cpu_load_meter_.Init(sample_rate_, static_cast<int>(samples));
            cpu_load_meter_initialized_ = true;
        }
        cpu_load_meter_.OnBlockStart();
    }

    void RecordAudioCallback() {
        if (!cpu_load_meter_initialized_) return;

        cpu_load_meter_.OnBlockEnd();
        const float cpu_usage = cpu_load_meter_.GetAvgCpuLoad() * 100.0f;
        if (!std::isfinite(cpu_usage) || cpu_usage < 0.0f) {
            audio_cpu_usage_hundredths_ = 0;
            return;
        }

        const float rounded_hundredths = (cpu_usage * 100.0f) + 0.5f;
        if (rounded_hundredths >= 4294967295.0f) {
            audio_cpu_usage_hundredths_ = 0xffffffffu;
        } else {
            audio_cpu_usage_hundredths_ = static_cast<uint32_t>(rounded_hundredths);
        }
    }

private:
    static constexpr int MAX_CMD_LEN = 128;
    static constexpr int MAX_TOKENS  = 8;
    static constexpr int TX_BUF_LEN  = 256;
    static constexpr int JSON_BUF_LEN = 32768;
    static constexpr uint32_t CPU_GUARD_PREFLIGHT_HUNDREDTHS = 6000;
    static constexpr uint32_t CPU_GUARD_POSTFLIGHT_HUNDREDTHS = 6000;
    static constexpr uint32_t CPU_GUARD_SETTLE_MS = 100;
    Router* router_ = nullptr;
    float   sample_rate_ = 48000.f;
    daisy::UsbHandle* usb_ = nullptr;
    daisy::UartHandler* uart_ = nullptr;
    char    buf_[MAX_CMD_LEN] = {};
    char    tx_buf_[TX_BUF_LEN] = {};
    char    json_buf_[JSON_BUF_LEN] = {};
    int     buf_pos_ = 0;
    bool    discarding_frame_ = false;
    uint32_t framing_error_count_ = 0;
    volatile bool dfu_requested_ = false;
    volatile uint32_t audio_cpu_usage_hundredths_ = 0;
    uint8_t usb_rx_queue_[USB_RX_QUEUE_LEN] = {};
    volatile uint16_t usb_rx_head_ = 0;
    volatile uint16_t usb_rx_tail_ = 0;
    volatile bool usb_rx_overflowed_ = false;
    daisy::CpuLoadMeter cpu_load_meter_;
    bool cpu_load_meter_initialized_ = false;
    SerialTransport transport_;

    // ─── Command Dispatch ────────────────────────────────────────────────────
    void Execute(char* cmd) {
        char* tokens[MAX_TOKENS] = {};
        int n = Tokenize(cmd, tokens, MAX_TOKENS);
        if (n == 0) return;

        if      (strcmp(tokens[0], "add") == 0)    CmdAdd(tokens, n);
        else if (strcmp(tokens[0], "insert") == 0) CmdInsert(tokens, n);
        else if (strcmp(tokens[0], "remove") == 0) CmdRemove(tokens, n);
        else if (strcmp(tokens[0], "swap") == 0)   CmdSwap(tokens, n);
        else if (strcmp(tokens[0], "move") == 0)   CmdMove(tokens, n);
        else if (strcmp(tokens[0], "set") == 0)    CmdSet(tokens, n);
        else if (strcmp(tokens[0], "get") == 0)    CmdGet(tokens, n);        else if (strcmp(tokens[0], "bypass") == 0) CmdBypass(tokens, n);
        else if (strcmp(tokens[0], "clear") == 0)  CmdClear(tokens, n);
        else if (strcmp(tokens[0], "route") == 0)  CmdRoute(tokens, n);
        else if (strcmp(tokens[0], "level") == 0)  CmdLevel(tokens, n);        else if (strcmp(tokens[0], "params") == 0) CmdParams(tokens, n);
        else if (strcmp(tokens[0], "status") == 0) CmdStatus(tokens, n);
        else if (strcmp(tokens[0], "info") == 0)   CmdInfo();
        else if (strcmp(tokens[0], "effect") == 0) CmdEffectInfo(tokens, n);
        else if (strcmp(tokens[0], "ping") == 0)   CmdPing();
        else if (strcmp(tokens[0], "uartdiag") == 0) CmdUartDiag();
        else if (strcmp(tokens[0], "dfu") == 0)    CmdDfu();
        else if (strcmp(tokens[0], "setpin") == 0)  CmdSetPin(tokens, n);
        else if (strcmp(tokens[0], "getpin") == 0)  CmdGetPin(tokens, n);
        else if (strcmp(tokens[0], "cpu_usage") == 0)  CmdCpuUsage();
        else if (strcmp(tokens[0], "reset") == 0)  CmdReset();
        else Reply("ERR unknown command\n");
    }

    static void HandleTransportByte(char byte, void* context) {
        if (context) {
            static_cast<SerialController*>(context)->Feed(byte);
        }
    }

    // ─── add <lane> <effect> ─────────────────────────────────────────────────
    void CmdAdd(char** t, int n) {
        if (n < 3) { Reply("ERR usage: add <lane> <effect>\n"); return; }
        int lane = atoi(t[1]);
        if (!ValidLane(lane)) return;

        Effect* fx = CreateFromName(t[2]);
        if (!fx) { Reply("ERR unknown effect: %s\n", t[2]); return; }

        const bool enable_effect = CpuBudgetAvailable();
        fx->SetEnabled(enable_effect);
        router_->lanes[lane].Add(fx);
        const int slot = router_->lanes[lane].count - 1;
        if (!enable_effect || !KeepEffectWithinCpuBudget(lane, slot)) {
            Reply("OK added %s to lane %d slot %d bypassed cpu_limit\n", t[2], lane, slot);
            return;
        }
        Reply("OK added %s to lane %d slot %d\n", t[2], lane, slot);
    }

    void CmdReset() {
        daisy::System::Delay(10);
        HAL_NVIC_SystemReset();
    }

    // ─── insert <lane> <slot> <effect> ───────────────────────────────────────
    void CmdInsert(char** t, int n) {
        if (n < 4) { Reply("ERR usage: insert <lane> <slot> <effect>\n"); return; }
        int lane = atoi(t[1]);
        int slot = atoi(t[2]);
        if (!ValidLane(lane)) return;

        Effect* fx = CreateFromName(t[3]);
        if (!fx) { Reply("ERR unknown effect: %s\n", t[3]); return; }

        const bool enable_effect = CpuBudgetAvailable();
        fx->SetEnabled(enable_effect);
        router_->lanes[lane].Insert(slot, fx);
        if (!enable_effect || !KeepEffectWithinCpuBudget(lane, slot)) {
            Reply("OK inserted %s at lane %d slot %d bypassed cpu_limit\n", t[3], lane, slot);
            return;
        }
        Reply("OK inserted %s at lane %d slot %d\n", t[3], lane, slot);
    }

    // ─── remove <lane> <slot> ────────────────────────────────────────────────
    void CmdRemove(char** t, int n) {
        if (n < 3) { Reply("ERR usage: remove <lane> <slot>\n"); return; }
        int lane = atoi(t[1]);
        int slot = atoi(t[2]);
        if (!ValidLane(lane)) return;
        if (!ValidSlot(lane, slot)) return;

        delete router_->lanes[lane].slots[slot];
        router_->lanes[lane].Remove(slot);
        Reply("OK removed lane %d slot %d\n", lane, slot);
    }

    // ─── swap <lane> <slotA> <slotB> ────────────────────────────────────────
    void CmdSwap(char** t, int n) {
        if (n < 4) { Reply("ERR usage: swap <lane> <slotA> <slotB>\n"); return; }
        int lane = atoi(t[1]);
        int a = atoi(t[2]);
        int b = atoi(t[3]);
        if (!ValidLane(lane)) return;

        router_->lanes[lane].Swap(a, b);
        Reply("OK swapped lane %d slots %d <-> %d\n", lane, a, b);
    }

    // ─── move <from_lane> <from_slot> <to_lane> <to_slot> ───────────────────
    void CmdMove(char** t, int n) {
        if (n < 5) { Reply("ERR usage: move <from_lane> <from_slot> <to_lane> <to_slot>\n"); return; }
        int fl = atoi(t[1]), fs = atoi(t[2]);
        int tl = atoi(t[3]), ts = atoi(t[4]);
        if (!ValidLane(fl) || !ValidLane(tl)) return;
        if (!ValidSlot(fl, fs)) return;

        router_->MoveEffect(fl, fs, tl, ts);
        Reply("OK moved %d:%d -> %d:%d\n", fl, fs, tl, ts);
    }    // ─── set <lane> <slot> <param> <value> ───────────────────────────────────
    void CmdSet(char** t, int n) {
        if (n < 5) { Reply("ERR usage: set <lane> <slot> <param> <value>\n"); return; }
        int lane = atoi(t[1]);
        int slot = atoi(t[2]);
        if (!ValidLane(lane) || !ValidSlot(lane, slot)) return;

        Effect* fx = router_->lanes[lane].slots[slot];

        // Check if value is a boolean string
        if (IsBoolStr(t[4])) {
            bool val = ParseBool(t[4]);
            fx->SetBoolParam(t[3], val);
            Reply("OK %s = %s\n", t[3], val ? "true" : "false");
        } else {
            float val = static_cast<float>(atof(t[4]));
            fx->SetParam(t[3], val);
            ReplyFloat("OK %s = ", t[3], val, 4, "\n");
        }
    }

    // ─── get <lane> <slot> <param> ───────────────────────────────────────────
    void CmdGet(char** t, int n) {
        if (n < 4) { Reply("ERR usage: get <lane> <slot> <param>\n"); return; }
        int lane = atoi(t[1]);
        int slot = atoi(t[2]);
        if (!ValidLane(lane) || !ValidSlot(lane, slot)) return;
        float val = router_->lanes[lane].slots[slot]->GetParam(t[3]);
        ReplyFloat("VAL %s = ", t[3], val, 4, "\n");
    }

    // ─── params <lane> <slot> ────────────────────────────────────────────────
    void CmdParams(char** t, int n) {
        if (n < 3) { Reply("ERR usage: params <lane> <slot>\n"); return; }
        int lane = atoi(t[1]);
        int slot = atoi(t[2]);
        if (!ValidLane(lane) || !ValidSlot(lane, slot)) return;

        Effect* fx = router_->lanes[lane].slots[slot];
        Reply("PARAMS [%s] %s\n", fx->GetName(), fx->GetParamList());
    }

    // ─── bypass <lane> <slot> <0|1> ─────────────────────────────────────────
    void CmdBypass(char** t, int n) {
        if (n < 4) { Reply("ERR usage: bypass <lane> <slot> <0|1>\n"); return; }
        int lane = atoi(t[1]);
        int slot = atoi(t[2]);
        if (!ValidLane(lane) || !ValidSlot(lane, slot)) return;

        bool enabled = (atoi(t[3]) != 0);
        if (enabled && !CpuBudgetAvailable()) {
            router_->lanes[lane].SetEffectEnabled(slot, false);
            Reply("CPU Limit Reached; lane %d slot %d kept bypassed\n", lane, slot);
            return;
        }
        router_->lanes[lane].SetEffectEnabled(slot, enabled);
        if (enabled && !KeepEffectWithinCpuBudget(lane, slot)) {
            Reply("CPU Limit Reached; lane %d slot %d kept bypassed\n", lane, slot);
            return;
        }
        Reply("OK bypass lane %d slot %d = %s\n", lane, slot, enabled ? "on" : "off");
    }

    bool CpuBudgetAvailable() const {
        return audio_cpu_usage_hundredths_ < CPU_GUARD_PREFLIGHT_HUNDREDTHS;
    }

    bool KeepEffectWithinCpuBudget(int lane, int slot) {
        daisy::System::Delay(CPU_GUARD_SETTLE_MS);
        if (audio_cpu_usage_hundredths_ < CPU_GUARD_POSTFLIGHT_HUNDREDTHS) return true;
        router_->lanes[lane].SetEffectEnabled(slot, false);
        return false;
    }

    // ─── clear <lane> ────────────────────────────────────────────────────────
    void CmdClear(char** t, int n) {
        if (n < 2) { Reply("ERR usage: clear <lane>\n"); return; }
        int lane = atoi(t[1]);
        if (!ValidLane(lane)) return;

        for (int i = 0; i < router_->lanes[lane].count; i++) {
            delete router_->lanes[lane].slots[i];
        }
        router_->lanes[lane].Clear();
        Reply("OK cleared lane %d\n", lane);
    }

    // ─── route <lane> <input> <output> ───────────────────────────────────────
    void CmdRoute(char** t, int n) {
        if (n < 4) { Reply("ERR usage: route <lane> <input> <output>\n"); return; }
        int lane = atoi(t[1]);
        if (!ValidLane(lane)) return;

        router_->lanes[lane].input  = ParseInput(t[2]);
        router_->lanes[lane].output = ParseOutput(t[3]);
        Reply("OK route lane %d: %s -> %s\n", lane, t[2], t[3]);
    }

    // ─── level <lane> <value> ────────────────────────────────────────────────
    void CmdLevel(char** t, int n) {
        if (n < 3) { Reply("ERR usage: level <lane> <value>\n"); return; }
        int lane = atoi(t[1]);
        if (!ValidLane(lane)) return;

        router_->lanes[lane].level = static_cast<float>(atof(t[2]));
        ReplyLevel(lane, router_->lanes[lane].level);
    }

    // ─── status [lane <lane>|slot <lane> <slot>] ─────────────────────────────
    void CmdStatus(char** t, int n) {
        if (n == 1) {
            Reply("{\"lane_count\":%d,\"max_slots\":%d}\n", Router::MAX_LANES, Router::MAX_SLOTS);
            return;
        }
        if (strcmp(t[1], "lane") == 0) {
            if (n < 3) { Reply("ERR usage: status lane <lane>\n"); return; }
            CmdLaneStatus(atoi(t[2]));
            return;
        }
        if (strcmp(t[1], "slot") == 0) {
            if (n < 4) { Reply("ERR usage: status slot <lane> <slot>\n"); return; }
            CmdSlotStatus(atoi(t[2]), atoi(t[3]));
            return;
        }
        Reply("ERR usage: status [lane <lane>|slot <lane> <slot>]\n");
    }

    void CmdLaneStatus(int lane_index) {
        if (!ValidLane(lane_index)) return;
        int pos = 0;
        auto& lane = router_->lanes[lane_index];
        Append(json_buf_, JSON_BUF_LEN, pos,
               "{\"lane\":%d,\"active\":%s,\"input\":\"%s\",\"output\":\"%s\",\"level\":",
               lane_index,
               lane.active ? "true" : "false",
               InputName(lane.input),
               OutputName(lane.output));
        AppendFloat(json_buf_, JSON_BUF_LEN, pos, lane.level, 4);
        Append(json_buf_, JSON_BUF_LEN, pos, ",\"effects\":[");
        for (int slot = 0; slot < lane.count; slot++) {
            Effect* fx = lane.slots[slot];
            if (slot > 0) Append(json_buf_, JSON_BUF_LEN, pos, ",");
            Append(json_buf_, JSON_BUF_LEN, pos,
                   "{\"slot\":%d,\"name\":\"%s\",\"enabled\":%s}",
                   slot, fx->GetName(), fx->IsEnabled() ? "true" : "false");
        }
        Append(json_buf_, JSON_BUF_LEN, pos, "]}\n");
        SendBuffer(json_buf_, pos);
    }

    void CmdSlotStatus(int lane_index, int slot_index) {
        if (!ValidLane(lane_index) || !ValidSlot(lane_index, slot_index)) return;
        Effect* fx = router_->lanes[lane_index].slots[slot_index];
        int pos = 0;
        Append(json_buf_, JSON_BUF_LEN, pos,
               "{\"lane\":%d,\"slot\":%d,\"name\":\"%s\",\"enabled\":%s,\"params\":{",
               lane_index, slot_index, fx->GetName(), fx->IsEnabled() ? "true" : "false");
        EmitParams(json_buf_, JSON_BUF_LEN, pos, fx);
         Append(json_buf_, JSON_BUF_LEN, pos, "},\"param_info\":{");
         EmitParamInfo(json_buf_, JSON_BUF_LEN, pos, fx);
         Append(json_buf_, JSON_BUF_LEN, pos, "}}\n");
        SendBuffer(json_buf_, pos);
    }

    void EmitParams(char* out, int max, int& pos, Effect* effect) {
        SerialResponseBuilder::EmitParams(out, max, pos, effect);
    }

    void EmitParamInfo(char* out, int max, int& pos, Effect* effect) {
        SerialResponseBuilder::EmitParamInfo(out, max, pos, effect);
    }

    // ─── info ────────────────────────────────────────────────────────────────
    // Returns static system capabilities as JSON (call once at connection).
    void CmdInfo() {
         int pos = 0;
         Append(json_buf_, JSON_BUF_LEN, pos,
             "{\"sample_rate\":");
         AppendFloat(json_buf_, JSON_BUF_LEN, pos, sample_rate_, 0);
         Append(json_buf_, JSON_BUF_LEN, pos,
             ",\"max_lanes\":%d,\"max_slots\":%d,",
             Router::MAX_LANES, Router::MAX_SLOTS);
         Append(json_buf_, JSON_BUF_LEN, pos, "\"effects\":[");
         for (int index = 0; index < EffectRegistry::Count(); index++) {
             if (index > 0) Append(json_buf_, JSON_BUF_LEN, pos, ",");
             Append(json_buf_, JSON_BUF_LEN, pos,
                 "{\"name\":\"%s\",\"category\":\"%s\"}",
                 EffectRegistry::NameAt(index),
                 EffectRegistry::CategoryName(EffectRegistry::CategoryAt(index)));
         }
         Append(json_buf_, JSON_BUF_LEN, pos, "],");
         Append(json_buf_, JSON_BUF_LEN, pos,
             "\"inputs\":[\"in1\",\"in2\",\"mix\",\"lane0\",\"lane1\",\"lane2\",\"lane3\"],");
         Append(json_buf_, JSON_BUF_LEN, pos,
             "\"outputs\":[\"out1\",\"out2\",\"both\",\"none\"],");
         Append(json_buf_, JSON_BUF_LEN, pos,
             "\"commands\":[\"add\",\"insert\",\"remove\",\"swap\",\"move\",\"set\",\"get\",\"bypass\",\"clear\",\"route\",\"level\",\"params\",\"status\",\"status lane\",\"status slot\",\"info\",\"effect\",\"ping\",\"loopback\",\"cpu_usage\",\"dfu\"]}\n");
         SendBuffer(json_buf_, pos);
    }

    void CmdEffectInfo(char** t, int n) {
        if (n < 2) { Reply("ERR usage: effect <effect>\n"); return; }

        int pos = 0;
        Append(json_buf_, JSON_BUF_LEN, pos, "{");
        EmitRegisteredEffectInfo(json_buf_, JSON_BUF_LEN, pos, t[1]);
        Append(json_buf_, JSON_BUF_LEN, pos, "}\n");
        SendBuffer(json_buf_, pos);
    }

    void CmdPing() {
        Reply("PONG ChimeraMultiFX\n");
    }

    void CmdUartDiag() {
        if (!uart_) {
            Reply("UART unavailable\n");
            return;
        }
        Reply("UART listening=%d error=%d rx_bytes=%lu rx_restarts=%lu rx_restart_failures=%lu tx_failures=%lu framing_errors=%lu\n",
              uart_->IsListening() ? 1 : 0,
              uart_->CheckError(),
              static_cast<unsigned long>(transport_.RxByteCount()),
              static_cast<unsigned long>(transport_.RxRestartCount()),
              static_cast<unsigned long>(transport_.RxRestartFailureCount()),
              static_cast<unsigned long>(transport_.TxFailureCount()),
              static_cast<unsigned long>(framing_error_count_));
    }

    void CmdCpuUsage() {
        const uint32_t usage = audio_cpu_usage_hundredths_;
        Reply("CPU Usage: %u.%02u%%\n", usage / 100, usage % 100);
    }

    void CmdSetPin(char** t, int n) {
        if (n < 3) { Reply("ERR usage: setpin <pin> <0|1>\n"); return; }
        int pin = atoi(t[1]);
        if (pin < 0 || pin > 31) { Reply("ERR invalid pin %d\n", pin); return; }
        bool val = (atoi(t[2]) != 0);
        dsy_gpio gpio;
        gpio.pin = daisy::DaisySeed::GetPin(static_cast<uint8_t>(pin));
        gpio.mode = DSY_GPIO_MODE_OUTPUT_PP;
        gpio.pull = DSY_GPIO_NOPULL;
        dsy_gpio_init(&gpio);
        dsy_gpio_write(&gpio, val ? 1 : 0);
        Reply("OK set pin %d = %s\n", pin, val ? "high" : "low");
    }

    void CmdGetPin(char** t, int n) {
        if (n < 2) { Reply("ERR usage: getpin <pin>\n"); return; }
        int pin = atoi(t[1]);
        if (pin < 0 || pin > 31) { Reply("ERR invalid pin %d\n", pin); return; }
        dsy_gpio gpio;
        gpio.pin = daisy::DaisySeed::GetPin(static_cast<uint8_t>(pin));
        gpio.mode = DSY_GPIO_MODE_INPUT;
        gpio.pull = DSY_GPIO_PULLDOWN;
        dsy_gpio_init(&gpio);
        Reply("PIN %d = %d\n", pin, dsy_gpio_read(&gpio) ? 1 : 0);
    }

    void CmdDfu() {
        Reply("OK rebooting to Daisy bootloader DFU\n");
        dfu_requested_ = true;
    }

    void EmitRegisteredEffectInfo(char* out, int max, int& pos, const char* name) {
        Effect* fx = CreateFromName(name);
        if (!fx) {
            Append(out, max, pos, "\"%s\":{\"category\":\"unknown\",\"params\":{}}", name);
            return;
        }

        Append(out, max, pos, "\"%s\":{\"category\":\"%s\",\"params\":{", name, CategoryName(fx->GetCategory()));
        EmitParamInfo(out, max, pos, fx);
        Append(out, max, pos, "}}");
        delete fx;
    }

    // ─── Effect Factory ──────────────────────────────────────────────────────
    // This is the single place that maps a name string to a concrete type.
    Effect* CreateFromName(const char* name) {
        return EffectRegistry::Create(name, sample_rate_);
    }

    // ─── Parsers ─────────────────────────────────────────────────────────────
    InputSource ParseInput(const char* s) {
        if (strcmp(s, "in1") == 0)   return InputSource::In_1;
        if (strcmp(s, "in2") == 0)   return InputSource::In_2;
        if (strcmp(s, "mix") == 0)   return InputSource::In_Mix;
        if (strcmp(s, "lane0") == 0) return InputSource::Lane_0;
        if (strcmp(s, "lane1") == 0) return InputSource::Lane_1;
        if (strcmp(s, "lane2") == 0) return InputSource::Lane_2;
        if (strcmp(s, "lane3") == 0) return InputSource::Lane_3;
        return InputSource::In_1;
    }

    OutputDest ParseOutput(const char* s) {
        if (strcmp(s, "out1") == 0) return OutputDest::Out_1;
        if (strcmp(s, "out2") == 0) return OutputDest::Out_2;
        if (strcmp(s, "both") == 0) return OutputDest::Out_Both;
        if (strcmp(s, "none") == 0) return OutputDest::Out_None;
        return OutputDest::Out_Both;
    }

    const char* InputName(InputSource s) {
        switch (s) {
            case InputSource::In_1:   return "in1";
            case InputSource::In_2:   return "in2";
            case InputSource::In_Mix: return "mix";
            case InputSource::Lane_0: return "lane0";
            case InputSource::Lane_1: return "lane1";
            case InputSource::Lane_2: return "lane2";
            case InputSource::Lane_3: return "lane3";
            default: return "?";
        }
    }

    const char* OutputName(OutputDest d) {
        switch (d) {
            case OutputDest::Out_1:    return "out1";
            case OutputDest::Out_2:    return "out2";
            case OutputDest::Out_Both: return "both";
            case OutputDest::Out_None: return "none";
            default: return "?";
        }
    }

    const char* CategoryName(EffectCategory category) {
        return EffectRegistry::CategoryName(category);
    }

    // ─── Validation ──────────────────────────────────────────────────────────
    bool ValidLane(int lane) {
        if (lane < 0 || lane >= Router::MAX_LANES) {
            Reply("ERR invalid lane %d\n", lane);
            return false;
        }
        return true;
    }

    bool ValidSlot(int lane, int slot) {
        if (slot < 0 || slot >= router_->lanes[lane].count) {
            Reply("ERR invalid slot %d (lane %d has %d)\n", slot, lane, router_->lanes[lane].count);
            return false;
        }
        return true;
    }

    // ─── Utilities ───────────────────────────────────────────────────────────
    bool IsBoolStr(const char* s) {
        return strcmp(s, "true") == 0 || strcmp(s, "false") == 0
            || strcmp(s, "on") == 0   || strcmp(s, "off") == 0;
    }

    bool ParseBool(const char* s) {
        return strcmp(s, "true") == 0 || strcmp(s, "on") == 0;
    }

    int Tokenize(char* str, char** tokens, int max) {
        int count = 0;
        char* tok = strtok(str, " \t");
        while (tok && count < max) {
            tokens[count++] = tok;
            tok = strtok(nullptr, " \t");
        }
        return count;
    }

    void Reply(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        int len = vsnprintf(tx_buf_, sizeof(tx_buf_), fmt, args);
        va_end(args);
        if (len < 0) return;
        if (len >= static_cast<int>(sizeof(tx_buf_))) {
            len = static_cast<int>(sizeof(tx_buf_)) - 1;
        }
        SendBuffer(tx_buf_, len);
    }

    void ReplyUsbOnly(const char* message) {
        int len = static_cast<int>(strlen(message));
        if (usb_ && len > 0) {
            usb_->TransmitInternal(reinterpret_cast<uint8_t*>(const_cast<char*>(message)), static_cast<size_t>(len));
        }
    }

    void ReplyFloat(const char* prefix_fmt, const char* name, float value, int decimals, const char* suffix) {
        int pos = 0;
        Append(tx_buf_, TX_BUF_LEN, pos, prefix_fmt, name);
        AppendFloat(tx_buf_, TX_BUF_LEN, pos, value, decimals);
        Append(tx_buf_, TX_BUF_LEN, pos, "%s", suffix);
        SendBuffer(tx_buf_, pos);
    }

    void ReplyLevel(int lane, float value) {
        int pos = 0;
        Append(tx_buf_, TX_BUF_LEN, pos, "OK level lane %d = ", lane);
        AppendFloat(tx_buf_, TX_BUF_LEN, pos, value, 2);
        Append(tx_buf_, TX_BUF_LEN, pos, "\n");
        SendBuffer(tx_buf_, pos);
    }

    void Append(char* out, int max, int& pos, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        char formatted[256];
        vsnprintf(formatted, sizeof(formatted), fmt, args);
        va_end(args);
        SerialResponseBuilder::Append(out, max, pos, "%s", formatted);
    }

    void AppendFloat(char* out, int max, int& pos, float value, int decimals) {
        SerialResponseBuilder::AppendFloat(out, max, pos, value, decimals);
    }

    void SendBuffer(char* out, int len) {
        transport_.Send(out, static_cast<size_t>(len));
    }

    void DrainUart() {
        if (!uart_) return;

        uint8_t ignored = 0;
        while (uart_->BlockingReceive(&ignored, 1, 1) == daisy::UartHandler::Result::OK) {}
    }
};
