#include "daisy_seed.h"
#include <cmath> // For tanhf

using namespace daisy;

DaisySeed hw;

// ==========================================
// DEFINE YOUR FIXED STATE HERE
// ==========================================
const float DRIVE_GAIN = 8.0f;   // Input boost (1.0 = clean, 10.0 = heavy distortion)
const float MIX_LEVEL  = 1.0f;   // 0.0 = dry, 1.0 = fully distorted
const float OUT_LEVEL  = 0.7f;   // Master volume adjust to prevent speaker clipping

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
    for (size_t i = 0; i < size; i++) {
        float dry_signal = in[0][i];

        // 1. Boost the input signal 
        float boosted_signal = dry_signal * DRIVE_GAIN;

        // 2. Soft-clip using standard math library tanhf (hyperbolic tangent)
        // This rounds off the peaks of the audio wave smoothly.
        float wet_signal = tanhf(boosted_signal);

        // 3. Compensate for the added volume boost
        wet_signal *= OUT_LEVEL;

        // 4. Blend dry and wet signals
        out[0][i] = (dry_signal * (1.0f - MIX_LEVEL)) + (wet_signal * MIX_LEVEL);
        out[1][i] = out[0][i]; // Stereo mirror
    }
}

int main(void) {
    hw.Init();
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_96KHZ);
    // Start the audio engine
    hw.StartAudio(AudioCallback);

    while (1) {
        System::Delay(100);
    }
}