#pragma once
#include "Effect.h"
#include <cmath>
#include <cstring>

// Original embedded model voiced after the broad-band, touch-sensitive response
// of an OCD-style MOSFET overdrive. It is not a component-level circuit clone.
class FulltoneOCD : public Effect {
public:
	void Init(float sample_rate) override {
		sample_rate_ = sample_rate > 1000.0f ? sample_rate : 48000.0f;
		drive_ = 0.38f;
		tone_ = 0.52f;
		volume_ = 0.50f;
		peak_mode_ = 0.0f;
		drive_z_ = drive_;
		tone_z_ = tone_;
		volume_z_ = volume_;
		peak_z_ = peak_mode_;
		input_lp_ = bass_lp_ = presence_lp_ = tone_lp_ = aa_lp_ = 0.0f;
		previous_pre_ = dc_x1_ = dc_y1_ = 0.0f;
		smooth_coeff_ = Coeff(32.0f, sample_rate_);
		input_coeff_ = Coeff(16500.0f, sample_rate_);
		bass_coeff_ = Coeff(105.0f, sample_rate_);
	}

	float Process(float in) override {
		Smooth(drive_z_, drive_);
		Smooth(tone_z_, tone_);
		Smooth(volume_z_, volume_);
		Smooth(peak_z_, peak_mode_);

		input_lp_ += input_coeff_ * (in - input_lp_);
		bass_lp_ += bass_coeff_ * (input_lp_ - bass_lp_);
		float high_band = input_lp_ - bass_lp_;
		float mode = peak_z_;
		float bass_amount = 0.78f - 0.16f * mode;
		float gain = (1.25f + 31.0f * drive_z_ * drive_z_) * (1.0f + 0.32f * mode);
		float pre = (high_band + bass_lp_ * bass_amount) * gain;

		float midpoint = 0.5f * (previous_pre_ + pre);
		previous_pre_ = pre;
		float clipped_mid = Shape(midpoint);
		float clipped = 0.5f * (clipped_mid + Shape(pre));

		float presence_coeff = Coeff(1150.0f + 950.0f * tone_z_, sample_rate_);
		presence_lp_ += presence_coeff * (clipped - presence_lp_);
		float presence = clipped - presence_lp_;
		float mode_voiced = clipped + presence * (0.08f + 0.24f * mode);
		float tone_cut = 1750.0f * powf(7.2f, tone_z_);
		tone_lp_ += Coeff(tone_cut, sample_rate_) * (mode_voiced - tone_lp_);

		float compensation = 0.92f / (1.0f + 0.58f * drive_z_);
		float output_gain = (0.05f + 2.45f * volume_z_ * volume_z_) * compensation;
		float wet = tone_lp_ * output_gain;
		float dc_blocked = wet - dc_x1_ + 0.995f * dc_y1_;
		dc_x1_ = wet;
		dc_y1_ = FlushTiny(dc_blocked);
		return dc_y1_;
	}

	const char* GetName() const override { return "FulltoneOCD"; }
	EffectCategory GetCategory() const override { return EffectCategory::Distortion; }

	void SetParam(const char* name, float value) override {
		if (strcmp(name, "drive") == 0) drive_ = Clamp(value, 0.0f, 1.0f);
		else if (strcmp(name, "tone") == 0) tone_ = Clamp(value, 0.0f, 1.0f);
		else if (strcmp(name, "volume") == 0) volume_ = Clamp(value, 0.0f, 1.0f);
		else if (strcmp(name, "peak_mode") == 0) peak_mode_ = Clamp(value, 0.0f, 1.0f);
	}

	float GetParam(const char* name) override {
		if (strcmp(name, "drive") == 0) return drive_;
		if (strcmp(name, "tone") == 0) return tone_;
		if (strcmp(name, "volume") == 0) return volume_;
		if (strcmp(name, "peak_mode") == 0) return peak_mode_;
		return 0.0f;
	}

	const char* GetParamList() const override { return "drive,tone,volume,peak_mode"; }
	int GetParamCount() const override { return 4; }
	bool GetParamInfo(int index, EffectParamInfo& info) const override {
		switch (index) {
			case 0: info = {"drive", "OCD Drive", "", "float", "log", 0.0f, 1.0f, 0.38f, 0.01f}; return true;
			case 1: info = {"tone", "OCD Tone", "", "float", "linear", 0.0f, 1.0f, 0.52f, 0.01f}; return true;
			case 2: info = {"volume", "OCD Volume", "", "float", "log", 0.0f, 1.0f, 0.50f, 0.01f}; return true;
			case 3: info = {"peak_mode", "OCD Peak Mode", "", "switch", "linear", 0.0f, 1.0f, 0.0f, 1.0f, "LP,HP"}; return true;
			default: return false;
		}
	}

private:
	float Coeff(float hz, float rate) const {
		return Clamp(1.0f - expf(-6.28318531f * hz / rate), 0.0001f, 0.98f);
	}
	void Smooth(float& current, float target) { current += smooth_coeff_ * (target - current); }
	float FlushTiny(float value) const { return fabsf(value) < 1.0e-18f ? 0.0f : value; }

	float Shape(float sample) {
		float positive = tanhf(sample * 0.72f);
		float negative = tanhf(sample * 0.88f) * 0.94f;
		float mosfet = sample >= 0.0f ? positive : negative;
		float open_clip = mosfet + 0.055f * tanhf(sample * 0.22f);
		aa_lp_ += Coeff(10500.0f, sample_rate_ * 2.0f) * (open_clip - aa_lp_);
		return aa_lp_;
	}

	float sample_rate_ = 48000.0f;
	float drive_ = 0.38f, tone_ = 0.52f, volume_ = 0.50f, peak_mode_ = 0.0f;
	float drive_z_ = 0.38f, tone_z_ = 0.52f, volume_z_ = 0.50f, peak_z_ = 0.0f;
	float input_lp_ = 0.0f, bass_lp_ = 0.0f, presence_lp_ = 0.0f, tone_lp_ = 0.0f, aa_lp_ = 0.0f;
	float previous_pre_ = 0.0f, dc_x1_ = 0.0f, dc_y1_ = 0.0f;
	float smooth_coeff_ = 0.01f, input_coeff_ = 0.1f, bass_coeff_ = 0.01f;
};