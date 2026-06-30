#pragma once
#include "../../Effect.h"
#include <cmath>

// Soft-clipping distortion using tanh waveshaping.
class TanhDistortion : public Effect {
public:
    void Init(float sample_rate) override {
        (void)sample_rate;
        drive_ = 8.0f;
        mix_   = 1.0f;
        level_ = 0.7f;
    }

    float Process(float in) override {
        float wet = tanhf(in * drive_) * level_;
        return (in * (1.0f - mix_)) + (wet * mix_);
    }

    const char* GetName() const override { return "Tanh Distortion"; }
    EffectCategory GetCategory() const override { return EffectCategory::Distortion; }

    void SetDrive(float d) { drive_ = d; }
    void SetMix(float m)   { mix_ = m; }
    void SetLevel(float l) { level_ = l; }

private:
    float drive_;
    float mix_;
    float level_;
};
