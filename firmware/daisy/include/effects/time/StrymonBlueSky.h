#pragma once
#include "Effect.h"
#include <cmath>
#include <cstring>

// StrymonBluesky: inspired by the Strymon BlueSky. Original lightweight plate/room
// reverb using delay-network diffusion and a bright shimmer approximation rather than CPU-heavy pitch shifting.
class StrymonBlueSky : public Effect {
public:
	static constexpr int PRE_BUF = 4800;
	static constexpr int TANK_SIZE = 8192;
	void Init(float sample_rate) override {
		sample_rate_ = sample_rate > 1000.0f ? sample_rate : 48000.0f;
		decay_ = 0.58f; pre_delay_ = 0.018f; tone_ = 0.56f; shimmer_ = 0.18f; mix_ = 0.32f; pre_write_ = tank_index_ = 0; damping_lp_ = shimmer_hp_lp_ = shimmer_lp_ = 0.0f;
		for (int i = 0; i < PRE_BUF; i++) pre_buffer_[i] = 0.0f;
		for (int i = 0; i < TANK_SIZE; i++) tank_[i] = 0.0f;
		smooth_coeff_ = Coeff(16.0f); UpdateTargets(); decay_z_ = decay_gain_; predelay_z_ = predelay_samples_; tone_z_ = tone_; shimmer_z_ = shimmer_; mix_z_ = mix_;
	}
	float Process(float in) override {
		Smooth(decay_z_, decay_gain_); Smooth(predelay_z_, predelay_samples_); Smooth(tone_z_, tone_); Smooth(shimmer_z_, shimmer_); Smooth(mix_z_, mix_);
		pre_buffer_[pre_write_] = in;
		float predelayed = ReadPreDelay(predelay_z_);
		float a = tank_[(tank_index_ +  631) & (TANK_SIZE - 1)];
		float b = tank_[(tank_index_ + 1879) & (TANK_SIZE - 1)];
		float c = tank_[(tank_index_ + 4211) & (TANK_SIZE - 1)];
		float d = tank_[(tank_index_ + 6763) & (TANK_SIZE - 1)];
		float tank_out = (a + b + c + d) * 0.25f;
		damping_lp_ += Coeff(1400.0f + tone_z_ * 7200.0f) * (tank_out - damping_lp_);
		shimmer_hp_lp_ += Coeff(950.0f) * (tank_out - shimmer_hp_lp_);
		shimmer_lp_ += Coeff(6200.0f) * ((tank_out - shimmer_hp_lp_) - shimmer_lp_);
		float inject = predelayed + damping_lp_ * decay_z_ + shimmer_lp_ * shimmer_z_ * 0.55f;
		float diffuse = tanhf(inject * 0.72f + (a - b + c - d) * 0.18f);
		tank_[tank_index_] = FlushTiny(diffuse);
		tank_index_ = (tank_index_ + 1) & (TANK_SIZE - 1);
		pre_write_ = (pre_write_ + 1) % PRE_BUF;
		return in * (1.0f - mix_z_) + tank_out * mix_z_;
	}
	const char* GetName() const override { return "StrymonBluesky"; }
	EffectCategory GetCategory() const override { return EffectCategory::Time; }
	void SetParam(const char* name, float value) override { if (strcmp(name, "decay") == 0) { decay_ = Clamp(value, 0.05f, 0.95f); UpdateTargets(); } else if (strcmp(name, "pre_delay") == 0) { pre_delay_ = Clamp(value, 0.0f, 0.1f); UpdateTargets(); } else if (strcmp(name, "tone") == 0) tone_ = Clamp(value, 0.0f, 1.0f); else if (strcmp(name, "shimmer") == 0) shimmer_ = Clamp(value, 0.0f, 1.0f); else if (strcmp(name, "mix") == 0) mix_ = Clamp(value, 0.0f, 1.0f); }
	float GetParam(const char* name) override { if (strcmp(name, "decay") == 0) return decay_; if (strcmp(name, "pre_delay") == 0) return pre_delay_; if (strcmp(name, "tone") == 0) return tone_; if (strcmp(name, "shimmer") == 0) return shimmer_; if (strcmp(name, "mix") == 0) return mix_; return 0.0f; }
	const char* GetParamList() const override { return "decay,pre_delay,tone,shimmer,mix"; }
	int GetParamCount() const override { return 5; }
	bool GetParamInfo(int index, EffectParamInfo& info) const override { switch (index) { case 0: info = {"decay", "Strymon BlueSky Decay", "", "float", "linear", 0.05f, 0.95f, 0.58f, 0.01f}; return true; case 1: info = {"pre_delay", "Strymon BlueSky Pre-Delay", "s", "float", "linear", 0.0f, 0.1f, 0.018f, 0.001f}; return true; case 2: info = {"tone", "Strymon BlueSky Tone", "", "float", "linear", 0.0f, 1.0f, 0.56f, 0.01f}; return true; case 3: info = {"shimmer", "Strymon BlueSky Shimmer", "", "float", "linear", 0.0f, 1.0f, 0.18f, 0.01f}; return true; case 4: info = {"mix", "Strymon BlueSky Mix", "", "float", "linear", 0.0f, 1.0f, 0.32f, 0.01f}; return true; default: return false; } }
private:
	float Coeff(float hz) const { return Clamp(1.0f - expf(-6.28318531f * hz / sample_rate_), 0.0001f, 0.98f); }
	void Smooth(float& current, float target) { current += smooth_coeff_ * (target - current); }
	void UpdateTargets() { decay_gain_ = 0.35f + decay_ * 0.58f; predelay_samples_ = Clamp(pre_delay_ * sample_rate_, 0.0f, static_cast<float>(PRE_BUF - 2)); }
	float FlushTiny(float value) const { return fabsf(value) < 1.0e-18f ? 0.0f : value; }
	float ReadPreDelay(float delay) const { float pos = static_cast<float>(pre_write_) - delay; while (pos < 0.0f) pos += PRE_BUF; int i0 = static_cast<int>(pos) % PRE_BUF; int i1 = (i0 + 1) % PRE_BUF; float frac = pos - static_cast<int>(pos); return pre_buffer_[i0] + frac * (pre_buffer_[i1] - pre_buffer_[i0]); }
	float sample_rate_ = 48000.0f, decay_ = 0.58f, pre_delay_ = 0.018f, tone_ = 0.56f, shimmer_ = 0.18f, mix_ = 0.32f;
	float decay_gain_ = 0.7f, predelay_samples_ = 864.0f, decay_z_ = 0.7f, predelay_z_ = 864.0f, tone_z_ = 0.56f, shimmer_z_ = 0.18f, mix_z_ = 0.32f, smooth_coeff_ = 0.01f;
	float damping_lp_ = 0.0f, shimmer_hp_lp_ = 0.0f, shimmer_lp_ = 0.0f;
	int pre_write_ = 0, tank_index_ = 0;
	float pre_buffer_[PRE_BUF];
	float tank_[TANK_SIZE];
};