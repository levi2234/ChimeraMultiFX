// =============================================================================
// ChimeraMultiFX — Main Entry Point
// =============================================================================
// Single audio loop delegates processing to the currently active Effect.
// Effects can be hot-swapped at runtime by calling SetActiveEffect().
// =============================================================================
#include "daisy_seed.h"
#include "Effect.h"
#include "effects/Distortion.h"
#include "effects/Delay.h"

using namespace daisy;

// ==========================================
// Hardware & Global State
// ==========================================
static DaisySeed hw;

// ==========================================
// Effect Chain
// ==========================================
static Distortion fx_distortion;
static Delay      fx_delay;

static constexpr int NUM_EFFECTS = 2;
static Effect* chain[NUM_EFFECTS] = { &fx_distortion, &fx_delay };

// ==========================================
// Audio Callback
// ==========================================
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
    for (size_t i = 0; i < size; i++) {
        float sample = in[0][i];

        // Signal flows through each effect in order
        for (int fx = 0; fx < NUM_EFFECTS; fx++) {
            sample = chain[fx]->Process(sample);
        }

        out[0][i] = sample;
        out[1][i] = sample;
    }
}

// ==========================================
// Main
// ==========================================
int main(void) {
    hw.Init();
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_96KHZ);
    float sample_rate = hw.AudioSampleRate();   
    // Initialize all effects in the chain
    for (int i = 0; i < NUM_EFFECTS; i++) {
        chain[i]->Init(sample_rate);
    }

    // Start audio processing
    hw.StartAudio(AudioCallback);

    // Main loop — add your UI/control logic here (buttons, knobs, MIDI, etc.)
    while (1) {
        System::Delay(10);
    }
}
