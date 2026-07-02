#pragma once
#include "../../Effect.h"
#include <cmath>
#include <cstring>

// Soft-clipping distortion using tanh waveshaping.
class TanhDistortion : public Effect {
public:
    void Init(float sample_rate) override {
        (void)sample_rate;
        drive_ = 8.0f;
        mix_   = 1.0f;
        level_ = 0.7f;
    }

    float Process(float in) override {
        float wet = tanhf(in * drive_) * level_;
        return (in * (1.0f - mix_)) + (wet * mix_);
    }    const char* GetName() const override { return "distortion"; }
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

    void SetDrive(float d) { drive_ = Clamp(d, 0.0f, 40.0f); }
    void SetMix(float m)   { mix_ = Clamp(m, 0.0f, 1.0f); }
    void SetLevel(float l) { level_ = Clamp(l, 0.0f, 2.0f); }

private:
    static float Clamp(float value, float min, float max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }

    float drive_;
    float mix_;
    float level_;
};
