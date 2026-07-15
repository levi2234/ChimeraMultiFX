#pragma once
#include "Effect.h"
#include <cmath>
#include <cstring>

// Noise gate for muting pickup and pedal-chain hiss between played notes.
// An absolute-value envelope avoids rectifier ripple, while separate attack
// and release smoothing lets notes open promptly and tails close naturally.
// A 6 dB soft knee drives a second smoothed gain stage to prevent chatter and
// zippering when the detector hovers around the threshold.
class NoiseGate : public Effect {
public:
    void Init(float sample_rate) override {
        sample_rate_ = sample_rate > 1000.0f ? sample_rate : 48000.0f;
        threshold_db_ = -50.0f;
        attack_ms_ = 2.0f;
        release_ms_ = 120.0f;
        threshold_linear_ = powf(10.0f, threshold_db_ / 20.0f);
        envelope_ = 0.0f;
        gain_ = 0.0f;
        UpdateCoefficients();
    }

    float Process(float in) override {
        const float magnitude = fabsf(in);
        const float detector_coeff = magnitude > envelope_ ? attack_coeff_ : release_coeff_;
        envelope_ = detector_coeff * envelope_ + (1.0f - detector_coeff) * magnitude;

        const float knee_bottom = threshold_linear_ * 0.50118723f;
        const float target = Clamp((envelope_ - knee_bottom) / (threshold_linear_ - knee_bottom), 0.0f, 1.0f);
        const float gain_coeff = target > gain_ ? attack_coeff_ : release_coeff_;
        gain_ = gain_coeff * gain_ + (1.0f - gain_coeff) * target;
        return in * gain_;
    }

    const char* GetName() const override { return "noisegate"; }
    EffectCategory GetCategory() const override { return EffectCategory::Dynamics; }

    void SetParam(const char* name, float value) override {
        if (strcmp(name, "threshold") == 0) {
            threshold_db_ = Clamp(value, -80.0f, -10.0f);
            threshold_linear_ = powf(10.0f, threshold_db_ / 20.0f);
        }
        else if (strcmp(name, "attack") == 0) { attack_ms_ = Clamp(value, 0.1f, 100.0f); UpdateCoefficients(); }
        else if (strcmp(name, "release") == 0) { release_ms_ = Clamp(value, 5.0f, 1000.0f); UpdateCoefficients(); }
    }

    float GetParam(const char* name) override {
        if      (strcmp(name, "threshold") == 0) return threshold_db_;
        else if (strcmp(name, "attack") == 0)    return attack_ms_;
        else if (strcmp(name, "release") == 0)   return release_ms_;
        return 0.0f;
    }

    const char* GetParamList() const override { return "threshold,attack,release"; }
    int GetParamCount() const override { return 3; }

    bool GetParamInfo(int index, EffectParamInfo& info) const override {
        switch (index) {
            case 0: info = {"threshold", "Threshold", "dB", "float", "linear", -80.0f, -10.0f, -50.0f, 0.5f}; return true;
            case 1: info = {"attack", "Attack", "ms", "float", "log", 0.1f, 100.0f, 2.0f, 0.1f}; return true;
            case 2: info = {"release", "Release", "ms", "float", "log", 5.0f, 1000.0f, 120.0f, 1.0f}; return true;
            default: return false;
        }
    }

private:
    void UpdateCoefficients() {
        attack_coeff_ = expf(-1.0f / (0.001f * attack_ms_ * sample_rate_));
        release_coeff_ = expf(-1.0f / (0.001f * release_ms_ * sample_rate_));
    }

    float sample_rate_;
    float threshold_db_;
    float threshold_linear_;
    float attack_ms_;
    float release_ms_;
    float attack_coeff_;
    float release_coeff_;
    float envelope_;
    float gain_;
};