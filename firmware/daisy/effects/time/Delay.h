#pragma once
#include "daisy_seed.h"
#include "../../Effect.h"
#include <cstdint>

// Simple delay line with feedback.
class Delay : public Effect {
public:
    static constexpr uint32_t MAX_DELAY_SAMPLES = 96000 * 2; // 2s @ 96kHz

    void Init(float sample_rate) override {
        sample_rate_   = sample_rate;
        delay_time_    = 0.5f;
        feedback_      = 0.6f;
        mix_           = 0.4f;
        write_ptr_     = 0;
        delay_samples_ = (uint32_t)(delay_time_ * sample_rate_);
        if (delay_samples_ >= MAX_DELAY_SAMPLES)
            delay_samples_ = MAX_DELAY_SAMPLES - 1;

        for (uint32_t i = 0; i < MAX_DELAY_SAMPLES; i++)
            buffer_[i] = 0.0f;
    }

    float Process(float in) override {
        uint32_t read_ptr = (write_ptr_ - delay_samples_ + MAX_DELAY_SAMPLES) % MAX_DELAY_SAMPLES;
        float wet = buffer_[read_ptr];
        buffer_[write_ptr_] = in + (wet * feedback_);
        write_ptr_ = (write_ptr_ + 1) % MAX_DELAY_SAMPLES;
        return (in * (1.0f - mix_)) + (wet * mix_);
    }

    const char* GetName() const override { return "Delay"; }
    EffectCategory GetCategory() const override { return EffectCategory::Time; }

    void SetDelayTime(float sec) {
        delay_time_ = sec;
        delay_samples_ = (uint32_t)(delay_time_ * sample_rate_);
        if (delay_samples_ >= MAX_DELAY_SAMPLES)
            delay_samples_ = MAX_DELAY_SAMPLES - 1;
    }
    void SetFeedback(float fb) { feedback_ = fb; }
    void SetMix(float m)       { mix_ = m; }

private:
    float    sample_rate_   = 96000.f;
    float    delay_time_    = 0.5f;
    float    feedback_      = 0.6f;
    float    mix_           = 0.4f;
    uint32_t write_ptr_     = 0;
    uint32_t delay_samples_ = 0;

    static inline float DSY_SDRAM_BSS buffer_[MAX_DELAY_SAMPLES];
};
