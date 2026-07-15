#pragma once
#include "Effect.h"
#include <cmath>
#include <cstring>

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
    }    const char* GetName() const override { return "compressor"; }
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
        else if (strcmp(name, "release") == 0)   return release_;        else if (strcmp(name, "makeup") == 0)    return makeup_;
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
    void SetRatio(float r)      { ratio_ = Clamp(r, 1.0f, 20.0f); }
    void SetAttack(float sec)   { attack_ = Clamp(sec, 0.0001f, 2.0f); }
    void SetRelease(float sec)  { release_ = Clamp(sec, 0.0001f, 5.0f); }
    void SetMakeup(float g)     { makeup_ = Clamp(g, 0.0f, 8.0f); }

private:
    float sample_rate_;
    float threshold_, ratio_;
    float attack_, release_;
    float makeup_;
    float env_;
};
