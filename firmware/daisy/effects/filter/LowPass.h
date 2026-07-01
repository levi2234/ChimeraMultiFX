#pragma once
#include "../../Effect.h"
#include <cmath>
#include <cstring>

// Simple one-pole resonant low-pass filter.
class LowPass : public Effect {
public:
    void Init(float sample_rate) override {
        sample_rate_ = sample_rate;
        cutoff_      = 2000.0f;  // Hz
        resonance_   = 0.5f;     // 0–1
        mix_         = 1.0f;
        y1_ = y2_ = x1_ = x2_ = 0.0f;
        CalcCoeffs();
    }

    float Process(float in) override {
        // Biquad LPF: y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
        float wet = b0_*in + b1_*x1_ + b2_*x2_ - a1_*y1_ - a2_*y2_;
        x2_ = x1_; x1_ = in;
        y2_ = y1_; y1_ = wet;
        return (in * (1.0f - mix_)) + (wet * mix_);
    }    const char* GetName() const override { return "lowpass"; }
    EffectCategory GetCategory() const override { return EffectCategory::Filter; }

    void SetParam(const char* name, float value) override {
        if      (strcmp(name, "cutoff") == 0)    { cutoff_ = value; CalcCoeffs(); }
        else if (strcmp(name, "resonance") == 0) { resonance_ = value; CalcCoeffs(); }
        else if (strcmp(name, "mix") == 0)         mix_ = value;
    }

    float GetParam(const char* name) override {
        if      (strcmp(name, "cutoff") == 0)    return cutoff_;
        else if (strcmp(name, "resonance") == 0) return resonance_;
        else if (strcmp(name, "mix") == 0)       return mix_;
        return 0.f;
    }

    void SetCutoff(float hz)   { cutoff_ = hz; CalcCoeffs(); }
    void SetResonance(float r) { resonance_ = r; CalcCoeffs(); }
    void SetMix(float m)       { mix_ = m; }

private:
    void CalcCoeffs() {
        float w0    = 2.0f * 3.14159265f * cutoff_ / sample_rate_;
        float alpha = sinf(w0) / (2.0f * (1.0f - resonance_ * 0.99f)); // Q from resonance
        float cosw0 = cosf(w0);
        float a0    = 1.0f + alpha;
        b0_ = ((1.0f - cosw0) * 0.5f) / a0;
        b1_ = (1.0f - cosw0) / a0;
        b2_ = b0_;
        a1_ = (-2.0f * cosw0) / a0;
        a2_ = (1.0f - alpha) / a0;
    }

    float sample_rate_;
    float cutoff_, resonance_, mix_;
    float b0_, b1_, b2_, a1_, a2_;
    float x1_, x2_, y1_, y2_;
};
