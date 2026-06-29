#pragma once
#include "../Effect.h"
#include <cmath> // tanhf

// Soft-clipping distortion using tanh waveshaping.
class Distortion : public Effect {
public:
    void Init(float sample_rate) override {
        (void)sample_rate; // Not needed for this effect
        drive_gain_ = 8.0f;
        mix_       = 1.0f;
        out_level_ = 0.7f;
    }

    float Process(float in) override {
        float boosted = in * drive_gain_;
        float wet = tanhf(boosted) * out_level_;
        return (in * (1.0f - mix_)) + (wet * mix_);
    }

    const char* GetName() const override { return "Distortion"; }

    // --- Setters for runtime control ---
    void SetDrive(float d) { drive_gain_ = d; }
    void SetMix(float m)   { mix_ = m; }
    void SetLevel(float l) { out_level_ = l; }

private:
    float drive_gain_;
    float mix_;
    float out_level_;
};
