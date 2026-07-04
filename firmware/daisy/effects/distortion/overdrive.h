#pragma once
#include "../../Effect.h"
#include <cmath>
#include <cstring>

class Overdrive : public Effect {
public:
	void Init(float sample_rate) override {
		sample_rate_ = sample_rate;
		drive_ = 4.0f;
		tone_  = 0.55f;
		level_ = 0.8f;
		mix_   = 1.0f;
		input_state_ = 0.0f;
		bass_state_ = 0.0f;
		prev_pre_ = 0.0f;
		aa_state_ = 0.0f;
		tone_state_ = 0.0f;
		post_state_ = 0.0f;
		input_coeff_ = LowPassCoeff(11000.0f, sample_rate_);
		bass_coeff_ = LowPassCoeff(120.0f, sample_rate_);
		UpdateDrive();
		UpdateToneCoeff();
	}

	float Process(float in) override {
		input_state_ += input_coeff_ * (in - input_state_);
		bass_state_ += bass_coeff_ * (input_state_ - bass_state_);
		float pre = input_state_ - (bass_state_ * bass_cut_);
		float midpoint = 0.5f * (prev_pre_ + pre);
		prev_pre_ = pre;

		float shaped = 0.5f * (ShapeOversampled(midpoint) + ShapeOversampled(pre));

		tone_state_ += tone_coeff_ * (shaped - tone_state_);
		float bright = shaped - tone_state_;
		float voiced = tone_state_ + (bright * (0.35f + (tone_ * 1.25f)));
		post_state_ += post_coeff_ * (voiced - post_state_);
		float wet = post_state_ * level_;

		return (in * (1.0f - mix_)) + (wet * mix_);
	}

	const char* GetName() const override { return "overdrive"; }
	EffectCategory GetCategory() const override { return EffectCategory::Distortion; }

	void SetParam(const char* name, float value) override {
		if      (strcmp(name, "drive") == 0) SetDrive(value);
		else if (strcmp(name, "tone") == 0)  SetTone(value);
		else if (strcmp(name, "level") == 0) SetLevel(value);
		else if (strcmp(name, "mix") == 0)   SetMix(value);
	}

	float GetParam(const char* name) override {
		if      (strcmp(name, "drive") == 0) return drive_;
		else if (strcmp(name, "tone") == 0)  return tone_;
		else if (strcmp(name, "level") == 0) return level_;
		else if (strcmp(name, "mix") == 0)   return mix_;
		return 0.0f;
	}

	const char* GetParamList() const override { return "drive,tone,level,mix"; }

	int GetParamCount() const override { return 4; }
	bool GetParamInfo(int index, EffectParamInfo& info) const override {
		switch (index) {
			case 0: info = {"drive", "Drive", "", "float", "linear", 1.0f, 16.0f, 4.0f, 0.1f}; return true;
			case 1: info = {"tone", "Tone", "", "float", "linear", 0.0f, 1.0f, 0.55f, 0.01f}; return true;
			case 2: info = {"level", "Level", "", "float", "linear", 0.0f, 2.0f, 0.8f, 0.01f}; return true;
			case 3: info = {"mix", "Mix", "", "float", "linear", 0.0f, 1.0f, 1.0f, 0.01f}; return true;
			default: return false;
		}
	}

	void SetDrive(float drive) { drive_ = Clamp(drive, 1.0f, 16.0f); UpdateDrive(); }
	void SetTone(float tone)   { tone_ = Clamp(tone, 0.0f, 1.0f); UpdateToneCoeff(); }
	void SetLevel(float level) { level_ = Clamp(level, 0.0f, 2.0f); }
	void SetMix(float mix)     { mix_ = Clamp(mix, 0.0f, 1.0f); }

private:
	static float Clamp(float value, float min, float max) {
		if (value < min) return min;
		if (value > max) return max;
		return value;
	}

	float LowPassCoeff(float cutoff, float rate) const {
		float x = -2.0f * 3.14159265f * cutoff / rate;
		return Clamp(1.0f - expf(x), 0.001f, 0.95f);
	}

	void UpdateDrive() {
		float normalized = (drive_ - 1.0f) / 15.0f;
		drive_gain_ = 1.2f + (normalized * 9.5f);
		output_trim_ = 1.0f / (1.0f + (normalized * 1.8f));
		bass_cut_ = 0.18f + (normalized * 0.28f);
		aa_coeff_ = LowPassCoeff(10000.0f - (normalized * 2500.0f), sample_rate_ * 2.0f);
	}

	void UpdateToneCoeff() {
		float cutoff = 700.0f + (tone_ * 4800.0f);
		tone_coeff_ = LowPassCoeff(cutoff, sample_rate_);
		post_coeff_ = LowPassCoeff(6800.0f + (tone_ * 3600.0f), sample_rate_);
	}

	float ShapeOversampled(float in) {
		float driven = in * drive_gain_;
		float shaped = driven >= 0.0f ? tanhf(driven) : tanhf(driven * 0.78f) * 1.08f;
		aa_state_ += aa_coeff_ * ((shaped * output_trim_) - aa_state_);
		return aa_state_;
	}

	float sample_rate_ = 48000.0f;
	float drive_ = 6.0f;
	float tone_ = 0.55f;
	float level_ = 0.8f;
	float mix_ = 1.0f;
	float input_state_ = 0.0f;
	float bass_state_ = 0.0f;
	float prev_pre_ = 0.0f;
	float aa_state_ = 0.0f;
	float tone_state_ = 0.0f;
	float post_state_ = 0.0f;
	float input_coeff_ = 0.1f;
	float bass_coeff_ = 0.1f;
	float aa_coeff_ = 0.1f;
	float tone_coeff_ = 0.1f;
	float post_coeff_ = 0.1f;
	float drive_gain_ = 4.0f;
	float output_trim_ = 0.8f;
	float bass_cut_ = 0.25f;
};
