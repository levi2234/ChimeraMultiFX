#include "daisy_seed.h"

using namespace daisy;

DaisySeed hw;

// ==========================================
// DEFINE YOUR FIXED STATE HERE
// ==========================================
#define SAMPLE_RATE 96000
const float DELAY_TIME_SEC = 0.5f;  // Delay time in seconds (e.g., 0.5s = 500ms)
const float FEEDBACK       = 0.6f;  // Repeats (0.0 = one echo, 0.9 = long tail)
const float MIX_LEVEL      = 0.4f;  // 0.0 = dry only, 1.0 = wet only

// Allocate delay buffer in SDRAM to prevent crashing from large memory usage
#define MAX_DELAY_SAMPLES (SAMPLE_RATE * 2) // 2 seconds max capacity
float DSY_SDRAM_BSS delay_buffer[MAX_DELAY_SAMPLES];

// Track our position in the buffer
uint32_t write_ptr = 0;
uint32_t delay_samples = 0;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
    for (size_t i = 0; i < size; i++) {
        float dry_signal = in[0][i];

        // 1. Calculate where to read from the past
        // If write_ptr is less than delay_samples, it wraps around seamlessly using modulo
        uint32_t read_ptr = (write_ptr - delay_samples + MAX_DELAY_SAMPLES) % MAX_DELAY_SAMPLES;
        
        // 2. Grab the delayed "wet" signal
        float wet_signal = delay_buffer[read_ptr];

        // 3. Mix current input with a feedback portion of the echo, then write to buffer
        float input_to_buffer = dry_signal + (wet_signal * FEEDBACK);
        delay_buffer[write_ptr] = input_to_buffer;

        // 4. Advance the write pointer (wrap around if it hits the end of the buffer)
        write_ptr = (write_ptr + 1) % MAX_DELAY_SAMPLES;

        // 5. Blend dry and wet signals to the outputs
        out[0][i] = (dry_signal * (1.0f - MIX_LEVEL)) + (wet_signal * MIX_LEVEL);
        out[1][i] = out[0][i]; // Stereo mirror
    }
}

int main(void) {
    hw.Init();
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_96KHZ);
    
    // Initialize buffer to zeroes to avoid loud initial pops/buzzes
    for (size_t i = 0; i < MAX_DELAY_SAMPLES; i++) {
        delay_buffer[i] = 0.0f;
    }

    // Convert seconds into actual samples based on the hardware sample rate
    delay_samples = (uint32_t)(DELAY_TIME_SEC * SAMPLE_RATE);
    if (delay_samples >= MAX_DELAY_SAMPLES) {
        delay_samples = MAX_DELAY_SAMPLES - 1; // Safety clamp
    }

    // Start the audio engine
    hw.StartAudio(AudioCallback);

    while (1) {
        System::Delay(100);
    }
}