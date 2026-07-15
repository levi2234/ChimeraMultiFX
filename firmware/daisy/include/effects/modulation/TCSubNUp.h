#pragma once
#include "Effect.h"
#include <cmath>
#include <cstring>

// Original low-latency octave model. Poly mode uses overlapping delay grains
// and retains chord content; Classic mode uses intentionally vintage pitch
// tracking and octave-divider waveforms.
class TCSubNUp : public Effect {
public:
	static constexpr int BUFFER_SIZE = 2048;

	void Init(float sample_rate) override {
		sample_rate_ = sample_rate > 1000.0f ? sample_rate : 48000.0f;
		dry_ = 1.0f;
		up_ = 0.32f;
		sub_ = 0.42f;
		sub2_ = 0.0f;
		mode_ = 0.0f;
		dry_z_ = dry_;
		up_z_ = up_;
		sub_z_ = sub_;
		sub2_z_ = sub2_;
		mode_z_ = mode_;
		write_ = crossing_count_ = samples_since_crossing_ = 0;
		phase_up_ = 0.17f;
		phase_sub_ = 0.43f;
		phase_sub2_ = 0.71f;
		classic_phase_ = 0.0f;
		period_ = sample_rate_ / 110.0f;
		previous_input_ = envelope_ = tracking_confidence_ = sub_lp_ = sub2_lp_ = classic_sub_lp_ = 0.0f;
		tracker_armed_ = false;
		smooth_coeff_ = Coeff(28.0f);
		for (int index = 0; index < BUFFER_SIZE; index++) buffer_[index] = 0.0f;
	}

	float Process(float in) override {
		Smooth(dry_z_, dry_);
		Smooth(up_z_, up_);
		Smooth(sub_z_, sub_);
		Smooth(sub2_z_, sub2_);
		Smooth(mode_z_, mode_);

		buffer_[write_] = FlushTiny(in);
		float window = Clamp(sample_rate_ * 0.021f, 640.0f, 1400.0f);
		float poly_up = ReadGrains(phase_up_, 2.0f, window);
		float poly_sub = ReadGrains(phase_sub_, 0.5f, window);
		float poly_sub2 = ReadGrains(phase_sub2_, 0.25f, window);
		sub_lp_ += Coeff(4200.0f) * (poly_sub - sub_lp_);
		sub2_lp_ += Coeff(2250.0f) * (poly_sub2 - sub2_lp_);

		UpdateTracker(in);
		float phase_radians = classic_phase_ * 6.28318531f;
		float classic_up = tanhf(sinf(phase_radians * 2.0f) * 2.6f) * envelope_ * 2.25f;
		float classic_sub_raw = tanhf(sinf(phase_radians * 0.5f) * 3.2f) * envelope_ * 2.35f;
		float classic_sub2_raw = tanhf(sinf(phase_radians * 0.25f) * 3.5f) * envelope_ * 2.35f;
		classic_sub_lp_ += Coeff(3100.0f) * (classic_sub_raw - classic_sub_lp_);
		classic_sub2_lp_ += Coeff(1650.0f) * (classic_sub2_raw - classic_sub2_lp_);
		float classic_gate = tracking_confidence_;

		float up_voice = Lerp(poly_up, classic_up * classic_gate, mode_z_);
		float sub_voice = Lerp(sub_lp_, classic_sub_lp_ * classic_gate, mode_z_);
		float sub2_voice = Lerp(sub2_lp_, classic_sub2_lp_ * classic_gate, mode_z_);
		float dry_gain = dry_z_ * dry_z_;
		float up_gain = up_z_ * up_z_;
		float sub_gain = sub_z_ * sub_z_;
		float sub2_gain = sub2_z_ * sub2_z_;
		float power = dry_gain * dry_gain + up_gain * up_gain + sub_gain * sub_gain + sub2_gain * sub2_gain;
		float normalization = power > 1.0f ? 1.0f / sqrtf(power) : 1.0f;
		float out = (in * dry_gain + up_voice * up_gain + sub_voice * sub_gain + sub2_voice * sub2_gain) * normalization;
		write_ = (write_ + 1) & (BUFFER_SIZE - 1);
		return FlushTiny(out);
	}

	const char* GetName() const override { return "TCSubNUp"; }
	EffectCategory GetCategory() const override { return EffectCategory::Modulation; }

	void SetParam(const char* name, float value) override {
		if (strcmp(name, "dry") == 0) dry_ = Clamp(value, 0.0f, 1.0f);
		else if (strcmp(name, "up") == 0) up_ = Clamp(value, 0.0f, 1.0f);
		else if (strcmp(name, "sub") == 0) sub_ = Clamp(value, 0.0f, 1.0f);
		else if (strcmp(name, "sub2") == 0) sub2_ = Clamp(value, 0.0f, 1.0f);
		else if (strcmp(name, "mode") == 0) mode_ = Clamp(value, 0.0f, 1.0f);
	}

	float GetParam(const char* name) override {
		if (strcmp(name, "dry") == 0) return dry_;
		if (strcmp(name, "up") == 0) return up_;
		if (strcmp(name, "sub") == 0) return sub_;
		if (strcmp(name, "sub2") == 0) return sub2_;
		if (strcmp(name, "mode") == 0) return mode_;
		return 0.0f;
	}

	const char* GetParamList() const override { return "dry,up,sub,sub2,mode"; }
	int GetParamCount() const override { return 5; }
	bool GetParamInfo(int index, EffectParamInfo& info) const override {
		switch (index) {
			case 0: info = {"dry", "Sub N Up Dry", "", "float", "log", 0.0f, 1.0f, 1.0f, 0.01f}; return true;
			case 1: info = {"up", "Sub N Up Up", "", "float", "log", 0.0f, 1.0f, 0.32f, 0.01f}; return true;
			case 2: info = {"sub", "Sub N Up Sub", "", "float", "log", 0.0f, 1.0f, 0.42f, 0.01f}; return true;
			case 3: info = {"sub2", "Sub N Up Sub 2", "", "float", "log", 0.0f, 1.0f, 0.0f, 0.01f}; return true;
			case 4: info = {"mode", "Sub N Up Mode", "", "switch", "linear", 0.0f, 1.0f, 0.0f, 1.0f, "Poly,Classic"}; return true;
			default: return false;
		}
	}

private:
	float Coeff(float hz) const { return Clamp(1.0f - expf(-6.28318531f * hz / sample_rate_), 0.0001f, 0.98f); }
	void Smooth(float& current, float target) { current += smooth_coeff_ * (target - current); }
	float FlushTiny(float value) const { return fabsf(value) < 1.0e-18f ? 0.0f : value; }
	float Lerp(float a, float b, float amount) const { return a + amount * (b - a); }

	float ReadDelay(float delay) const {
		float position = static_cast<float>(write_) - delay;
		while (position < 0.0f) position += BUFFER_SIZE;
		int first = static_cast<int>(position) & (BUFFER_SIZE - 1);
		int second = (first + 1) & (BUFFER_SIZE - 1);
		float fraction = position - floorf(position);
		return buffer_[first] + fraction * (buffer_[second] - buffer_[first]);
	}

	float ReadGrains(float& phase, float ratio, float window) {
		float phase_b = phase + 0.5f;
		if (phase_b >= 1.0f) phase_b -= 1.0f;
		float weight_a = 0.5f - 0.5f * cosf(phase * 6.28318531f);
		float weight_b = 1.0f - weight_a;
		float first = ReadDelay(24.0f + phase * window);
		float second = ReadDelay(24.0f + phase_b * window);
		phase += (1.0f - ratio) / window;
		while (phase < 0.0f) phase += 1.0f;
		while (phase >= 1.0f) phase -= 1.0f;
		return first * weight_a + second * weight_b;
	}

	void UpdateTracker(float in) {
		float envelope_coeff = fabsf(in) > envelope_ ? Coeff(950.0f) : Coeff(18.0f);
		envelope_ += envelope_coeff * (fabsf(in) - envelope_);
		samples_since_crossing_++;
		if (in < -0.0015f) tracker_armed_ = true;
		bool rising_crossing = tracker_armed_ && in > 0.0015f;
		if (rising_crossing && envelope_ > 0.0025f) {
			tracker_armed_ = false;
			int candidate = samples_since_crossing_;
			float minimum = sample_rate_ / 1200.0f;
			float maximum = sample_rate_ / 45.0f;
			if (candidate >= minimum && candidate <= maximum) {
				float ratio = static_cast<float>(candidate) / period_;
				if (ratio > 0.55f && ratio < 1.82f) {
					period_ += 0.24f * (static_cast<float>(candidate) - period_);
					tracking_confidence_ += 0.22f * (1.0f - tracking_confidence_);
				}
			}
			samples_since_crossing_ = 0;
			crossing_count_++;
		}
		if (envelope_ < 0.0012f || samples_since_crossing_ > static_cast<int>(sample_rate_ * 0.08f)) {
			tracking_confidence_ *= 0.9985f;
		}
		float increment = period_ > 1.0f ? 1.0f / period_ : 0.0f;
		classic_phase_ += increment;
		if (classic_phase_ >= 4.0f) classic_phase_ -= 4.0f;
		previous_input_ = in;
	}

	float sample_rate_ = 48000.0f;
	float dry_ = 1.0f, up_ = 0.32f, sub_ = 0.42f, sub2_ = 0.0f, mode_ = 0.0f;
	float dry_z_ = 1.0f, up_z_ = 0.32f, sub_z_ = 0.42f, sub2_z_ = 0.0f, mode_z_ = 0.0f;
	float phase_up_ = 0.17f, phase_sub_ = 0.43f, phase_sub2_ = 0.71f;
	float classic_phase_ = 0.0f, period_ = 436.0f, previous_input_ = 0.0f;
	float envelope_ = 0.0f, tracking_confidence_ = 0.0f;
	float sub_lp_ = 0.0f, sub2_lp_ = 0.0f, classic_sub_lp_ = 0.0f, classic_sub2_lp_ = 0.0f;
	float smooth_coeff_ = 0.01f;
	int write_ = 0, crossing_count_ = 0, samples_since_crossing_ = 0;
	bool tracker_armed_ = false;
	float buffer_[BUFFER_SIZE] = {};
};