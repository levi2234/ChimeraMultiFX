#pragma once
#include "Effect.h"
#include <cmath>
#include <cstring>

// mxr_phase90: inspired by the MXR Phase 90. Original four-stage first-order
// all-pass phaser with a simple sine sweep and carefully clamped feedback.
class MXRPhase90 : public Effect {
public:
	void Init(float sample_rate) override {
		sample_rate_ = sample_rate > 1000.0f ? sample_rate : 48000.0f;
		speed_ = 0.45f; depth_ = 0.72f; feedback_ = 0.28f; mix_ = 0.55f; phase_ = fb_state_ = 0.0f;
		for (int i = 0; i < 4; i++) x1_[i] = y1_[i] = 0.0f;
		smooth_coeff_ = Coeff(25.0f); UpdateTargets(); speed_z_ = speed_hz_; depth_z_ = depth_; feedback_z_ = feedback_; mix_z_ = mix_;
	}
	float Process(float in) override {
		Smooth(speed_z_, speed_hz_); Smooth(depth_z_, depth_); Smooth(feedback_z_, feedback_); Smooth(mix_z_, mix_);
		float lfo = 0.5f + 0.5f * sinf(phase_ * 6.28318531f);
		float sweep = 0.08f + depth_z_ * (0.78f * lfo + 0.12f);
		float a = Clamp(0.92f - sweep, 0.05f, 0.92f);
		float y = in + fb_state_ * Clamp(feedback_z_, -0.75f, 0.75f);
		for (int i = 0; i < 4; i++) { float out = -a * y + x1_[i] + a * y1_[i]; x1_[i] = y; y1_[i] = out; y = out; }
		fb_state_ = y;
		phase_ += speed_z_ / sample_rate_; if (phase_ >= 1.0f) phase_ -= 1.0f;
		return in * (1.0f - mix_z_) + y * mix_z_;
	}
	const char* GetName() const override { return "mxr_phase90"; }
	EffectCategory GetCategory() const override { return EffectCategory::Modulation; }
	void SetParam(const char* name, float value) override { if (strcmp(name, "speed") == 0) { speed_ = Clamp(value, 0.05f, 8.0f); UpdateTargets(); } else if (strcmp(name, "depth") == 0) depth_ = Clamp(value, 0.0f, 1.0f); else if (strcmp(name, "feedback") == 0) feedback_ = Clamp(value, -0.75f, 0.75f); else if (strcmp(name, "mix") == 0) mix_ = Clamp(value, 0.0f, 1.0f); }
	float GetParam(const char* name) override { if (strcmp(name, "speed") == 0) return speed_; if (strcmp(name, "depth") == 0) return depth_; if (strcmp(name, "feedback") == 0) return feedback_; if (strcmp(name, "mix") == 0) return mix_; return 0.0f; }
	const char* GetParamList() const override { return "speed,depth,feedback,mix"; }
	int GetParamCount() const override { return 4; }
	bool GetParamInfo(int index, EffectParamInfo& info) const override { switch (index) { case 0: info = {"speed", "MXR Phase 90 Speed", "Hz", "float", "linear", 0.05f, 8.0f, 0.45f, 0.01f}; return true; case 1: info = {"depth", "MXR Phase 90 Depth", "", "float", "linear", 0.0f, 1.0f, 0.72f, 0.01f}; return true; case 2: info = {"feedback", "MXR Phase 90 Feedback", "", "float", "linear", -0.75f, 0.75f, 0.28f, 0.01f}; return true; case 3: info = {"mix", "MXR Phase 90 Mix", "", "float", "linear", 0.0f, 1.0f, 0.55f, 0.01f}; return true; default: return false; } }
private:
	float Coeff(float hz) const { return Clamp(1.0f - expf(-6.28318531f * hz / sample_rate_), 0.0001f, 0.98f); }
	void Smooth(float& current, float target) { current += smooth_coeff_ * (target - current); }
	void UpdateTargets() { speed_hz_ = speed_; }
	float sample_rate_ = 48000.0f, speed_ = 0.45f, depth_ = 0.72f, feedback_ = 0.28f, mix_ = 0.55f, phase_ = 0.0f, fb_state_ = 0.0f;
	float speed_hz_ = 0.45f, smooth_coeff_ = 0.01f, speed_z_ = 0.45f, depth_z_ = 0.72f, feedback_z_ = 0.28f, mix_z_ = 0.55f;
	float x1_[4] = {}, y1_[4] = {};
};