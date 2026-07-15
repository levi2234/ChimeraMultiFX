#pragma once
#include "Effect.h"
#include <cmath>
#include <cstring>

// Reduces bit depth and sample rate for lo-fi grit.
class Bitcrusher : public Effect {
public:
    void Init(float sample_rate) override {
        sample_rate_  = sample_rate;
        bit_depth_    = 12.0f;
        rate_reduce_  = 1;
        mix_          = 0.5f;
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
        if      (strcmp(name, "bits") == 0)    SetBitDepth(value);
        else if (strcmp(name, "rate") == 0)    SetRateReduce((int)value);
        else if (strcmp(name, "mix") == 0)     SetMix(value);
    }    float GetParam(const char* name) override {
        if      (strcmp(name, "bits") == 0)    return bit_depth_;
        else if (strcmp(name, "rate") == 0)    return (float)rate_reduce_;
        else if (strcmp(name, "mix") == 0)     return mix_;
        return 0.f;
    }

    const char* GetParamList() const override { return "bits,rate,mix"; }

    int GetParamCount() const override { return 3; }
    bool GetParamInfo(int index, EffectParamInfo& info) const override {
        switch (index) {
            case 0: info = {"bits", "Bits", "", "int", "linear", 1.0f, 16.0f, 12.0f, 1.0f}; return true;
            case 1: info = {"rate", "Rate Divide", "samples", "int", "linear", 1.0f, 256.0f, 1.0f, 1.0f}; return true;
            case 2: info = {"mix", "Mix", "", "float", "linear", 0.0f, 1.0f, 0.5f, 0.01f}; return true;
            default: return false;
        }
    }

    void SetBitDepth(float bits) { bit_depth_ = Clamp(bits, 1.0f, 16.0f); }
    void SetRateReduce(int n)    { rate_reduce_ = ClampInt(n, 1, 256); }
    void SetMix(float m)         { mix_ = Clamp(m, 0.0f, 1.0f); }

private:
    float sample_rate_;
    float bit_depth_;
    int   rate_reduce_;
    float mix_;
    float hold_;
    int   counter_;
};
