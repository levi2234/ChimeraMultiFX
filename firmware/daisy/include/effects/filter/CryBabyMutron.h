#pragma once
#include "Effect.h"
#include <cmath>
#include <cstring>

// CryBabyMutron: inspired by Dunlop Cry Baby Wah and Mu-Tron III. Original
// resonant TPT band-pass wah with manual, envelope-up, and envelope-down modes.
class CryBabyMutron : public Effect {
public:
	void Init(float sample_rate) override {
		sample_rate_ = sample_rate > 1000.0f ? sample_rate : 48000.0f;
		mode_ = 1.0f; position_ = 0.42f; sensitivity_ = 0.64f; resonance_ = 0.62f; attack_ = 9.0f; release_ = 180.0f; mix_ = 1.0f;
		envelope_ = ic1eq_ = ic2eq_ = 0.0f; update_counter_ = 0; smooth_coeff_ = Coeff(35.0f); UpdateEnvelopeCoeffs(); UpdateFilterTargets(); cutoff_z_ = cutoff_; resonance_z_ = resonance_; mix_z_ = mix_;
	}
	float Process(float in) override {
		float mag = fabsf(in);
		float detector = mag > envelope_ ? attack_coeff_ : release_coeff_;
		envelope_ = detector * envelope_ + (1.0f - detector) * mag;
		if (++update_counter_ >= 8) { update_counter_ = 0; UpdateFilterTargets(); }
		Smooth(cutoff_z_, cutoff_); Smooth(resonance_z_, resonance_); Smooth(mix_z_, mix_);
		float g = tanf(3.14159265f * Clamp(cutoff_z_, 120.0f, 3200.0f) / sample_rate_);
		float damping = 1.9f - resonance_z_ * 1.65f;
		float a1 = 1.0f / (1.0f + g * (g + damping));
		float v1 = a1 * (ic1eq_ + g * (in - ic2eq_));
		float v2 = ic2eq_ + g * v1;
		ic1eq_ = FlushTiny(2.0f * v1 - ic1eq_);
		ic2eq_ = FlushTiny(2.0f * v2 - ic2eq_);
		float wet = tanhf(v1 * (1.4f + resonance_z_));
		return in * (1.0f - mix_z_) + wet * mix_z_;
	}
	const char* GetName() const override { return "CryBabyMutron"; }
	EffectCategory GetCategory() const override { return EffectCategory::Filter; }
	void SetParam(const char* name, float value) override { if (strcmp(name, "mode") == 0) { mode_ = Clamp(value, 0.0f, 2.0f); UpdateFilterTargets(); } else if (strcmp(name, "position") == 0) { position_ = Clamp(value, 0.0f, 1.0f); UpdateFilterTargets(); } else if (strcmp(name, "sensitivity") == 0) { sensitivity_ = Clamp(value, 0.0f, 1.0f); UpdateFilterTargets(); } else if (strcmp(name, "resonance") == 0) resonance_ = Clamp(value, 0.0f, 1.0f); else if (strcmp(name, "attack") == 0) { attack_ = Clamp(value, 1.0f, 80.0f); UpdateEnvelopeCoeffs(); } else if (strcmp(name, "release") == 0) { release_ = Clamp(value, 20.0f, 800.0f); UpdateEnvelopeCoeffs(); } else if (strcmp(name, "mix") == 0) mix_ = Clamp(value, 0.0f, 1.0f); }
	float GetParam(const char* name) override { if (strcmp(name, "mode") == 0) return mode_; if (strcmp(name, "position") == 0) return position_; if (strcmp(name, "sensitivity") == 0) return sensitivity_; if (strcmp(name, "resonance") == 0) return resonance_; if (strcmp(name, "attack") == 0) return attack_; if (strcmp(name, "release") == 0) return release_; if (strcmp(name, "mix") == 0) return mix_; return 0.0f; }
	const char* GetParamList() const override { return "mode,position,sensitivity,resonance,attack,release,mix"; }
	int GetParamCount() const override { return 7; }
	bool GetParamInfo(int index, EffectParamInfo& info) const override { switch (index) { case 0: info = {"mode", "Cry Baby / Mu-Tron Mode", "", "int", "linear", 0.0f, 2.0f, 1.0f, 1.0f}; return true; case 1: info = {"position", "Cry Baby Position", "", "float", "linear", 0.0f, 1.0f, 0.42f, 0.01f}; return true; case 2: info = {"sensitivity", "Mu-Tron Sensitivity", "", "float", "linear", 0.0f, 1.0f, 0.64f, 0.01f}; return true; case 3: info = {"resonance", "Cry Baby / Mu-Tron Resonance", "", "float", "linear", 0.0f, 1.0f, 0.62f, 0.01f}; return true; case 4: info = {"attack", "Mu-Tron Attack", "ms", "float", "log", 1.0f, 80.0f, 9.0f, 0.1f}; return true; case 5: info = {"release", "Mu-Tron Release", "ms", "float", "log", 20.0f, 800.0f, 180.0f, 1.0f}; return true; case 6: info = {"mix", "Cry Baby / Mu-Tron Mix", "", "float", "linear", 0.0f, 1.0f, 1.0f, 0.01f}; return true; default: return false; } }
private:
	float Coeff(float hz) const { return Clamp(1.0f - expf(-6.28318531f * hz / sample_rate_), 0.0001f, 0.98f); }
	void Smooth(float& current, float target) { current += smooth_coeff_ * (target - current); }
	void UpdateEnvelopeCoeffs() { attack_coeff_ = expf(-1.0f / (0.001f * attack_ * sample_rate_)); release_coeff_ = expf(-1.0f / (0.001f * release_ * sample_rate_)); }
	void UpdateFilterTargets() { float env = Clamp((envelope_ / (envelope_ + 0.06f)) * sensitivity_ * 1.35f, 0.0f, 1.0f); int mode = static_cast<int>(mode_ + 0.5f); float sweep = mode == 0 ? position_ : (mode == 1 ? env : 1.0f - env); cutoff_ = 260.0f + sweep * 2350.0f; }
	float FlushTiny(float value) const { return fabsf(value) < 1.0e-18f ? 0.0f : value; }
	float sample_rate_ = 48000.0f, mode_ = 1.0f, position_ = 0.42f, sensitivity_ = 0.64f, resonance_ = 0.62f, attack_ = 9.0f, release_ = 180.0f, mix_ = 1.0f;
	float envelope_ = 0.0f, attack_coeff_ = 0.0f, release_coeff_ = 0.0f, cutoff_ = 900.0f, cutoff_z_ = 900.0f, resonance_z_ = 0.62f, mix_z_ = 1.0f, smooth_coeff_ = 0.01f;
	float ic1eq_ = 0.0f, ic2eq_ = 0.0f;
	int update_counter_ = 0;
};