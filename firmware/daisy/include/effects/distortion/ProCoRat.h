#pragma once
#include "Effect.h"
#include <cmath>
#include <cstring>

// ProcoRAT: inspired by the ProCo RAT. Original high-gain op-amp-style drive
// with a hard/soft clipping blend and the famous post-distortion filter behavior.
class ProCoRat : public Effect {
public:
	void Init(float sample_rate) override {
		sample_rate_ = sample_rate > 1000.0f ? sample_rate : 48000.0f;
		distortion_ = 0.56f; filter_ = 0.45f; level_ = 0.72f; mix_ = 1.0f;
		input_lp_ = hp_lp_ = aa_lp_ = filter_lp_ = dc_x1_ = dc_y1_ = 0.0f;
		input_coeff_ = Coeff(9800.0f); hp_coeff_ = Coeff(95.0f); smooth_coeff_ = Coeff(34.0f);
		UpdateTargets(); gain_z_ = gain_; clip_z_ = clip_blend_; filter_coeff_z_ = filter_coeff_; trim_z_ = trim_; level_z_ = level_; mix_z_ = mix_;
	}
	float Process(float in) override {
		Smooth(gain_z_, gain_); Smooth(clip_z_, clip_blend_); Smooth(filter_coeff_z_, filter_coeff_); Smooth(trim_z_, trim_); Smooth(level_z_, level_); Smooth(mix_z_, mix_);
		input_lp_ += input_coeff_ * (in - input_lp_); hp_lp_ += hp_coeff_ * (input_lp_ - hp_lp_);
		float pre = (input_lp_ - hp_lp_) * gain_z_;
		float soft = tanhf(pre * 0.72f);
		float hard = Clamp(pre, -0.74f, 0.74f) / 0.74f;
		float clipped = soft * (1.0f - clip_z_) + hard * clip_z_;
		aa_lp_ += Coeff(7600.0f) * (clipped * trim_z_ - aa_lp_);
		filter_lp_ += filter_coeff_z_ * (aa_lp_ - filter_lp_);
		float dc = filter_lp_ - dc_x1_ + 0.995f * dc_y1_; dc_x1_ = filter_lp_; dc_y1_ = dc;
		float wet = dc * level_z_;
		return in * (1.0f - mix_z_) + wet * mix_z_;
	}
	const char* GetName() const override { return "ProcoRAT"; }
	EffectCategory GetCategory() const override { return EffectCategory::Distortion; }
	void SetParam(const char* name, float value) override { if (strcmp(name, "distortion") == 0) { distortion_ = Clamp(value, 0.0f, 1.0f); UpdateTargets(); } else if (strcmp(name, "filter") == 0) { filter_ = Clamp(value, 0.0f, 1.0f); UpdateTargets(); } else if (strcmp(name, "level") == 0) level_ = Clamp(value, 0.0f, 2.0f); else if (strcmp(name, "mix") == 0) mix_ = Clamp(value, 0.0f, 1.0f); }
	float GetParam(const char* name) override { if (strcmp(name, "distortion") == 0) return distortion_; if (strcmp(name, "filter") == 0) return filter_; if (strcmp(name, "level") == 0) return level_; if (strcmp(name, "mix") == 0) return mix_; return 0.0f; }
	const char* GetParamList() const override { return "distortion,filter,level,mix"; }
	int GetParamCount() const override { return 4; }
	bool GetParamInfo(int index, EffectParamInfo& info) const override { switch (index) { case 0: info = {"distortion", "ProCo RAT Distortion", "", "float", "linear", 0.0f, 1.0f, 0.56f, 0.01f}; return true; case 1: info = {"filter", "ProCo RAT Filter", "", "float", "linear", 0.0f, 1.0f, 0.45f, 0.01f}; return true; case 2: info = {"level", "ProCo RAT Level", "", "float", "linear", 0.0f, 2.0f, 0.72f, 0.01f}; return true; case 3: info = {"mix", "ProCo RAT Mix", "", "float", "linear", 0.0f, 1.0f, 1.0f, 0.01f}; return true; default: return false; } }
private:
	float Coeff(float hz) const { return Clamp(1.0f - expf(-6.28318531f * hz / sample_rate_), 0.0001f, 0.98f); }
	void Smooth(float& current, float target) { current += smooth_coeff_ * (target - current); }
	void UpdateTargets() { gain_ = 3.0f + distortion_ * distortion_ * 34.0f; clip_blend_ = 0.25f + distortion_ * 0.68f; trim_ = 0.78f / (1.0f + distortion_ * 1.9f); filter_coeff_ = Coeff(900.0f + (1.0f - filter_) * 6200.0f); }
	float sample_rate_ = 48000.0f, distortion_ = 0.56f, filter_ = 0.45f, level_ = 0.72f, mix_ = 1.0f;
	float input_lp_ = 0.0f, hp_lp_ = 0.0f, aa_lp_ = 0.0f, filter_lp_ = 0.0f, dc_x1_ = 0.0f, dc_y1_ = 0.0f;
	float input_coeff_ = 0.1f, hp_coeff_ = 0.1f, smooth_coeff_ = 0.01f, gain_ = 8.0f, clip_blend_ = 0.5f, filter_coeff_ = 0.1f, trim_ = 0.5f;
	float gain_z_ = 8.0f, clip_z_ = 0.5f, filter_coeff_z_ = 0.1f, trim_z_ = 0.5f, level_z_ = 0.72f, mix_z_ = 1.0f;
};