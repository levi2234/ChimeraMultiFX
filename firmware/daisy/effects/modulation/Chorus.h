#pragma once
#include "../../Effect.h"
#include <cmath>
#include <cstring>

// Simple chorus using a modulated delay line.
class Chorus : public Effect {
public:
    static constexpr int MAX_BUF = 4800; // 50ms @ 96kHz

    void Init(float sample_rate) override {
        sample_rate_ = sample_rate;
        rate_   = 1.5f;   // LFO Hz
        depth_  = 0.003f; // modulation depth in seconds
        mix_    = 0.5f;
        phase_  = 0.0f;
        write_  = 0;
        for (int i = 0; i < MAX_BUF; i++) buf_[i] = 0.0f;
    }

    float Process(float in) override {
        buf_[write_] = in;

        // LFO generates a modulated read offset
        float lfo = (sinf(phase_ * 2.0f * 3.14159265f) * 0.5f + 0.5f) * depth_ * sample_rate_;
        float read_f = (float)write_ - lfo - 1.0f;
        if (read_f < 0.0f) read_f += MAX_BUF;

        // Linear interpolation between two buffer samples
        int   r0 = (int)read_f % MAX_BUF;
        int   r1 = (r0 + 1) % MAX_BUF;
        float frac = read_f - (int)read_f;
        float wet = buf_[r0] + frac * (buf_[r1] - buf_[r0]);

        write_ = (write_ + 1) % MAX_BUF;
        phase_ += rate_ / sample_rate_;
        if (phase_ >= 1.0f) phase_ -= 1.0f;

        return (in * (1.0f - mix_)) + (wet * mix_);
    }    const char* GetName() const override { return "chorus"; }
    EffectCategory GetCategory() const override { return EffectCategory::Modulation; }

    void SetParam(const char* name, float value) override {
        if      (strcmp(name, "rate") == 0)  rate_ = value;
        else if (strcmp(name, "depth") == 0) depth_ = value;
        else if (strcmp(name, "mix") == 0)   mix_ = value;
    }

    float GetParam(const char* name) override {
        if      (strcmp(name, "rate") == 0)  return rate_;
        else if (strcmp(name, "depth") == 0) return depth_;
        else if (strcmp(name, "mix") == 0)   return mix_;
        return 0.f;
    }

    void SetRate(float hz)    { rate_ = hz; }
    void SetDepth(float sec)  { depth_ = sec; }
    void SetMix(float m)      { mix_ = m; }

private:
    float sample_rate_;
    float rate_, depth_, mix_;
    float phase_;
    int   write_;
    float buf_[MAX_BUF];
};
