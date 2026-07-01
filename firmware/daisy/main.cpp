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
    hw.StartLog();  // Enable USB serial output

    float sr = hw.AudioSampleRate();
    serial.Init(&router, sr);

    // Start audio processing
    hw.StartAudio(AudioCallback);

    // Main loop — read serial commands
    while (1) {
        // Read any available bytes from USB serial
        uint8_t byte;
        while (hw.usb_handle.Receive(&byte, 1) > 0) {
            serial.Feed(static_cast<char>(byte));
        }
        System::Delay(1);
    }
}
