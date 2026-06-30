// =============================================================================
// ChimeraMultiFX — Main Entry Point
// =============================================================================
// Single audio loop delegates processing to the currently active Effect.
// Effects can be hot-swapped at runtime by calling SetActiveEffect().
// =============================================================================
#include "daisy_seed.h"
#include "Effect.h"

// Distortion
#include "effects/distortion/TanhDistortion.h"
#include "effects/distortion/Bitcrusher.h"
// Modulation
#include "effects/modulation/Chorus.h"
#include "effects/modulation/Tremolo.h"
// Time
#include "effects/time/Delay.h"
// Dynamics
#include "effects/dynamics/Compressor.h"
// Filter
#include "effects/filter/LowPass.h"

using namespace daisy;

// ==========================================
// Hardware & Global State
// ==========================================
static DaisySeed hw;

// ==========================================
// Effect Chain
// ==========================================
static TanhDistortion fx_distortion;
static Bitcrusher     fx_bitcrusher;
static Chorus         fx_chorus;
static Tremolo        fx_tremolo;
static Delay          fx_delay;
static Compressor     fx_compressor;
static LowPass        fx_lowpass;

// Signal flows through the chain in array order.
// Rearrange, add, or remove entries to shape your sound.
static constexpr int NUM_EFFECTS = 7;
static Effect* chain[NUM_EFFECTS] = {
    &fx_compressor,   // Dynamics — tame input peaks first
    &fx_distortion,   // Distortion — tanh saturation
    &fx_bitcrusher,   // Distortion — lo-fi crush
    &fx_chorus,       // Modulation — chorus
    &fx_tremolo,      // Modulation — tremolo
    &fx_delay,        // Time — echo
    &fx_lowpass,      // Filter — tone shaping
};

// ==========================================
// Audio Callback
// ==========================================
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
    for (size_t i = 0; i < size; i++) {
        float sample = in[0][i];        // Signal flows through each effect in order (Tick respects bypass)
        for (int fx = 0; fx < NUM_EFFECTS; fx++) {
            sample = chain[fx]->Tick(sample); //Tick respects bypass and processes the sample through the effect
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
