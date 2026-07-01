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
static Router router;
static SerialController serial;

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
    for (size_t i = 0; i < size; i++) {
        auto result = router.Process(in[0][i], in[1][i]);
        out[0][i] = result.out1;
        out[1][i] = result.out2;
    }
}

// ==========================================
// Main
// ==========================================
int main(void) {
    hw.Init();
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_96KHZ);

    // Initialize USB CDC for serial communication
    hw.usb_handle.Init(UsbHandle::FS_INTERNAL);
    System::Delay(500);  // Give USB time to enumerate on the host PC    float sr = hw.AudioSampleRate();
    serial.Init(&router, sr, &hw.usb_handle);

    // Register USB receive callback (interrupt-driven)
    hw.usb_handle.SetReceiveCallback(UsbRxCallback, UsbHandle::FS_INTERNAL);

    // Start audio processing
    hw.StartAudio(AudioCallback);

    // Main loop — nothing else needed, USB rx is interrupt-driven
    while (1) {
        System::Delay(10);
    }
}
