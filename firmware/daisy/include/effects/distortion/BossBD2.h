#pragma once
#include "Effect.h"
#include <cmath>
#include <cstring>

// Original multi-stage model voiced after the bright, full-range and highly
// dynamic response associated with a BD-2-style discrete overdrive.
class BossBD2 : public Effect {
public:
	void Init(float sample_rate) override {
		sample_rate_ = sample_rate > 1000.0f ? sample_rate : 48000.0f;
		gain_ = 0.34f;
		tone_ = 0.50f;
		level_ = 0.52f;
		gain_z_ = gain_;
		tone_z_ = tone_;
		level_z_ = level_;
		input_lp_ = input_hp_lp_ = interstage_lp_ = stage1_memory_ = aa_lp_ = tone_lp_ = 0.0f;
		previous_pre_ = envelope_ = dc_x1_ = dc_y1_ = 0.0f;
		smooth_coeff_ = Coeff(34.0f, sample_rate_);
		input_coeff_ = Coeff(17000.0f, sample_rate_);
		hp_coeff_ = Coeff(68.0f, sample_rate_);
	}

	float Process(float in) override {
		Smooth(gain_z_, gain_);
		Smooth(tone_z_, tone_);
		Smooth(level_z_, level_);

		input_lp_ += input_coeff_ * (in - input_lp_);
		input_hp_lp_ += hp_coeff_ * (input_lp_ - input_hp_lp_);
		float pre = input_lp_ - input_hp_lp_;
		float envelope_coeff = fabsf(pre) > envelope_ ? Coeff(1800.0f, sample_rate_) : Coeff(24.0f, sample_rate_);
		envelope_ += envelope_coeff * (fabsf(pre) - envelope_);

		float midpoint = 0.5f * (previous_pre_ + pre);
		previous_pre_ = pre;
		float first = Cascade(midpoint);
		float second = Cascade(pre);
		float clipped = 0.5f * (first + second);

		float tone_cut = 1250.0f * powf(9.5f, tone_z_);
		tone_lp_ += Coeff(tone_cut, sample_rate_) * (clipped - tone_lp_);
		float top = clipped - tone_lp_;
		float voiced = tone_lp_ + top * (0.38f + 1.28f * tone_z_);
		float compensation = 1.04f / (1.0f + 0.72f * gain_z_);
		float output_gain = (0.04f + 2.35f * level_z_ * level_z_) * compensation;
		float wet = voiced * output_gain;
		float dc_blocked = wet - dc_x1_ + 0.995f * dc_y1_;
		dc_x1_ = wet;
		dc_y1_ = FlushTiny(dc_blocked);
		return dc_y1_;
	}

	const char* GetName() const override { return "BossBD2"; }
	EffectCategory GetCategory() const override { return EffectCategory::Distortion; }

	void SetParam(const char* name, float value) override {
		if (strcmp(name, "gain") == 0) gain_ = Clamp(value, 0.0f, 1.0f);
		else if (strcmp(name, "tone") == 0) tone_ = Clamp(value, 0.0f, 1.0f);
		else if (strcmp(name, "level") == 0) level_ = Clamp(value, 0.0f, 1.0f);
	}

	float GetParam(const char* name) override {
		if (strcmp(name, "gain") == 0) return gain_;
		if (strcmp(name, "tone") == 0) return tone_;
		if (strcmp(name, "level") == 0) return level_;
		return 0.0f;
	}

	const char* GetParamList() const override { return "gain,tone,level"; }
	int GetParamCount() const override { return 3; }
	bool GetParamInfo(int index, EffectParamInfo& info) const override {
		switch (index) {
			case 0: info = {"gain", "BD-2 Gain", "", "float", "log", 0.0f, 1.0f, 0.34f, 0.01f}; return true;
			case 1: info = {"tone", "BD-2 Tone", "", "float", "linear", 0.0f, 1.0f, 0.50f, 0.01f}; return true;
			case 2: info = {"level", "BD-2 Level", "", "float", "log", 0.0f, 1.0f, 0.52f, 0.01f}; return true;
			default: return false;
		}
	}

private:
	float Coeff(float hz, float rate) const {
		return Clamp(1.0f - expf(-6.28318531f * hz / rate), 0.0001f, 0.98f);
	}
	void Smooth(float& current, float target) { current += smooth_coeff_ * (target - current); }
	float FlushTiny(float value) const { return fabsf(value) < 1.0e-18f ? 0.0f : value; }

	float Cascade(float sample) {
		float rate = sample_rate_ * 2.0f;
		float first_gain = 1.05f + 8.5f * gain_z_ * gain_z_;
		float first_drive = sample * first_gain;
		float first_pos = tanhf(first_drive * 0.72f);
		float first_neg = tanhf(first_drive * 0.94f) * 0.91f;
		float stage1 = first_drive >= 0.0f ? first_pos : first_neg;
		stage1_memory_ += Coeff(6100.0f, rate) * (stage1 - stage1_memory_);

		interstage_lp_ += Coeff(245.0f, rate) * (stage1_memory_ - interstage_lp_);
		float controlled = (stage1_memory_ - interstage_lp_) + interstage_lp_ * (0.82f - 0.22f * gain_z_);
		float second_gain = 1.0f + 10.5f * gain_z_ * gain_z_;
		float dynamic_headroom = 1.0f + Clamp(envelope_ * 1.8f, 0.0f, 0.22f);
		float second_drive = controlled * second_gain / dynamic_headroom;
		float soft = second_drive >= 0.0f ? tanhf(second_drive * 0.82f) : tanhf(second_drive * 1.08f) * 0.90f;
		float hard = Clamp(second_drive, -0.84f, 0.76f);
		float edge = gain_z_ * gain_z_ * 0.32f;
		float shaped = soft * (1.0f - edge) + hard * edge;
		aa_lp_ += Coeff(10400.0f, rate) * (shaped - aa_lp_);
		return aa_lp_;
	}

	float sample_rate_ = 48000.0f;
	float gain_ = 0.34f, tone_ = 0.50f, level_ = 0.52f;
	float gain_z_ = 0.34f, tone_z_ = 0.50f, level_z_ = 0.52f;
	float input_lp_ = 0.0f, input_hp_lp_ = 0.0f, interstage_lp_ = 0.0f;
	float stage1_memory_ = 0.0f, aa_lp_ = 0.0f, tone_lp_ = 0.0f;
	float previous_pre_ = 0.0f, envelope_ = 0.0f, dc_x1_ = 0.0f, dc_y1_ = 0.0f;
	float smooth_coeff_ = 0.01f, input_coeff_ = 0.1f, hp_coeff_ = 0.01f;
};