#pragma once
#include "Effect.h"
#include <cmath>
#include <cstring>

// boss_bf2: inspired by the Boss BF-2 Flanger. Original short modulated delay
// with interpolated reads, manual offset, and clamped regeneration.
class BossBF2 : public Effect {
public:
	static constexpr int MAX_BUF = 2048;
	void Init(float sample_rate) override {
		sample_rate_ = sample_rate > 1000.0f ? sample_rate : 48000.0f;
		rate_ = 0.28f; depth_ = 0.64f; manual_ = 0.38f; resonance_ = 0.32f; mix_ = 0.52f; phase_ = feedback_sample_ = 0.0f; write_ = 0;
		for (int i = 0; i < MAX_BUF; i++) buffer_[i] = 0.0f;
		smooth_coeff_ = Coeff(28.0f); UpdateTargets(); rate_z_ = rate_; depth_z_ = depth_; manual_z_ = manual_; resonance_z_ = resonance_; mix_z_ = mix_;
	}
	float Process(float in) override {
		Smooth(rate_z_, rate_); Smooth(depth_z_, depth_); Smooth(manual_z_, manual_); Smooth(resonance_z_, resonance_); Smooth(mix_z_, mix_);
		float write_sample = in + feedback_sample_ * Clamp(resonance_z_, -0.86f, 0.86f);
		buffer_[write_] = tanhf(write_sample * 0.9f);
		float lfo = 0.5f + 0.5f * sinf(phase_ * 6.28318531f);
		float base = (0.0007f + manual_z_ * 0.0055f) * sample_rate_;
		float sweep = depth_z_ * lfo * 0.0045f * sample_rate_;
		float wet = ReadDelay(base + sweep + 1.0f);
		feedback_sample_ = wet;
		write_ = (write_ + 1) % MAX_BUF;
		phase_ += rate_z_ / sample_rate_; if (phase_ >= 1.0f) phase_ -= 1.0f;
		return in * (1.0f - mix_z_) + wet * mix_z_;
	}
	const char* GetName() const override { return "boss_bf2"; }
	EffectCategory GetCategory() const override { return EffectCategory::Modulation; }
	void SetParam(const char* name, float value) override { if (strcmp(name, "rate") == 0) rate_ = Clamp(value, 0.02f, 5.0f); else if (strcmp(name, "depth") == 0) depth_ = Clamp(value, 0.0f, 1.0f); else if (strcmp(name, "manual") == 0) manual_ = Clamp(value, 0.0f, 1.0f); else if (strcmp(name, "resonance") == 0) resonance_ = Clamp(value, -0.86f, 0.86f); else if (strcmp(name, "mix") == 0) mix_ = Clamp(value, 0.0f, 1.0f); }
	float GetParam(const char* name) override { if (strcmp(name, "rate") == 0) return rate_; if (strcmp(name, "depth") == 0) return depth_; if (strcmp(name, "manual") == 0) return manual_; if (strcmp(name, "resonance") == 0) return resonance_; if (strcmp(name, "mix") == 0) return mix_; return 0.0f; }
	const char* GetParamList() const override { return "rate,depth,manual,resonance,mix"; }
	int GetParamCount() const override { return 5; }
	bool GetParamInfo(int index, EffectParamInfo& info) const override { switch (index) { case 0: info = {"rate", "Boss BF-2 Rate", "Hz", "float", "linear", 0.02f, 5.0f, 0.28f, 0.01f}; return true; case 1: info = {"depth", "Boss BF-2 Depth", "", "float", "linear", 0.0f, 1.0f, 0.64f, 0.01f}; return true; case 2: info = {"manual", "Boss BF-2 Manual", "", "float", "linear", 0.0f, 1.0f, 0.38f, 0.01f}; return true; case 3: info = {"resonance", "Boss BF-2 Resonance", "", "float", "linear", -0.86f, 0.86f, 0.32f, 0.01f}; return true; case 4: info = {"mix", "Boss BF-2 Mix", "", "float", "linear", 0.0f, 1.0f, 0.52f, 0.01f}; return true; default: return false; } }
private:
	float Coeff(float hz) const { return Clamp(1.0f - expf(-6.28318531f * hz / sample_rate_), 0.0001f, 0.98f); }
	void Smooth(float& current, float target) { current += smooth_coeff_ * (target - current); }
	void UpdateTargets() {}
	float ReadDelay(float delay) const { float pos = static_cast<float>(write_) - delay; while (pos < 0.0f) pos += MAX_BUF; int i0 = static_cast<int>(pos) % MAX_BUF; int i1 = (i0 + 1) % MAX_BUF; float frac = pos - static_cast<int>(pos); return buffer_[i0] + frac * (buffer_[i1] - buffer_[i0]); }
	float sample_rate_ = 48000.0f, rate_ = 0.28f, depth_ = 0.64f, manual_ = 0.38f, resonance_ = 0.32f, mix_ = 0.52f, phase_ = 0.0f, feedback_sample_ = 0.0f;
	float smooth_coeff_ = 0.01f, rate_z_ = 0.28f, depth_z_ = 0.64f, manual_z_ = 0.38f, resonance_z_ = 0.32f, mix_z_ = 0.52f;
	int write_ = 0;
	float buffer_[MAX_BUF];
};