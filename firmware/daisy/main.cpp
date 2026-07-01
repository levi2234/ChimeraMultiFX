// =============================================================================
// ChimeraMultiFX — Main Entry Point
// =============================================================================
// Single audio loop delegates processing to the currently active Effect.
// Effects can be hot-swapped at runtime by calling SetActiveEffect().
// =============================================================================
#include "daisy_seed.h"
#include "Effect.h"
#include "Router.h"

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
// Effect Instances
// ==========================================
static TanhDistortion fx_distortion;
static Bitcrusher     fx_bitcrusher;
static Chorus         fx_chorus;
static Tremolo        fx_tremolo;
static Delay          fx_delay;
static Compressor     fx_compressor;
static LowPass        fx_lowpass;

// ==========================================
// Router
// ==========================================
static Router router;

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

    // ── Build the routing grid ──────────────────────────────
    // Single lane: Input 1 → all effects → both outputs
    router.lanes[0].input  = InputSource::In_1;
    router.lanes[0].output = OutputDest::Out_Both;
    router.lanes[0].Add(&fx_compressor);
    router.lanes[0].Add(&fx_distortion);
    router.lanes[0].Add(&fx_bitcrusher);
    router.lanes[0].Add(&fx_chorus);
    router.lanes[0].Add(&fx_tremolo);
    router.lanes[0].Add(&fx_delay);
    router.lanes[0].Add(&fx_lowpass);

    // Initialize all effects in all lanes
    router.Init(hw.AudioSampleRate());

    // Start audio processing
    hw.StartAudio(AudioCallback);

    // Main loop — add your UI/control logic here (buttons, knobs, MIDI, etc.)
    while (1) {
        System::Delay(10);
    }
}
