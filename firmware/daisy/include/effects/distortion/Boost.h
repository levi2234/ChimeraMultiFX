#pragma once
#include "Effect.h"
#include <cmath>
#include <cstring>

// Clean guitar boost with a broad tone control and guarded output stage.
// A one-pole low-pass splits the signal into low/high bands so tone can tilt
// without a costly EQ; tanh only catches excessive peaks and stays nearly
// linear at normal levels, preventing hard digital clipping after the boost.
class Boost : public Effect {
public:
    void Init(float sample_rate) override {
        sample_rate_ = sample_rate > 1000.0f ? sample_rate : 48000.0f;
        gain_ = 6.0f;
        gain_linear_ = powf(10.0f, gain_ / 20.0f);
        tone_ = 0.5f;
        level_ = 1.0f;
        low_state_ = 0.0f;
        tone_coeff_ = 1.0f - expf(-2.0f * 3.14159265f * 1200.0f / sample_rate_);
    }

    float Process(float in) override {
        low_state_ += tone_coeff_ * (in - low_state_);
        const float high = in - low_state_;
        const float tilt = (tone_ - 0.5f) * 1.5f;
        const float voiced = low_state_ * (1.0f - tilt) + high * (1.0f + tilt);
        const float boosted = voiced * gain_linear_ * level_;

        // The Hermite knee is exactly linear below 0.75, reaches full scale
        // with zero slope, then remains bounded at the converter rails.
        return SoftLimit(boosted);
    }

    const char* GetName() const override { return "boost"; }
    EffectCategory GetCategory() const override { return EffectCategory::Distortion; }

    void SetParam(const char* name, float value) override {
        if (strcmp(name, "gain") == 0) {
            gain_ = Clamp(value, 0.0f, 24.0f);
            gain_linear_ = powf(10.0f, gain_ / 20.0f);
        }
        else if (strcmp(name, "tone") == 0)  tone_ = Clamp(value, 0.0f, 1.0f);
        else if (strcmp(name, "level") == 0) level_ = Clamp(value, 0.0f, 2.0f);
    }

    float GetParam(const char* name) override {
        if      (strcmp(name, "gain") == 0)  return gain_;
        else if (strcmp(name, "tone") == 0)  return tone_;
        else if (strcmp(name, "level") == 0) return level_;
        return 0.0f;
    }

    const char* GetParamList() const override { return "gain,tone,level"; }
    int GetParamCount() const override { return 3; }

    bool GetParamInfo(int index, EffectParamInfo& info) const override {
        switch (index) {
            case 0: info = {"gain", "Gain", "dB", "float", "linear", 0.0f, 24.0f, 6.0f, 0.1f}; return true;
            case 1: info = {"tone", "Tone", "", "float", "linear", 0.0f, 1.0f, 0.5f, 0.01f}; return true;
            case 2: info = {"level", "Level", "", "float", "linear", 0.0f, 2.0f, 1.0f, 0.01f}; return true;
            default: return false;
        }
    }

private:
    float SoftLimit(float value) const {
        const float magnitude = fabsf(value);
        if (magnitude <= 0.75f) return value;
        const float sign = value < 0.0f ? -1.0f : 1.0f;
        if (magnitude >= 1.0f) return sign;

        const float position = (magnitude - 0.75f) * 4.0f;
        const float position_squared = position * position;
        const float position_cubed = position_squared * position;
        const float h00 = 2.0f * position_cubed - 3.0f * position_squared + 1.0f;
        const float h10 = position_cubed - 2.0f * position_squared + position;
        const float h01 = -2.0f * position_cubed + 3.0f * position_squared;
        return sign * (0.75f * h00 + 0.25f * h10 + h01);
    }

    float sample_rate_;
    float gain_;
    float gain_linear_;
    float tone_;
    float level_;
    float low_state_;
    float tone_coeff_;
};