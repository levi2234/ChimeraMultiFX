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
        if      (strcmp(name, "drive") == 0) drive_ = value;
        else if (strcmp(name, "mix") == 0)   mix_ = value;
        else if (strcmp(name, "level") == 0) level_ = value;
    }    float GetParam(const char* name) override {
        if      (strcmp(name, "drive") == 0) return drive_;
        else if (strcmp(name, "mix") == 0)   return mix_;
        else if (strcmp(name, "level") == 0) return level_;
        return 0.f;
    }

    const char* GetParamList() const override { return "drive,mix,level"; }

    void SetDrive(float d) { drive_ = d; }
    void SetMix(float m)   { mix_ = m; }
    void SetLevel(float l) { level_ = l; }

private:
    float drive_;
    float mix_;
    float level_;
};
