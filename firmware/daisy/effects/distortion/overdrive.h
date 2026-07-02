#pragma once
#include "../../Effect.h"
#include "Effects/overdrive.h"
#include <cmath>
#include <cstring>

class Overdrive : public Effect {
public:
	void Init(float sample_rate) override {
		sample_rate_ = sample_rate;
		overdrive_.Init();
		drive_ = 4.0f;
		tone_  = 0.55f;
		level_ = 0.8f;
		mix_   = 1.0f;
		pre_state_ = 0.0f;
		tone_state_ = 0.0f;
		post_state_ = 0.0f;
		pre_coeff_ = LowPassCoeff(9000.0f);
		post_coeff_ = LowPassCoeff(12000.0f);
		UpdateDrive();
		UpdateToneCoeff();
	}

	float Process(float in) override {
		pre_state_ += pre_coeff_ * (in - pre_state_);
		float shaped = overdrive_.Process(pre_state_);

		tone_state_ += tone_coeff_ * (shaped - tone_state_);
		float bright = shaped - tone_state_;
		post_state_ += post_coeff_ * ((tone_state_ + (bright * tone_ * 2.0f)) - post_state_);
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

	float LowPassCoeff(float cutoff) const {
		float x = 2.0f * 3.14159265f * cutoff / sample_rate_;
		return Clamp(x, 0.001f, 1.0f);
	}

	void UpdateDrive() {
		float normalized = (drive_ - 1.0f) / 15.0f;
		overdrive_.SetDrive(normalized);
	}

	void UpdateToneCoeff() {
		float cutoff = 800.0f + (tone_ * 5200.0f);
		tone_coeff_ = LowPassCoeff(cutoff);
	}

	float sample_rate_ = 48000.0f;
	float drive_ = 6.0f;
	float tone_ = 0.55f;
	float level_ = 0.8f;
	float mix_ = 1.0f;
	float pre_state_ = 0.0f;
	float tone_state_ = 0.0f;
	float post_state_ = 0.0f;
	float pre_coeff_ = 0.1f;
	float tone_coeff_ = 0.1f;
	float post_coeff_ = 0.1f;
	daisysp::Overdrive overdrive_;
};
