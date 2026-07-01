#pragma once
#include "../../Effect.h"
#include <cmath>
#include <cstring>

// Amplitude modulation tremolo using a sine LFO.
class Tremolo : public Effect {
public:
    void Init(float sample_rate) override {
        sample_rate_ = sample_rate;
        rate_  = 5.0f;   // LFO speed in Hz
        depth_ = 0.8f;   // 0.0 = no effect, 1.0 = full volume swing
        phase_ = 0.0f;
    }

    float Process(float in) override {
        // LFO swings between (1 - depth) and 1.0
        float lfo = sinf(phase_ * 2.0f * 3.14159265f);
        float gain = 1.0f - depth_ * 0.5f * (1.0f - lfo);

        phase_ += rate_ / sample_rate_;
        if (phase_ >= 1.0f) phase_ -= 1.0f;

        return in * gain;
    }    const char* GetName() const override { return "tremolo"; }
    EffectCategory GetCategory() const override { return EffectCategory::Modulation; }

    void SetParam(const char* name, float value) override {
        if      (strcmp(name, "rate") == 0)  rate_ = value;
        else if (strcmp(name, "depth") == 0) depth_ = value;
    }    float GetParam(const char* name) override {
        if      (strcmp(name, "rate") == 0)  return rate_;
        else if (strcmp(name, "depth") == 0) return depth_;
        return 0.f;
    }

    const char* GetParamList() const override { return "rate,depth"; }

    void SetRate(float hz)   { rate_ = hz; }
    void SetDepth(float d)   { depth_ = d; }

private:
    float sample_rate_;
    float rate_, depth_;
    float phase_;
};
