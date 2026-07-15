#pragma once
#include "Effect.h"
#include <cmath>
#include <cstring>

// Guitar sustainer that combines gentle compression and upward gain.
// A fast/slow envelope preserves the pick transient but decays slowly, then a
// bounded inverse-level curve reduces loud peaks and lifts fading notes. This
// topology extends sustain without allocating delay buffers or regenerating
// audio, and the final dry/wet blend retains natural attack detail.
class Sustain : public Effect {
public:
    void Init(float sample_rate) override {
        sample_rate_ = sample_rate > 1000.0f ? sample_rate : 48000.0f;
        sustain_ = 0.6f;
        attack_ms_ = 12.0f;
        level_ = 1.0f;
        mix_ = 1.0f;
        envelope_ = 0.0f;
        gain_state_ = 1.0f;
        UpdateCoefficients();
    }

    float Process(float in) override {
        const float magnitude = fabsf(in);
        const float detector_coeff = magnitude > envelope_ ? attack_coeff_ : release_coeff_;
        envelope_ = detector_coeff * envelope_ + (1.0f - detector_coeff) * magnitude;

        // The exponent controls how strongly levels converge on the -15 dB
        // target. Gain is bounded so silence cannot produce runaway makeup.
        const float strength = sustain_ * 0.75f;
        const float target_gain = Clamp(powf(0.18f / (envelope_ + 0.002f), strength), 0.3f, 10.0f);
        const float gain_coeff = target_gain < gain_state_ ? attack_coeff_ : release_coeff_;
        gain_state_ = gain_coeff * gain_state_ + (1.0f - gain_coeff) * target_gain;

        const float wet = tanhf(in * gain_state_ * level_);
        return in * (1.0f - mix_) + wet * mix_;
    }

    const char* GetName() const override { return "sustain"; }
    EffectCategory GetCategory() const override { return EffectCategory::Dynamics; }

    void SetParam(const char* name, float value) override {
        if      (strcmp(name, "sustain") == 0) sustain_ = Clamp(value, 0.0f, 1.0f);
        else if (strcmp(name, "attack") == 0) { attack_ms_ = Clamp(value, 1.0f, 100.0f); UpdateCoefficients(); }
        else if (strcmp(name, "level") == 0) level_ = Clamp(value, 0.0f, 2.0f);
        else if (strcmp(name, "mix") == 0) mix_ = Clamp(value, 0.0f, 1.0f);
    }

    float GetParam(const char* name) override {
        if      (strcmp(name, "sustain") == 0) return sustain_;
        else if (strcmp(name, "attack") == 0)  return attack_ms_;
        else if (strcmp(name, "level") == 0)   return level_;
        else if (strcmp(name, "mix") == 0)     return mix_;
        return 0.0f;
    }

    const char* GetParamList() const override { return "sustain,attack,level,mix"; }
    int GetParamCount() const override { return 4; }

    bool GetParamInfo(int index, EffectParamInfo& info) const override {
        switch (index) {
            case 0: info = {"sustain", "Sustain", "", "float", "linear", 0.0f, 1.0f, 0.6f, 0.01f}; return true;
            case 1: info = {"attack", "Attack", "ms", "float", "log", 1.0f, 100.0f, 12.0f, 0.1f}; return true;
            case 2: info = {"level", "Level", "", "float", "linear", 0.0f, 2.0f, 1.0f, 0.01f}; return true;
            case 3: info = {"mix", "Mix", "", "float", "linear", 0.0f, 1.0f, 1.0f, 0.01f}; return true;
            default: return false;
        }
    }

private:
    void UpdateCoefficients() {
        attack_coeff_ = expf(-1.0f / (0.001f * attack_ms_ * sample_rate_));
        release_coeff_ = expf(-1.0f / (0.25f * sample_rate_));
    }

    float sample_rate_;
    float sustain_;
    float attack_ms_;
    float level_;
    float mix_;
    float attack_coeff_;
    float release_coeff_;
    float envelope_;
    float gain_state_;
};