#pragma once
#include "Effect.h"
#include <cmath>
#include <cstring>

// Tunable band-pass for focused, radio-like, or resonant guitar tones.
// The constant-skirt-gain RBJ biquad keeps peak gain bounded as resonance
// changes, while the dry/wet path permits useful parallel filtering.
class BandPass : public Effect {
public:
    void Init(float sample_rate) override {
        sample_rate_ = sample_rate > 1000.0f ? sample_rate : 48000.0f;
        frequency_ = 900.0f;
        resonance_ = 0.35f;
        mix_ = 1.0f;
        x1_ = x2_ = y1_ = y2_ = 0.0f;
        UpdateCoefficients();
    }

    float Process(float in) override {
        const float wet = b0_ * in + b2_ * x2_ - a1_ * y1_ - a2_ * y2_;
        x2_ = x1_; x1_ = in;
        y2_ = y1_; y1_ = wet;
        return in * (1.0f - mix_) + wet * mix_;
    }

    const char* GetName() const override { return "bandpass"; }
    EffectCategory GetCategory() const override { return EffectCategory::Filter; }

    void SetParam(const char* name, float value) override {
        if (strcmp(name, "frequency") == 0) { frequency_ = Clamp(value, 20.0f, MaxFrequency()); UpdateCoefficients(); }
        else if (strcmp(name, "resonance") == 0) { resonance_ = Clamp(value, 0.0f, 1.0f); UpdateCoefficients(); }
        else if (strcmp(name, "mix") == 0) mix_ = Clamp(value, 0.0f, 1.0f);
    }

    float GetParam(const char* name) override {
        if      (strcmp(name, "frequency") == 0) return frequency_;
        else if (strcmp(name, "resonance") == 0) return resonance_;
        else if (strcmp(name, "mix") == 0) return mix_;
        return 0.0f;
    }

    const char* GetParamList() const override { return "frequency,resonance,mix"; }
    int GetParamCount() const override { return 3; }
    bool GetParamInfo(int index, EffectParamInfo& info) const override {
        switch (index) {
            case 0: info = {"frequency", "Frequency", "Hz", "float", "log", 20.0f, MaxFrequency(), 900.0f, 1.0f}; return true;
            case 1: info = {"resonance", "Resonance", "", "float", "linear", 0.0f, 1.0f, 0.35f, 0.01f}; return true;
            case 2: info = {"mix", "Mix", "", "float", "linear", 0.0f, 1.0f, 1.0f, 0.01f}; return true;
            default: return false;
        }
    }

private:
    float MaxFrequency() const { return sample_rate_ * 0.45f < 20000.0f ? sample_rate_ * 0.45f : 20000.0f; }
    void UpdateCoefficients() {
        const float omega = 2.0f * 3.14159265f * frequency_ / sample_rate_;
        const float q = 0.5f + resonance_ * 9.5f;
        const float alpha = sinf(omega) / (2.0f * q);
        const float inverse_a0 = 1.0f / (1.0f + alpha);
        b0_ = alpha * inverse_a0;
        b2_ = -b0_;
        a1_ = -2.0f * cosf(omega) * inverse_a0;
        a2_ = (1.0f - alpha) * inverse_a0;
    }

    float sample_rate_, frequency_, resonance_, mix_;
    float b0_, b2_, a1_, a2_;
    float x1_, x2_, y1_, y2_;
};