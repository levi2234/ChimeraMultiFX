#pragma once
#include "Effect.h"
#include <cmath>
#include <cstring>

// Symmetric soft-clipping distortion for smooth, amp-like saturation.
//
// The nonlinearity creates harmonics above Nyquist, which would fold back as
// harsh aliasing at the normal sample rate.  The shaper therefore runs at 2x:
// a linear midpoint supplies the extra sample, and a reconstruction low-pass
// removes much of the ultrasonic energy before returning to the host rate.
class TanhDistortion : public Effect {
public:
    void Init(float sample_rate) override {
        sample_rate_ = sample_rate;
        drive_ = 5.0f;
        mix_   = 1.0f;
        level_ = 0.7f;
        pre_state_ = 0.0f;
        previous_pre_ = 0.0f;
        post_state_ = 0.0f;
        pre_coeff_ = LowPassCoeff(12000.0f, sample_rate_);
        post_coeff_ = LowPassCoeff(10000.0f, sample_rate_ * 2.0f);
    }

    float Process(float in) override {
        // Band-limit the guitar signal before generating new harmonics.  The
        // exact one-pole coefficient keeps this filter effective at 48 kHz.
        pre_state_ += pre_coeff_ * (in - pre_state_);

        float midpoint = 0.5f * (previous_pre_ + pre_state_);
        previous_pre_ = pre_state_;
        ProcessOversample(midpoint);
        ProcessOversample(pre_state_);

        float wet = post_state_ * level_;
        return (in * (1.0f - mix_)) + (wet * mix_);
    }

    const char* GetName() const override { return "distortion"; }
    EffectCategory GetCategory() const override { return EffectCategory::Distortion; }

    void SetParam(const char* name, float value) override {
        if      (strcmp(name, "drive") == 0) SetDrive(value);
        else if (strcmp(name, "mix") == 0)   SetMix(value);
        else if (strcmp(name, "level") == 0) SetLevel(value);
    }    float GetParam(const char* name) override {
        if      (strcmp(name, "drive") == 0) return drive_;
        else if (strcmp(name, "mix") == 0)   return mix_;
        else if (strcmp(name, "level") == 0) return level_;
        return 0.f;
    }

    const char* GetParamList() const override { return "drive,mix,level"; }

    int GetParamCount() const override { return 3; }
    bool GetParamInfo(int index, EffectParamInfo& info) const override {
        switch (index) {
            case 0: info = {"drive", "Drive", "", "float", "linear", 0.0f, 18.0f, 5.0f, 0.1f}; return true;
            case 1: info = {"mix", "Mix", "", "float", "linear", 0.0f, 1.0f, 1.0f, 0.01f}; return true;
            case 2: info = {"level", "Level", "", "float", "linear", 0.0f, 2.0f, 0.7f, 0.01f}; return true;
            default: return false;
        }
    }

    void SetDrive(float d) { drive_ = Clamp(d, 0.0f, 18.0f); }
    void SetMix(float m)   { mix_ = Clamp(m, 0.0f, 1.0f); }
    void SetLevel(float l) { level_ = Clamp(l, 0.0f, 2.0f); }

private:
    float LowPassCoeff(float cutoff, float rate) const {
        // 1 - exp(-2*pi*fc/fs) is the matched one-pole coefficient.  Unlike
        // the small-angle approximation it does not become an accidental
        // bypass when the cutoff is a substantial fraction of sample rate.
        float exponent = -2.0f * 3.14159265f * cutoff / rate;
        return Clamp(1.0f - expf(exponent), 0.0001f, 0.99f);
    }

    float ProcessOversample(float in) {
        // drive=0 is intentionally clean; higher values progressively push a
        // bounded tanh curve.  The post filter doubles as the 2x downsampler.
        float shaped = drive_ <= 0.0001f ? in : tanhf(in * (1.0f + drive_));
        post_state_ += post_coeff_ * (shaped - post_state_);
        return post_state_;
    }

    float sample_rate_;
    float drive_;
    float mix_;
    float level_;
    float pre_state_;
    float previous_pre_;
    float post_state_;
    float pre_coeff_;
    float post_coeff_;
};
