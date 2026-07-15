#pragma once
#include "daisy_seed.h"
#include "Effect.h"
#include <cmath>
#include <cstdint>
#include <cstring>

// DeluxeMemoryMan: inspired by the EHX Deluxe Memory Man. Original dark analog
// echo voicing with interpolated reads, filtered repeats, modulation, and feedback saturation.
class DeluxeMemoryMan : public Effect {
public:
	static constexpr uint32_t MAX_DELAY_SAMPLES = 96000 * 2;
	void Init(float sample_rate) override {
		sample_rate_ = sample_rate > 1000.0f ? sample_rate : 48000.0f;
		time_ = 0.42f; feedback_ = 0.48f; modulation_ = 0.28f; tone_ = 0.42f; mix_ = 0.38f; write_ = 0; phase_ = 0.0f; fb_lp_ = fb_hp_lp_ = 0.0f;
		smooth_coeff_ = Coeff(18.0f); UpdateTargets(); time_z_ = delay_samples_; feedback_z_ = feedback_; mod_z_ = modulation_; tone_z_ = tone_; mix_z_ = mix_;
		for (uint32_t i = 0; i < MAX_DELAY_SAMPLES; i++) buffer_[i] = 0.0f;
	}
	float Process(float in) override {
		Smooth(time_z_, delay_samples_); Smooth(feedback_z_, feedback_); Smooth(mod_z_, modulation_); Smooth(tone_z_, tone_); Smooth(mix_z_, mix_);
		float lfo = sinf(phase_ * 6.28318531f);
		float wet = ReadDelay(time_z_ + lfo * mod_z_ * 0.006f * sample_rate_);
		float lp_coeff = Coeff(1200.0f + tone_z_ * 5200.0f);
		float hp_coeff = Coeff(55.0f + tone_z_ * 110.0f);
		fb_lp_ += lp_coeff * (wet - fb_lp_);
		fb_hp_lp_ += hp_coeff * (fb_lp_ - fb_hp_lp_);
		float feedback_sample = tanhf((fb_lp_ - fb_hp_lp_) * 1.35f) * Clamp(feedback_z_, 0.0f, 0.88f);
		buffer_[write_] = FlushTiny(in + feedback_sample);
		write_ = (write_ + 1) % MAX_DELAY_SAMPLES;
		phase_ += (0.18f + mod_z_ * 1.8f) / sample_rate_; if (phase_ >= 1.0f) phase_ -= 1.0f;
		return in * (1.0f - mix_z_) + wet * mix_z_;
	}
	const char* GetName() const override { return "DeluxeMemoryMan"; }
	EffectCategory GetCategory() const override { return EffectCategory::Time; }
	void SetParam(const char* name, float value) override { if (strcmp(name, "time") == 0) { time_ = Clamp(value, 0.04f, 1.5f); UpdateTargets(); } else if (strcmp(name, "feedback") == 0) feedback_ = Clamp(value, 0.0f, 0.88f); else if (strcmp(name, "modulation") == 0) modulation_ = Clamp(value, 0.0f, 1.0f); else if (strcmp(name, "tone") == 0) tone_ = Clamp(value, 0.0f, 1.0f); else if (strcmp(name, "mix") == 0) mix_ = Clamp(value, 0.0f, 1.0f); }
	float GetParam(const char* name) override { if (strcmp(name, "time") == 0) return time_; if (strcmp(name, "feedback") == 0) return feedback_; if (strcmp(name, "modulation") == 0) return modulation_; if (strcmp(name, "tone") == 0) return tone_; if (strcmp(name, "mix") == 0) return mix_; return 0.0f; }
	const char* GetParamList() const override { return "time,feedback,modulation,tone,mix"; }
	int GetParamCount() const override { return 5; }
	bool GetParamInfo(int index, EffectParamInfo& info) const override { switch (index) { case 0: info = {"time", "Deluxe Memory Man Time", "s", "float", "linear", 0.04f, 1.5f, 0.42f, 0.001f}; return true; case 1: info = {"feedback", "Deluxe Memory Man Feedback", "", "float", "linear", 0.0f, 0.88f, 0.48f, 0.01f}; return true; case 2: info = {"modulation", "Deluxe Memory Man Mod", "", "float", "linear", 0.0f, 1.0f, 0.28f, 0.01f}; return true; case 3: info = {"tone", "Deluxe Memory Man Tone", "", "float", "linear", 0.0f, 1.0f, 0.42f, 0.01f}; return true; case 4: info = {"mix", "Deluxe Memory Man Mix", "", "float", "linear", 0.0f, 1.0f, 0.38f, 0.01f}; return true; default: return false; } }
private:
	float Coeff(float hz) const { return Clamp(1.0f - expf(-6.28318531f * hz / sample_rate_), 0.0001f, 0.98f); }
	void Smooth(float& current, float target) { current += smooth_coeff_ * (target - current); }
	void UpdateTargets() { delay_samples_ = Clamp(time_ * sample_rate_, 1.0f, static_cast<float>(MAX_DELAY_SAMPLES - 3)); }
	float FlushTiny(float value) const { return fabsf(value) < 1.0e-18f ? 0.0f : value; }
	float ReadDelay(float delay) const { delay = Clamp(delay, 1.0f, static_cast<float>(MAX_DELAY_SAMPLES - 3)); float pos = static_cast<float>(write_) - delay; while (pos < 0.0f) pos += MAX_DELAY_SAMPLES; uint32_t i0 = static_cast<uint32_t>(pos) % MAX_DELAY_SAMPLES; uint32_t i1 = (i0 + 1) % MAX_DELAY_SAMPLES; float frac = pos - static_cast<float>(i0); return buffer_[i0] + frac * (buffer_[i1] - buffer_[i0]); }
	float sample_rate_ = 48000.0f, time_ = 0.42f, feedback_ = 0.48f, modulation_ = 0.28f, tone_ = 0.42f, mix_ = 0.38f;
	float delay_samples_ = 20160.0f, time_z_ = 20160.0f, feedback_z_ = 0.48f, mod_z_ = 0.28f, tone_z_ = 0.42f, mix_z_ = 0.38f, smooth_coeff_ = 0.01f, phase_ = 0.0f, fb_lp_ = 0.0f, fb_hp_lp_ = 0.0f;
	uint32_t write_ = 0;
	static inline float DSY_SDRAM_BSS buffer_[MAX_DELAY_SAMPLES];
};