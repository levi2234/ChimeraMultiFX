#pragma once
#include "daisy_seed.h"
#include "Effect.h"
#include <cmath>
#include <cstring>

// StrymonBluesky: inspired by the Strymon BlueSky reverberator shown on the pedal:
// VERB selects plate/room/spring, MOD selects off/light/deep, and the front-panel
// knobs are decay, pre-delay, low, high, shimmer, and mix. This is an original
// embedded FDN-style reverb with a lightweight octave-like shimmer approximation,
// not a component-accurate or algorithm-identical clone.
class StrymonBlueSky : public Effect {
public:
	static constexpr int MAX_PRE = 12000;
	static constexpr int MAX_LINE = 16384;

	void Init(float sample_rate) override {
		sample_rate_ = sample_rate > 1000.0f ? sample_rate : 48000.0f;
		verb_ = 0.0f;
		mod_ = 1.0f;
		decay_ = 0.58f;
		pre_delay_ = 0.018f;
		low_ = 0.52f;
		high_ = 0.58f;
		shimmer_ = 0.20f;
		mix_ = 0.32f;

		pre_write_ = 0;
		lfo_phase_ = 0.0f;
		density_state_ = shimmer_hp_lp_ = shimmer_lp_ = spring_bp_ = spring_lp_ = 0.0f;
		for (int line = 0; line < 4; line++) {
			write_[line] = 0;
			damp_lp_[line] = 0.0f;
			low_lp_[line] = 0.0f;
			for (int i = 0; i < MAX_LINE; i++) tank_[line][i] = 0.0f;
		}
		for (int i = 0; i < MAX_PRE; i++) pre_buffer_[i] = 0.0f;

		smooth_coeff_ = Coeff(18.0f);
		UpdateTargets();
		verb_z_ = verb_;
		mod_z_ = mod_;
		decay_z_ = decay_gain_;
		predelay_z_ = predelay_samples_;
		low_z_ = low_;
		high_z_ = high_;
		shimmer_z_ = shimmer_;
		mix_z_ = mix_;
	}

	float Process(float in) override {
		Smooth(verb_z_, verb_);
		Smooth(mod_z_, mod_);
		Smooth(decay_z_, decay_gain_);
		Smooth(predelay_z_, predelay_samples_);
		Smooth(low_z_, low_);
		Smooth(high_z_, high_);
		Smooth(shimmer_z_, shimmer_);
		Smooth(mix_z_, mix_);

		pre_buffer_[pre_write_] = in;
		float predelayed = ReadPreDelay(predelay_z_);
		int mode = ClampInt(static_cast<int>(verb_z_ + 0.5f), 0, 2);
		int mod_mode = ClampInt(static_cast<int>(mod_z_ + 0.5f), 0, 2);
		const float* base_times = ModeTimes(mode);
		float mod_depth = mod_mode == 0 ? 0.0f : (mod_mode == 1 ? 0.00055f : 0.0018f);
		float mod_rate = mod_mode == 0 ? 0.0f : (mod_mode == 1 ? 0.18f : 0.42f);
		float lfo = sinf(lfo_phase_ * 6.28318531f);
		float lfo_quadrature = sinf(lfo_phase_ * 6.28318531f + 1.57079633f);

		float y[4];
		for (int line = 0; line < 4; line++) {
			float polarity = (line & 1) ? lfo_quadrature : lfo;
			float delay = (base_times[line] + mod_depth * polarity) * sample_rate_;
			y[line] = ReadLine(line, delay);
		}

		float tank_out = (y[0] + y[1] + y[2] + y[3]) * 0.25f;
		shimmer_hp_lp_ += Coeff(900.0f) * (tank_out - shimmer_hp_lp_);
		float shimmer_source = tank_out - shimmer_hp_lp_;
		shimmer_lp_ += Coeff(5200.0f) * ((fabsf(shimmer_source) * 2.0f - 0.18f) - shimmer_lp_);
		float shimmer_feed = shimmer_lp_ * shimmer_z_ * 0.36f;

		float density_drive = predelayed + shimmer_feed;
		density_state_ += Coeff(mode == 2 ? 1800.0f : 4200.0f) * (density_drive - density_state_);
		float inject = density_drive * 0.72f + density_state_ * 0.28f;

		float matrix[4];
		matrix[0] = ( y[0] + y[1] - y[2] - y[3]) * 0.5f;
		matrix[1] = (-y[0] + y[1] + y[2] - y[3]) * 0.5f;
		matrix[2] = ( y[0] - y[1] + y[2] - y[3]) * 0.5f;
		matrix[3] = ( y[0] + y[1] + y[2] + y[3]) * 0.5f;

		float damp_coeff = Coeff(HighCutForMode(mode, high_z_));
		float low_coeff = Coeff(mode == 2 ? 260.0f : 190.0f);
		float low_gain = 0.62f + low_z_ * 0.78f;
		float mode_gain = mode == 0 ? 0.96f : (mode == 1 ? 0.90f : 0.82f);
		float feedback_gain = Clamp(decay_z_ * mode_gain, 0.05f, 0.93f);

		for (int line = 0; line < 4; line++) {
			damp_lp_[line] += damp_coeff * (matrix[line] - damp_lp_[line]);
			low_lp_[line] += low_coeff * (damp_lp_[line] - low_lp_[line]);
			float voiced = damp_lp_[line] + low_lp_[line] * (low_gain - 1.0f);
			float input = inject * InputGain(line, mode) + voiced * feedback_gain;
			tank_[line][write_[line]] = FlushTiny(tanhf(input * 0.92f));
			write_[line] = (write_[line] + 1) & (MAX_LINE - 1);
		}

		if (mode == 2) {
			float spring_in = tank_out + predelayed * 0.18f;
			spring_lp_ += Coeff(760.0f) * (spring_in - spring_lp_);
			spring_bp_ += Coeff(2100.0f) * ((spring_in - spring_lp_) - spring_bp_);
			tank_out = tank_out * 0.72f + tanhf(spring_bp_ * 2.1f) * 0.28f;
		}

		lfo_phase_ += mod_rate / sample_rate_;
		if (lfo_phase_ >= 1.0f) lfo_phase_ -= 1.0f;
		pre_write_ = (pre_write_ + 1) % MAX_PRE;

		float wet = tank_out + shimmer_lp_ * shimmer_z_ * 0.18f;
		return in * (1.0f - mix_z_) + wet * mix_z_;
	}

	const char* GetName() const override { return "StrymonBluesky"; }
	EffectCategory GetCategory() const override { return EffectCategory::Time; }

	void SetParam(const char* name, float value) override {
		if (strcmp(name, "verb") == 0) verb_ = Clamp(value, 0.0f, 2.0f);
		else if (strcmp(name, "mod") == 0) mod_ = Clamp(value, 0.0f, 2.0f);
		else if (strcmp(name, "decay") == 0) { decay_ = Clamp(value, 0.05f, 0.98f); UpdateTargets(); }
		else if (strcmp(name, "pre_delay") == 0) { pre_delay_ = Clamp(value, 0.0f, 0.12f); UpdateTargets(); }
		else if (strcmp(name, "low") == 0) low_ = Clamp(value, 0.0f, 1.0f);
		else if (strcmp(name, "high") == 0) high_ = Clamp(value, 0.0f, 1.0f);
		else if (strcmp(name, "tone") == 0) high_ = Clamp(value, 0.0f, 1.0f);
		else if (strcmp(name, "shimmer") == 0) shimmer_ = Clamp(value, 0.0f, 1.0f);
		else if (strcmp(name, "mix") == 0) mix_ = Clamp(value, 0.0f, 1.0f);
	}

	float GetParam(const char* name) override {
		if (strcmp(name, "verb") == 0) return verb_;
		if (strcmp(name, "mod") == 0) return mod_;
		if (strcmp(name, "decay") == 0) return decay_;
		if (strcmp(name, "pre_delay") == 0) return pre_delay_;
		if (strcmp(name, "low") == 0) return low_;
		if (strcmp(name, "high") == 0) return high_;
		if (strcmp(name, "tone") == 0) return high_;
		if (strcmp(name, "shimmer") == 0) return shimmer_;
		if (strcmp(name, "mix") == 0) return mix_;
		return 0.0f;
	}

	const char* GetParamList() const override { return "verb,mod,decay,pre_delay,low,high,shimmer,mix"; }
	int GetParamCount() const override { return 8; }
	bool GetParamInfo(int index, EffectParamInfo& info) const override {
		switch (index) {
			case 0: info = {"verb", "BlueSky Verb", "", "int", "linear", 0.0f, 2.0f, 0.0f, 1.0f}; return true;
			case 1: info = {"mod", "BlueSky Mod", "", "int", "linear", 0.0f, 2.0f, 1.0f, 1.0f}; return true;
			case 2: info = {"decay", "BlueSky Decay", "", "float", "linear", 0.05f, 0.98f, 0.58f, 0.01f}; return true;
			case 3: info = {"pre_delay", "BlueSky Pre-Delay", "s", "float", "linear", 0.0f, 0.12f, 0.018f, 0.001f}; return true;
			case 4: info = {"low", "BlueSky Low", "", "float", "linear", 0.0f, 1.0f, 0.52f, 0.01f}; return true;
			case 5: info = {"high", "BlueSky High", "", "float", "linear", 0.0f, 1.0f, 0.58f, 0.01f}; return true;
			case 6: info = {"shimmer", "BlueSky Shimmer", "", "float", "linear", 0.0f, 1.0f, 0.20f, 0.01f}; return true;
			case 7: info = {"mix", "BlueSky Mix", "", "float", "linear", 0.0f, 1.0f, 0.32f, 0.01f}; return true;
			default: return false;
		}
	}

private:
	float Coeff(float hz) const { return Clamp(1.0f - expf(-6.28318531f * hz / sample_rate_), 0.0001f, 0.98f); }
	void Smooth(float& current, float target) { current += smooth_coeff_ * (target - current); }
	void UpdateTargets() { float curve = decay_ * decay_; decay_gain_ = 0.28f + curve * 0.68f; predelay_samples_ = Clamp(pre_delay_ * sample_rate_, 0.0f, static_cast<float>(MAX_PRE - 2)); }

	const float* ModeTimes(int mode) const {
		static const float plate[4] = {0.0317f, 0.0379f, 0.0437f, 0.0571f};
		static const float room[4] = {0.0413f, 0.0539f, 0.0677f, 0.0893f};
		static const float spring[4] = {0.0241f, 0.0311f, 0.0449f, 0.0623f};
		return mode == 0 ? plate : (mode == 1 ? room : spring);
	}

	float InputGain(int line, int mode) const {
		static const float plate[4] = {0.62f, 0.48f, 0.55f, 0.41f};
		static const float room[4] = {0.52f, 0.44f, 0.37f, 0.31f};
		static const float spring[4] = {0.72f, 0.38f, 0.56f, 0.29f};
		return mode == 0 ? plate[line] : (mode == 1 ? room[line] : spring[line]);
	}

	float HighCutForMode(int mode, float high) const {
		float base = mode == 0 ? 1900.0f : (mode == 1 ? 1500.0f : 1200.0f);
		float span = mode == 0 ? 10500.0f : (mode == 1 ? 7800.0f : 5600.0f);
		return base + high * span;
	}

	float FlushTiny(float value) const { return fabsf(value) < 1.0e-18f ? 0.0f : value; }
	float ReadPreDelay(float delay) const {
		delay = Clamp(delay, 0.0f, static_cast<float>(MAX_PRE - 2));
		float pos = static_cast<float>(pre_write_) - delay;
		while (pos < 0.0f) pos += MAX_PRE;
		int i0 = static_cast<int>(pos) % MAX_PRE;
		int i1 = (i0 + 1) % MAX_PRE;
		float frac = pos - static_cast<int>(pos);
		return pre_buffer_[i0] + frac * (pre_buffer_[i1] - pre_buffer_[i0]);
	}

	float ReadLine(int line, float delay) const {
		delay = Clamp(delay, 1.0f, static_cast<float>(MAX_LINE - 2));
		float pos = static_cast<float>(write_[line]) - delay;
		while (pos < 0.0f) pos += MAX_LINE;
		int i0 = static_cast<int>(pos) & (MAX_LINE - 1);
		int i1 = (i0 + 1) & (MAX_LINE - 1);
		float frac = pos - static_cast<int>(pos);
		return tank_[line][i0] + frac * (tank_[line][i1] - tank_[line][i0]);
	}

	float sample_rate_ = 48000.0f;
	float verb_ = 0.0f, mod_ = 1.0f, decay_ = 0.58f, pre_delay_ = 0.018f, low_ = 0.52f, high_ = 0.58f, shimmer_ = 0.20f, mix_ = 0.32f;
	float verb_z_ = 0.0f, mod_z_ = 1.0f, decay_gain_ = 0.7f, predelay_samples_ = 864.0f, decay_z_ = 0.7f, predelay_z_ = 864.0f;
	float low_z_ = 0.52f, high_z_ = 0.58f, shimmer_z_ = 0.20f, mix_z_ = 0.32f, smooth_coeff_ = 0.01f;
	float lfo_phase_ = 0.0f, density_state_ = 0.0f, shimmer_hp_lp_ = 0.0f, shimmer_lp_ = 0.0f, spring_bp_ = 0.0f, spring_lp_ = 0.0f;
	float damp_lp_[4] = {}, low_lp_[4] = {};
	int pre_write_ = 0;
	int write_[4] = {};
	static inline float DSY_SDRAM_BSS pre_buffer_[MAX_PRE];
	static inline float DSY_SDRAM_BSS tank_[4][MAX_LINE];
};