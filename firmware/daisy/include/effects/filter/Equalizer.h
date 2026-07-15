#pragma once
#include "Effect.h"
#include <cmath>
#include <cstring>

// Compact three-band guitar EQ with fixed 250 Hz and 2.5 kHz crossovers.
// Two matched one-poles form complementary low, mid, and high bands whose sum
// is the input at flat settings. Parallel band gains keep the control response
// intuitive and avoid the phase buildup of three cascaded shelving filters.
class Equalizer : public Effect {
public:
    void Init(float sample_rate) override {
        sample_rate_ = sample_rate > 1000.0f ? sample_rate : 48000.0f;
        low_gain_db_ = mid_gain_db_ = high_gain_db_ = 0.0f;
        low_state_ = high_split_state_ = 0.0f;
        low_coeff_ = OnePoleCoefficient(250.0f);
        high_coeff_ = OnePoleCoefficient(2500.0f);
        UpdateGains();
    }

    float Process(float in) override {
        low_state_ += low_coeff_ * (in - low_state_);
        high_split_state_ += high_coeff_ * (in - high_split_state_);
        const float low = low_state_;
        const float mid = high_split_state_ - low_state_;
        const float high = in - high_split_state_;
        return low * low_gain_ + mid * mid_gain_ + high * high_gain_;
    }

    const char* GetName() const override { return "eq"; }
    EffectCategory GetCategory() const override { return EffectCategory::Filter; }

    void SetParam(const char* name, float value) override {
        if      (strcmp(name, "low") == 0) low_gain_db_ = Clamp(value, -12.0f, 12.0f);
        else if (strcmp(name, "mid") == 0) mid_gain_db_ = Clamp(value, -12.0f, 12.0f);
        else if (strcmp(name, "high") == 0) high_gain_db_ = Clamp(value, -12.0f, 12.0f);
        else return;
        UpdateGains();
    }

    float GetParam(const char* name) override {
        if      (strcmp(name, "low") == 0) return low_gain_db_;
        else if (strcmp(name, "mid") == 0) return mid_gain_db_;
        else if (strcmp(name, "high") == 0) return high_gain_db_;
        return 0.0f;
    }

    const char* GetParamList() const override { return "low,mid,high"; }
    int GetParamCount() const override { return 3; }
    bool GetParamInfo(int index, EffectParamInfo& info) const override {
        switch (index) {
            case 0: info = {"low", "Low", "dB", "float", "linear", -12.0f, 12.0f, 0.0f, 0.5f}; return true;
            case 1: info = {"mid", "Mid", "dB", "float", "linear", -12.0f, 12.0f, 0.0f, 0.5f}; return true;
            case 2: info = {"high", "High", "dB", "float", "linear", -12.0f, 12.0f, 0.0f, 0.5f}; return true;
            default: return false;
        }
    }

private:
    float OnePoleCoefficient(float cutoff) const {
        return 1.0f - expf(-2.0f * 3.14159265f * cutoff / sample_rate_);
    }

    void UpdateGains() {
        low_gain_ = powf(10.0f, low_gain_db_ / 20.0f);
        mid_gain_ = powf(10.0f, mid_gain_db_ / 20.0f);
        high_gain_ = powf(10.0f, high_gain_db_ / 20.0f);
    }

    float sample_rate_;
    float low_gain_db_, mid_gain_db_, high_gain_db_;
    float low_gain_, mid_gain_, high_gain_;
    float low_coeff_, high_coeff_;
    float low_state_, high_split_state_;
};