#pragma once
#include "Effect.h"
#include <cmath>
#include <cstring>

// KlonCentaur: inspired by the Klon Centaur. Original DSP built around a
// clean/dirty blend, gentle pre-emphasis, high-headroom clipping, and treble trim.
class KlonCentaur : public Effect {
public:
	void Init(float sample_rate) override {
		sample_rate_ = sample_rate > 1000.0f ? sample_rate : 48000.0f;
		gain_ = 0.44f; treble_ = 0.55f; level_ = 0.95f; mix_ = 1.0f;
		pre_lp_ = pre_hp_lp_ = aa_lp_ = treble_lp_ = post_lp_ = dc_x1_ = dc_y1_ = 0.0f;
		pre_lp_coeff_ = Coeff(9000.0f); hp_coeff_ = Coeff(180.0f); smooth_coeff_ = Coeff(35.0f);
		UpdateTargets(); gain_z_ = gain_drive_; clean_z_ = clean_blend_; treble_coeff_z_ = treble_coeff_; post_coeff_z_ = post_coeff_; level_z_ = level_; mix_z_ = mix_;
	}

	float Process(float in) override {
		Smooth(gain_z_, gain_drive_); Smooth(clean_z_, clean_blend_); Smooth(treble_coeff_z_, treble_coeff_); Smooth(post_coeff_z_, post_coeff_); Smooth(level_z_, level_); Smooth(mix_z_, mix_);
		pre_lp_ += pre_lp_coeff_ * (in - pre_lp_);
		pre_hp_lp_ += hp_coeff_ * (pre_lp_ - pre_hp_lp_);
		float emphasized = (pre_lp_ - pre_hp_lp_) + pre_lp_ * 0.28f;
		float dirty = Shape(emphasized * gain_z_);
		treble_lp_ += treble_coeff_z_ * (dirty - treble_lp_);
		float dirty_voiced = treble_lp_ + (dirty - treble_lp_) * (0.45f + treble_ * 1.25f);
		float blended = in * clean_z_ + dirty_voiced * (1.0f - clean_z_ * 0.45f);
		post_lp_ += post_coeff_z_ * (blended - post_lp_);
		float dc = post_lp_ - dc_x1_ + 0.995f * dc_y1_;
		dc_x1_ = post_lp_; dc_y1_ = dc;
		float wet = dc * level_z_;
		return in * (1.0f - mix_z_) + wet * mix_z_;
	}

	const char* GetName() const override { return "KlonCentaur"; }
	EffectCategory GetCategory() const override { return EffectCategory::Distortion; }
	void SetParam(const char* name, float value) override {
		if (strcmp(name, "gain") == 0) { gain_ = Clamp(value, 0.0f, 1.0f); UpdateTargets(); }
		else if (strcmp(name, "treble") == 0) { treble_ = Clamp(value, 0.0f, 1.0f); UpdateTargets(); }
		else if (strcmp(name, "level") == 0) level_ = Clamp(value, 0.0f, 2.0f);
		else if (strcmp(name, "mix") == 0) mix_ = Clamp(value, 0.0f, 1.0f);
	}
	float GetParam(const char* name) override {
		if (strcmp(name, "gain") == 0) return gain_;
		if (strcmp(name, "treble") == 0) return treble_;
		if (strcmp(name, "level") == 0) return level_;
		if (strcmp(name, "mix") == 0) return mix_;
		return 0.0f;
	}
	const char* GetParamList() const override { return "gain,treble,level,mix"; }
	int GetParamCount() const override { return 4; }
	bool GetParamInfo(int index, EffectParamInfo& info) const override {
		switch (index) {
			case 0: info = {"gain", "Klon Centaur Gain", "", "float", "linear", 0.0f, 1.0f, 0.44f, 0.01f}; return true;
			case 1: info = {"treble", "Klon Centaur Treble", "", "float", "linear", 0.0f, 1.0f, 0.55f, 0.01f}; return true;
			case 2: info = {"level", "Klon Centaur Level", "", "float", "linear", 0.0f, 2.0f, 0.95f, 0.01f}; return true;
			case 3: info = {"mix", "Klon Centaur Mix", "", "float", "linear", 0.0f, 1.0f, 1.0f, 0.01f}; return true;
			default: return false;
		}
	}

private:
	float Coeff(float hz) const { return Clamp(1.0f - expf(-6.28318531f * hz / sample_rate_), 0.0001f, 0.98f); }
	void Smooth(float& current, float target) { current += smooth_coeff_ * (target - current); }
	float Shape(float x) { float y = tanhf(x * 0.62f) * (1.0f + 0.08f * x); aa_lp_ += Coeff(8500.0f) * (y - aa_lp_); return aa_lp_; }
	void UpdateTargets() { gain_drive_ = 1.1f + gain_ * gain_ * 9.5f; clean_blend_ = Clamp(0.58f - gain_ * 0.32f, 0.18f, 0.58f); treble_coeff_ = Coeff(900.0f + treble_ * 6400.0f); post_coeff_ = Coeff(5200.0f + treble_ * 5200.0f); }
	float sample_rate_ = 48000.0f, gain_ = 0.44f, treble_ = 0.55f, level_ = 0.95f, mix_ = 1.0f;
	float pre_lp_ = 0.0f, pre_hp_lp_ = 0.0f, aa_lp_ = 0.0f, treble_lp_ = 0.0f, post_lp_ = 0.0f, dc_x1_ = 0.0f, dc_y1_ = 0.0f;
	float pre_lp_coeff_ = 0.1f, hp_coeff_ = 0.1f, smooth_coeff_ = 0.01f, gain_drive_ = 3.0f, clean_blend_ = 0.45f, treble_coeff_ = 0.1f, post_coeff_ = 0.1f;
	float gain_z_ = 3.0f, clean_z_ = 0.45f, treble_coeff_z_ = 0.1f, post_coeff_z_ = 0.1f, level_z_ = 0.95f, mix_z_ = 1.0f;
};