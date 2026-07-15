#pragma once
#include "Effect.h"
#include <cmath>
#include <cstring>

// boss_ce2: inspired by the Boss CE-2 Chorus. Original warm BBD-style chorus
// using an interpolated modulated delay, smoothed controls, and wet-path rolloff.
class BossCE2 : public Effect {
public:
	static constexpr int MAX_BUF = 4096;

	void Init(float sample_rate) override {
		sample_rate_ = sample_rate > 1000.0f ? sample_rate : 48000.0f;
		rate_ = 0.82f; depth_ = 0.58f; mix_ = 0.46f;
		phase_ = 0.0f; write_ = 0; wet_lp_ = 0.0f; smooth_coeff_ = Coeff(25.0f);
		for (int i = 0; i < MAX_BUF; i++) buffer_[i] = 0.0f;
		UpdateTargets(); rate_z_ = rate_hz_; depth_z_ = depth_samples_; mix_z_ = mix_;
	}

	float Process(float in) override {
		Smooth(rate_z_, rate_hz_); Smooth(depth_z_, depth_samples_); Smooth(mix_z_, mix_);
		buffer_[write_] = in;
		float lfo = 0.5f + 0.5f * sinf(phase_ * 6.28318531f);
		float delay = base_delay_samples_ + depth_z_ * lfo;
		float wet = ReadDelay(delay);
		wet_lp_ += Coeff(5400.0f) * (wet - wet_lp_);
		write_ = (write_ + 1) % MAX_BUF;
		phase_ += rate_z_ / sample_rate_;
		if (phase_ >= 1.0f) phase_ -= 1.0f;
		return in * (1.0f - mix_z_) + wet_lp_ * mix_z_;
	}

	const char* GetName() const override { return "boss_ce2"; }
	EffectCategory GetCategory() const override { return EffectCategory::Modulation; }
	void SetParam(const char* name, float value) override { if (strcmp(name, "rate") == 0) { rate_ = Clamp(value, 0.05f, 5.0f); UpdateTargets(); } else if (strcmp(name, "depth") == 0) { depth_ = Clamp(value, 0.0f, 1.0f); UpdateTargets(); } else if (strcmp(name, "mix") == 0) mix_ = Clamp(value, 0.0f, 1.0f); }
	float GetParam(const char* name) override { if (strcmp(name, "rate") == 0) return rate_; if (strcmp(name, "depth") == 0) return depth_; if (strcmp(name, "mix") == 0) return mix_; return 0.0f; }
	const char* GetParamList() const override { return "rate,depth,mix"; }
	int GetParamCount() const override { return 3; }
	bool GetParamInfo(int index, EffectParamInfo& info) const override { switch (index) { case 0: info = {"rate", "Boss CE-2 Rate", "Hz", "float", "linear", 0.05f, 5.0f, 0.82f, 0.01f}; return true; case 1: info = {"depth", "Boss CE-2 Depth", "", "float", "linear", 0.0f, 1.0f, 0.58f, 0.01f}; return true; case 2: info = {"mix", "Boss CE-2 Mix", "", "float", "linear", 0.0f, 1.0f, 0.46f, 0.01f}; return true; default: return false; } }

private:
	float Coeff(float hz) const { return Clamp(1.0f - expf(-6.28318531f * hz / sample_rate_), 0.0001f, 0.98f); }
	void Smooth(float& current, float target) { current += smooth_coeff_ * (target - current); }
	void UpdateTargets() { rate_hz_ = rate_; base_delay_samples_ = 0.016f * sample_rate_; depth_samples_ = (0.002f + depth_ * 0.0065f) * sample_rate_; }
	float ReadDelay(float delay) const { float pos = static_cast<float>(write_) - delay; while (pos < 0.0f) pos += MAX_BUF; int i0 = static_cast<int>(pos) % MAX_BUF; int i1 = (i0 + 1) % MAX_BUF; float frac = pos - static_cast<int>(pos); return buffer_[i0] + frac * (buffer_[i1] - buffer_[i0]); }
	float sample_rate_ = 48000.0f, rate_ = 0.82f, depth_ = 0.58f, mix_ = 0.46f, phase_ = 0.0f, wet_lp_ = 0.0f;
	float rate_hz_ = 0.82f, depth_samples_ = 320.0f, base_delay_samples_ = 768.0f, smooth_coeff_ = 0.01f, rate_z_ = 0.82f, depth_z_ = 320.0f, mix_z_ = 0.46f;
	int write_ = 0;
	float buffer_[MAX_BUF];
};