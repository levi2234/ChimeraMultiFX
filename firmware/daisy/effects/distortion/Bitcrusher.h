#pragma once
#include "../../Effect.h"
#include <cmath>
#include <cstring>

// Reduces bit depth and sample rate for lo-fi grit.
class Bitcrusher : public Effect {
public:
    void Init(float sample_rate) override {
        sample_rate_  = sample_rate;
        bit_depth_    = 8.0f;    // 1–16
        rate_reduce_  = 4;       // keep every Nth sample
        mix_          = 1.0f;
        hold_         = 0.0f;
        counter_      = 0;
    }

    float Process(float in) override {
        // Sample rate reduction: hold the value for N samples
        if (++counter_ >= rate_reduce_) {
            counter_ = 0;
            // Bit depth reduction: quantize to fewer levels
            float levels = powf(2.0f, bit_depth_);
            hold_ = roundf(in * levels) / levels;
        }
        return (in * (1.0f - mix_)) + (hold_ * mix_);
    }    const char* GetName() const override { return "bitcrusher"; }
    EffectCategory GetCategory() const override { return EffectCategory::Distortion; }

    void SetParam(const char* name, float value) override {
        if      (strcmp(name, "bits") == 0)    bit_depth_ = value;
        else if (strcmp(name, "rate") == 0)    rate_reduce_ = (int)value;
        else if (strcmp(name, "mix") == 0)     mix_ = value;
    }    float GetParam(const char* name) override {
        if      (strcmp(name, "bits") == 0)    return bit_depth_;
        else if (strcmp(name, "rate") == 0)    return (float)rate_reduce_;
        else if (strcmp(name, "mix") == 0)     return mix_;
        return 0.f;
    }

    const char* GetParamList() const override { return "bits,rate,mix"; }

    void SetBitDepth(float bits) { bit_depth_ = bits; }
    void SetRateReduce(int n)    { rate_reduce_ = (n < 1) ? 1 : n; }
    void SetMix(float m)         { mix_ = m; }

private:
    float sample_rate_;
    float bit_depth_;
    int   rate_reduce_;
    float mix_;
    float hold_;
    int   counter_;
};
