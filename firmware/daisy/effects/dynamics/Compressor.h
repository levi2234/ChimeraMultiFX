#pragma once
#include "../../Effect.h"
#include <cmath>

// Simple feed-forward compressor (envelope follower + gain reduction).
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
    }

    float Process(float in) override {
        // Envelope follower
        float abs_in = fabsf(in);
        float coeff  = (abs_in > env_)
                        ? expf(-1.0f / (attack_  * sample_rate_))
                        : expf(-1.0f / (release_ * sample_rate_));
        env_ = coeff * env_ + (1.0f - coeff) * abs_in;

        // Gain computation
        float gain = 1.0f;
        if (env_ > threshold_) {
            float over_db = 20.0f * log10f(env_ / threshold_);
            float reduced = over_db * (1.0f - 1.0f / ratio_);
            gain = powf(10.0f, -reduced / 20.0f);
        }

        return in * gain * makeup_;
    }

    const char* GetName() const override { return "Compressor"; }
    EffectCategory GetCategory() const override { return EffectCategory::Dynamics; }

    void SetThreshold(float t)  { threshold_ = t; }
    void SetRatio(float r)      { ratio_ = (r < 1.0f) ? 1.0f : r; }
    void SetAttack(float sec)   { attack_ = sec; }
    void SetRelease(float sec)  { release_ = sec; }
    void SetMakeup(float g)     { makeup_ = g; }

private:
    float sample_rate_;
    float threshold_, ratio_;
    float attack_, release_;
    float makeup_;
    float env_;
};
