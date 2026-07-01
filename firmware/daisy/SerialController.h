#pragma once
#include "daisy_seed.h"
#include "Router.h"

// Effect includes — the name→type mapping lives here
#include "effects/distortion/TanhDistortion.h"
#include "effects/distortion/Bitcrusher.h"
#include "effects/modulation/Chorus.h"
#include "effects/modulation/Tremolo.h"
#include "effects/time/Delay.h"
#include "effects/dynamics/Compressor.h"
#include "effects/filter/LowPass.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>

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
//   status                                   → print full router state
//
// Effect names: distortion, bitcrusher, chorus, tremolo, delay, compressor, lowpass
// Input names:  in1, in2, mix, lane0, lane1, lane2, lane3
// Output names: out1, out2, both, none
//
// =============================================================================

class SerialController {
public:
    void Init(Router* router, float sample_rate, daisy::UsbHandle* usb = nullptr) {
        router_ = router;
        sample_rate_ = sample_rate;
        usb_ = usb;
        buf_pos_ = 0;
    }

    // Feed one character at a time (call from main loop)
    void Feed(char c) {
        if (c == '\n' || c == '\r') {
            if (buf_pos_ > 0) {
                buf_[buf_pos_] = '\0';
                Execute(buf_);
                buf_pos_ = 0;
            }
        } else if (buf_pos_ < MAX_CMD_LEN - 1) {
            buf_[buf_pos_++] = c;
        }
    }

private:
    static constexpr int MAX_CMD_LEN = 128;
    static constexpr int MAX_TOKENS  = 8;    Router* router_ = nullptr;
    float   sample_rate_ = 48000.f;
    daisy::UsbHandle* usb_ = nullptr;
    char    buf_[MAX_CMD_LEN] = {};
    int     buf_pos_ = 0;

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
        else if (strcmp(tokens[0], "level") == 0)  CmdLevel(tokens, n);
        else if (strcmp(tokens[0], "params") == 0) CmdParams(tokens, n);
        else if (strcmp(tokens[0], "status") == 0) CmdStatus();
        else Reply("ERR unknown command\n");
    }

    // ─── add <lane> <effect> ─────────────────────────────────────────────────
    void CmdAdd(char** t, int n) {
        if (n < 3) { Reply("ERR usage: add <lane> <effect>\n"); return; }
        int lane = atoi(t[1]);
        if (!ValidLane(lane)) return;

        Effect* fx = CreateFromName(t[2]);
        if (!fx) { Reply("ERR unknown effect: %s\n", t[2]); return; }

        router_->lanes[lane].Add(fx);
        Reply("OK added %s to lane %d slot %d\n", t[2], lane, router_->lanes[lane].count - 1);
    }

    // ─── insert <lane> <slot> <effect> ───────────────────────────────────────
    void CmdInsert(char** t, int n) {
        if (n < 4) { Reply("ERR usage: insert <lane> <slot> <effect>\n"); return; }
        int lane = atoi(t[1]);
        int slot = atoi(t[2]);
        if (!ValidLane(lane)) return;

        Effect* fx = CreateFromName(t[3]);
        if (!fx) { Reply("ERR unknown effect: %s\n", t[3]); return; }

        router_->lanes[lane].Insert(slot, fx);
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
            Reply("OK %s = %.4f\n", t[3], val);
        }
    }

    // ─── get <lane> <slot> <param> ───────────────────────────────────────────
    void CmdGet(char** t, int n) {
        if (n < 4) { Reply("ERR usage: get <lane> <slot> <param>\n"); return; }
        int lane = atoi(t[1]);
        int slot = atoi(t[2]);
        if (!ValidLane(lane) || !ValidSlot(lane, slot)) return;        float val = router_->lanes[lane].slots[slot]->GetParam(t[3]);
        Reply("VAL %s = %.4f\n", t[3], val);
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
        router_->lanes[lane].slots[slot]->SetEnabled(enabled);
        Reply("OK bypass lane %d slot %d = %s\n", lane, slot, enabled ? "on" : "off");
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
        Reply("OK level lane %d = %.2f\n", lane, router_->lanes[lane].level);
    }

    // ─── status ──────────────────────────────────────────────────────────────
    void CmdStatus() {
        Reply("=== Router Status ===\n");
        for (int r = 0; r < Router::MAX_LANES; r++) {
            auto& lane = router_->lanes[r];
            if (!lane.active) continue;
            Reply("Lane %d [%s -> %s] level=%.2f\n",
                  r, InputName(lane.input), OutputName(lane.output), lane.level);
            for (int i = 0; i < lane.count; i++) {
                Reply("  [%d] %s %s\n", i,
                      lane.slots[i]->GetName(),
                      lane.slots[i]->IsEnabled() ? "" : "(bypassed)");
            }
        }
        Reply("=====================\n");
    }

    // ─── Effect Factory ──────────────────────────────────────────────────────
    // This is the single place that maps a name string to a concrete type.
    Effect* CreateFromName(const char* name) {
        Effect* fx = nullptr;

        if      (strcmp(name, "distortion") == 0) fx = new TanhDistortion();
        else if (strcmp(name, "bitcrusher") == 0) fx = new Bitcrusher();
        else if (strcmp(name, "chorus") == 0)     fx = new Chorus();
        else if (strcmp(name, "tremolo") == 0)    fx = new Tremolo();
        else if (strcmp(name, "delay") == 0)      fx = new Delay();
        else if (strcmp(name, "compressor") == 0) fx = new Compressor();
        else if (strcmp(name, "lowpass") == 0)    fx = new LowPass();
        else return nullptr;

        fx->Init(sample_rate_);
        return fx;
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
    }    // ─── Utilities ───────────────────────────────────────────────────────────
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
    }void Reply(const char* fmt, ...) {
        char out[128];
        va_list args;
        va_start(args, fmt);
        int len = vsnprintf(out, sizeof(out), fmt, args);
        va_end(args);
        if (usb_ && len > 0) {
            usb_->TransmitInternal(reinterpret_cast<uint8_t*>(out), static_cast<size_t>(len));
        }
    }
};
