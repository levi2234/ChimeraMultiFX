#pragma once
#include "Effect.h"
#include <cmath>
#include <cstring>

// BigMuffPi: inspired by the EHX Big Muff Pi. Original cascaded clipping fuzz
// with compressed sustain and a passive-style scooped tone blend.
class BigMuffPi : public Effect {
public:
	void Init(float sample_rate) override {
		sample_rate_ = sample_rate > 1000.0f ? sample_rate : 48000.0f;
		sustain_ = 0.62f; tone_ = 0.48f; level_ = 0.72f; mix_ = 1.0f;
		pre_lp_ = hp_lp_ = stage1_lp_ = stage2_lp_ = low_tone_ = high_tone_ = dc_x1_ = dc_y1_ = env_ = 0.0f;
		pre_coeff_ = Coeff(9000.0f); hp_coeff_ = Coeff(80.0f); smooth_coeff_ = Coeff(28.0f); env_coeff_ = Coeff(18.0f);
		UpdateTargets(); gain_z_ = gain_; tone_z_ = tone_; trim_z_ = trim_; level_z_ = level_; mix_z_ = mix_;
	}
	float Process(float in) override {
		Smooth(gain_z_, gain_); Smooth(tone_z_, tone_); Smooth(trim_z_, trim_); Smooth(level_z_, level_); Smooth(mix_z_, mix_);
		env_ += env_coeff_ * (fabsf(in) - env_);
		float sustain_gain = gain_z_ * (1.25f - Clamp(env_ * 4.0f, 0.0f, 0.5f));
		pre_lp_ += pre_coeff_ * (in - pre_lp_); hp_lp_ += hp_coeff_ * (pre_lp_ - hp_lp_);
		float stage1 = tanhf((pre_lp_ - hp_lp_) * sustain_gain);
		stage1_lp_ += Coeff(8200.0f) * (stage1 - stage1_lp_);
		float stage2 = tanhf((stage1_lp_ + 0.12f * stage1_lp_ * stage1_lp_) * (1.7f + sustain_ * 4.0f));
		stage2_lp_ += Coeff(6800.0f) * (stage2 * trim_z_ - stage2_lp_);
		low_tone_ += Coeff(520.0f + tone_z_ * 280.0f) * (stage2_lp_ - low_tone_);
		high_tone_ += Coeff(2400.0f + tone_z_ * 5200.0f) * (stage2_lp_ - high_tone_);
		float high = stage2_lp_ - high_tone_;
		float scooped = low_tone_ * (1.0f - tone_z_) + high * (0.35f + tone_z_ * 1.55f);
		float dc = scooped - dc_x1_ + 0.995f * dc_y1_; dc_x1_ = scooped; dc_y1_ = dc;
		float wet = dc * level_z_;
		return in * (1.0f - mix_z_) + wet * mix_z_;
	}
	const char* GetName() const override { return "BigMuffPi"; }
	EffectCategory GetCategory() const override { return EffectCategory::Distortion; }
	void SetParam(const char* name, float value) override { if (strcmp(name, "sustain") == 0) { sustain_ = Clamp(value, 0.0f, 1.0f); UpdateTargets(); } else if (strcmp(name, "tone") == 0) tone_ = Clamp(value, 0.0f, 1.0f); else if (strcmp(name, "level") == 0) level_ = Clamp(value, 0.0f, 2.0f); else if (strcmp(name, "mix") == 0) mix_ = Clamp(value, 0.0f, 1.0f); }
	float GetParam(const char* name) override { if (strcmp(name, "sustain") == 0) return sustain_; if (strcmp(name, "tone") == 0) return tone_; if (strcmp(name, "level") == 0) return level_; if (strcmp(name, "mix") == 0) return mix_; return 0.0f; }
	const char* GetParamList() const override { return "sustain,tone,level,mix"; }
	int GetParamCount() const override { return 4; }
	bool GetParamInfo(int index, EffectParamInfo& info) const override { switch (index) { case 0: info = {"sustain", "Big Muff Pi Sustain", "", "float", "linear", 0.0f, 1.0f, 0.62f, 0.01f}; return true; case 1: info = {"tone", "Big Muff Pi Tone", "", "float", "linear", 0.0f, 1.0f, 0.48f, 0.01f}; return true; case 2: info = {"level", "Big Muff Pi Level", "", "float", "linear", 0.0f, 2.0f, 0.72f, 0.01f}; return true; case 3: info = {"mix", "Big Muff Pi Mix", "", "float", "linear", 0.0f, 1.0f, 1.0f, 0.01f}; return true; default: return false; } }
private:
	float Coeff(float hz) const { return Clamp(1.0f - expf(-6.28318531f * hz / sample_rate_), 0.0001f, 0.98f); }
	void Smooth(float& current, float target) { current += smooth_coeff_ * (target - current); }
	void UpdateTargets() { gain_ = 4.0f + sustain_ * sustain_ * 24.0f; trim_ = 0.72f / (1.0f + sustain_ * 1.35f); }
	float sample_rate_ = 48000.0f, sustain_ = 0.62f, tone_ = 0.48f, level_ = 0.72f, mix_ = 1.0f;
	float pre_lp_ = 0.0f, hp_lp_ = 0.0f, stage1_lp_ = 0.0f, stage2_lp_ = 0.0f, low_tone_ = 0.0f, high_tone_ = 0.0f, dc_x1_ = 0.0f, dc_y1_ = 0.0f, env_ = 0.0f;
	float pre_coeff_ = 0.1f, hp_coeff_ = 0.1f, smooth_coeff_ = 0.01f, env_coeff_ = 0.01f, gain_ = 10.0f, trim_ = 0.4f;
	float gain_z_ = 10.0f, tone_z_ = 0.48f, trim_z_ = 0.4f, level_z_ = 0.72f, mix_z_ = 1.0f;
};