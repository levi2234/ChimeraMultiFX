#pragma once
#include "Effect.h"
#include <cmath>
#include <cstring>

// Feed-forward compressor for controlling guitar dynamics. The rectified input
// drives an envelope follower with a fast attack and slower release. Samples
// above the threshold are attenuated by a power curve derived from the selected
// ratio, then makeup gain restores the desired output level. Coefficients are
// cached outside the audio path and gain changes are smoothed to avoid zipper
// noise when parameters arrive over serial.
class Compressor : public Effect {
public:
    void Init(float sample_rate) override {
        sample_rate_ = sample_rate;
        threshold_ = 0.3f;   // signal level where compression kicks in
        ratio_     = 4.0f;   // compression ratio (4:1)
        attack_    = 0.01f;  // seconds
        release_   = 0.1f;   // seconds
        makeup_    = 1.5f;   // output gain
        env_       = 0.0f;
        gain_      = 1.0f;
        makeup_smoothed_ = makeup_;
        smoothing_coeff_ = 1.0f - expf(-2.0f * 3.14159265f * 35.0f / sample_rate_);
        UpdateAttackCoeff();
        UpdateReleaseCoeff();
        UpdateGainExponent();
    }

    float Process(float in) override {
        // Rectification turns the bipolar waveform into a level estimate. The
        // attack coefficient catches peaks while release lets the detector fall
        // naturally between notes; both are precomputed when controls change.
        float abs_in = fabsf(in);
        float coeff = abs_in > env_ ? attack_coeff_ : release_coeff_;
        env_ = coeff * env_ + (1.0f - coeff) * abs_in;

        // This power law is algebraically equivalent to converting the amount
        // above threshold to decibels, applying the ratio, then converting back.
        // It avoids a log10 plus a second power operation on every active sample.
        float target_gain = 1.0f;
        if (env_ > threshold_) {
            target_gain = powf(env_ / threshold_, gain_exponent_);
        }

        // Serial parameter changes are stepwise. Slewing the final gain and
        // makeup controls prevents those steps from becoming audible clicks.
        gain_ += smoothing_coeff_ * (target_gain - gain_);
        makeup_smoothed_ += smoothing_coeff_ * (makeup_ - makeup_smoothed_);
        return in * gain_ * makeup_smoothed_;
    }

    const char* GetName() const override { return "compressor"; }
    EffectCategory GetCategory() const override { return EffectCategory::Dynamics; }

    void SetParam(const char* name, float value) override {
        if      (strcmp(name, "threshold") == 0) SetThreshold(value);
        else if (strcmp(name, "ratio") == 0)     SetRatio(value);
        else if (strcmp(name, "attack") == 0)    SetAttack(value);
        else if (strcmp(name, "release") == 0)   SetRelease(value);
        else if (strcmp(name, "makeup") == 0)    SetMakeup(value);
    }

    float GetParam(const char* name) override {
        if      (strcmp(name, "threshold") == 0) return threshold_;
        else if (strcmp(name, "ratio") == 0)     return ratio_;
        else if (strcmp(name, "attack") == 0)    return attack_;
        else if (strcmp(name, "release") == 0)   return release_;
        else if (strcmp(name, "makeup") == 0)    return makeup_;
        return 0.f;
    }

    const char* GetParamList() const override { return "threshold,ratio,attack,release,makeup"; }

    int GetParamCount() const override { return 5; }
    bool GetParamInfo(int index, EffectParamInfo& info) const override {
        switch (index) {
            case 0: info = {"threshold", "Threshold", "", "float", "linear", 0.0001f, 1.0f, 0.3f, 0.001f}; return true;
            case 1: info = {"ratio", "Ratio", ":1", "float", "linear", 1.0f, 20.0f, 4.0f, 0.1f}; return true;
            case 2: info = {"attack", "Attack", "s", "float", "log", 0.0001f, 2.0f, 0.01f, 0.0001f}; return true;
            case 3: info = {"release", "Release", "s", "float", "log", 0.0001f, 5.0f, 0.1f, 0.0001f}; return true;
            case 4: info = {"makeup", "Makeup", "", "float", "linear", 0.0f, 8.0f, 1.5f, 0.01f}; return true;
            default: return false;
        }
    }

    void SetThreshold(float t)  { threshold_ = Clamp(t, 0.0001f, 1.0f); }
    void SetRatio(float r)      { ratio_ = Clamp(r, 1.0f, 20.0f); UpdateGainExponent(); }
    void SetAttack(float sec)   { attack_ = Clamp(sec, 0.0001f, 2.0f); UpdateAttackCoeff(); }
    void SetRelease(float sec)  { release_ = Clamp(sec, 0.0001f, 5.0f); UpdateReleaseCoeff(); }
    void SetMakeup(float g)     { makeup_ = Clamp(g, 0.0f, 8.0f); }

private:
    void UpdateAttackCoeff() {
        attack_coeff_ = expf(-1.0f / (attack_ * sample_rate_));
    }

    void UpdateReleaseCoeff() {
        release_coeff_ = expf(-1.0f / (release_ * sample_rate_));
    }

    void UpdateGainExponent() {
        gain_exponent_ = -(1.0f - (1.0f / ratio_));
    }

    float sample_rate_;
    float threshold_, ratio_;
    float attack_, release_;
    float makeup_;
    float env_;
    float attack_coeff_;
    float release_coeff_;
    float gain_exponent_;
    float gain_;
    float makeup_smoothed_;
    float smoothing_coeff_;
};
