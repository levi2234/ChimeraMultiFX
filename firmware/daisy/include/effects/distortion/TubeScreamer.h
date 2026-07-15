#pragma once
#include "Effect.h"
#include <cmath>
#include <cstring>

// TubeScreamer: inspired by the Ibanez Tube Screamer TS9/808. This is an
// original embedded-friendly mid-hump overdrive voicing, not a circuit clone.
class TubeScreamer : public Effect {
public:
	void Init(float sample_rate) override {
		sample_rate_ = sample_rate > 1000.0f ? sample_rate : 48000.0f;
		drive_ = 0.48f;
		tone_ = 0.54f;
		level_ = 0.86f;
		mix_ = 1.0f;
		input_lp_ = bass_lp_ = mid_lp_ = aa_lp_ = tone_lp_ = output_lp_ = 0.0f;
		prev_pre_ = dc_x1_ = dc_y1_ = 0.0f;
		input_lp_coeff_ = OnePoleCoeff(10000.0f, sample_rate_);
		bass_coeff_ = OnePoleCoeff(150.0f, sample_rate_);
		mid_coeff_ = OnePoleCoeff(720.0f, sample_rate_);
		smooth_coeff_ = OnePoleCoeff(38.0f, sample_rate_);
		UpdateTargets();
		drive_gain_z_ = drive_gain_;
		trim_z_ = trim_;
		mid_gain_z_ = mid_gain_;
		tone_coeff_z_ = tone_coeff_;
		output_coeff_z_ = output_coeff_;
		level_z_ = level_;
		mix_z_ = mix_;
	}

	float Process(float in) override {
		Smooth(drive_gain_z_, drive_gain_);
		Smooth(trim_z_, trim_);
		Smooth(mid_gain_z_, mid_gain_);
		Smooth(tone_coeff_z_, tone_coeff_);
		Smooth(output_coeff_z_, output_coeff_);
		Smooth(level_z_, level_);
		Smooth(mix_z_, mix_);

		input_lp_ += input_lp_coeff_ * (in - input_lp_);
		bass_lp_ += bass_coeff_ * (input_lp_ - bass_lp_);
		float low_cut = input_lp_ - bass_lp_;
		mid_lp_ += mid_coeff_ * (low_cut - mid_lp_);
		float mid_hump = low_cut + (mid_lp_ * mid_gain_z_);

		float halfway = 0.5f * (prev_pre_ + mid_hump);
		prev_pre_ = mid_hump;
		Shape(halfway);
		float clipped = Shape(mid_hump);

		tone_lp_ += tone_coeff_z_ * (clipped - tone_lp_);
		float bright = clipped - tone_lp_;
		float toned = tone_lp_ + bright * (0.28f + 1.22f * tone_);
		output_lp_ += output_coeff_z_ * (toned - output_lp_);

		float dc_blocked = output_lp_ - dc_x1_ + 0.995f * dc_y1_;
		dc_x1_ = output_lp_;
		dc_y1_ = dc_blocked;
		float wet = dc_blocked * level_z_;
		return in * (1.0f - mix_z_) + wet * mix_z_;
	}

	const char* GetName() const override { return "TubeScreamer"; }
	EffectCategory GetCategory() const override { return EffectCategory::Distortion; }

	void SetParam(const char* name, float value) override {
		if (strcmp(name, "drive") == 0) { drive_ = Clamp(value, 0.0f, 1.0f); UpdateTargets(); }
		else if (strcmp(name, "tone") == 0) { tone_ = Clamp(value, 0.0f, 1.0f); UpdateTargets(); }
		else if (strcmp(name, "level") == 0) level_ = Clamp(value, 0.0f, 2.0f);
		else if (strcmp(name, "mix") == 0) mix_ = Clamp(value, 0.0f, 1.0f);
	}

	float GetParam(const char* name) override {
		if (strcmp(name, "drive") == 0) return drive_;
		if (strcmp(name, "tone") == 0) return tone_;
		if (strcmp(name, "level") == 0) return level_;
		if (strcmp(name, "mix") == 0) return mix_;
		return 0.0f;
	}

	const char* GetParamList() const override { return "drive,tone,level,mix"; }
	int GetParamCount() const override { return 4; }
	bool GetParamInfo(int index, EffectParamInfo& info) const override {
		switch (index) {
			case 0: info = {"drive", "TS9/808 Drive", "", "float", "linear", 0.0f, 1.0f, 0.48f, 0.01f}; return true;
			case 1: info = {"tone", "TS9/808 Tone", "", "float", "linear", 0.0f, 1.0f, 0.54f, 0.01f}; return true;
			case 2: info = {"level", "TS9/808 Level", "", "float", "linear", 0.0f, 2.0f, 0.86f, 0.01f}; return true;
			case 3: info = {"mix", "TS9/808 Mix", "", "float", "linear", 0.0f, 1.0f, 1.0f, 0.01f}; return true;
			default: return false;
		}
	}

private:
	float OnePoleCoeff(float hz, float rate) const {
		return Clamp(1.0f - expf(-6.28318531f * hz / rate), 0.0001f, 0.98f);
	}

	void Smooth(float& current, float target) {
		current += smooth_coeff_ * (target - current);
	}

	void UpdateTargets() {
		float drive_curve = drive_ * drive_;
		drive_gain_ = 1.8f + drive_curve * 14.0f;
		trim_ = 0.9f / (1.0f + drive_curve * 2.2f);
		mid_gain_ = 0.52f + drive_ * 0.85f;
		tone_coeff_ = OnePoleCoeff(720.0f + tone_ * 4300.0f, sample_rate_);
		output_coeff_ = OnePoleCoeff(4400.0f + tone_ * 4700.0f, sample_rate_);
	}

	float Shape(float sample) {
		float driven = sample * drive_gain_z_;
		float positive = tanhf(driven * 0.82f);
		float negative = tanhf(driven * 1.12f) * 0.88f;
		float shaped = driven >= 0.0f ? positive : negative;
		aa_lp_ += OnePoleCoeff(9000.0f, sample_rate_ * 2.0f) * (shaped * trim_z_ - aa_lp_);
		return aa_lp_;
	}

	float sample_rate_ = 48000.0f;
	float drive_ = 0.48f, tone_ = 0.54f, level_ = 0.86f, mix_ = 1.0f;
	float input_lp_ = 0.0f, bass_lp_ = 0.0f, mid_lp_ = 0.0f, aa_lp_ = 0.0f;
	float tone_lp_ = 0.0f, output_lp_ = 0.0f, prev_pre_ = 0.0f, dc_x1_ = 0.0f, dc_y1_ = 0.0f;
	float input_lp_coeff_ = 0.1f, bass_coeff_ = 0.1f, mid_coeff_ = 0.1f, smooth_coeff_ = 0.01f;
	float drive_gain_ = 6.0f, trim_ = 0.6f, mid_gain_ = 0.8f, tone_coeff_ = 0.1f, output_coeff_ = 0.1f;
	float drive_gain_z_ = 6.0f, trim_z_ = 0.6f, mid_gain_z_ = 0.8f, tone_coeff_z_ = 0.1f, output_coeff_z_ = 0.1f;
	float level_z_ = 0.86f, mix_z_ = 1.0f;
};