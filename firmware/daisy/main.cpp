// =============================================================================
// ChimeraMultiFX — Main Entry Point
// =============================================================================
// Effects are created dynamically via UART serial commands.
// No effects are pre-instantiated — the SerialController handles everything.
// =============================================================================
#include "daisy_seed.h"
#include "Router.h"
#include "SerialController.h"

using namespace daisy;

// ==========================================
// Hardware & Global State
// ==========================================
static DaisySeed hw; 
static Router router; // Audio routing engine that manages lanes and effects
static SerialController serial; // UART command interface that allows dynamic control of the router and effects
static UartHandler esp_uart;
static uint8_t esp_uart_rx_dma_buffer[SerialController::UART_RX_DMA_BUFFER_LEN];

void EspUartRxCallback(uint8_t* data,
                       size_t size,
                       void* context,
                       UartHandler::Result result) {
    if (result == UartHandler::Result::OK && context != nullptr) {
        static_cast<SerialController*>(context)->QueueUartBytes(data, size);
    }
}

// ==========================================
// USB CDC Receive Callback
// ==========================================
// Called by libDaisy when bytes arrive over USB serial.
void UsbRxCallback(uint8_t* buf, uint32_t* len) {
    for (uint32_t i = 0; i < *len; i++) {
        serial.Feed(static_cast<char>(buf[i]));
    }
}

// ==========================================
// Audio Callback
// ==========================================
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
    serial.BeginAudioCallback(size);

    for (size_t i = 0; i < size; i++) {
        auto result = router.Process(in[0][i], in[1][i]);
        out[0][i] = result.out1;
        out[1][i] = result.out2;
    }

    serial.RecordAudioCallback();
}

// ==========================================
// Main
// ==========================================
int main(void) {
    hw.Init();
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    float sr = hw.AudioSampleRate();

    // Initialize USB CDC for serial communication
    hw.usb_handle.Init(UsbHandle::FS_INTERNAL);
    System::Delay(500);  // Give USB time to enumerate on the host PC

    UartHandler::Config esp_uart_config;
    esp_uart_config.periph = UartHandler::Config::Peripheral::USART_1;
    esp_uart_config.mode = UartHandler::Config::Mode::TX_RX;
    esp_uart_config.baudrate = 115200;
    esp_uart_config.pin_config.tx = seed::D13; // Physical pin 14 / PB6 / USART1_TX, wire to ESP32 GPIO16 RX
    esp_uart_config.pin_config.rx = seed::D14; // Physical pin 15 / PB7 / USART1_RX, wire from ESP32 GPIO17 TX
    esp_uart.Init(esp_uart_config);

    serial.Init(&router, sr, &hw.usb_handle, &esp_uart);
    serial.StartUartDmaReceive(esp_uart_rx_dma_buffer,
                               sizeof(esp_uart_rx_dma_buffer),
                               EspUartRxCallback,
                               &serial);

    // Register USB receive callback (interrupt-driven)
    hw.usb_handle.SetReceiveCallback(UsbRxCallback, UsbHandle::FS_INTERNAL);

    // Start audio processing
    hw.StartAudio(AudioCallback);

    while (1) {
        serial.ProcessPendingUart();
        serial.ProcessPendingActions();
    }
}
