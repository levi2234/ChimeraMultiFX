#pragma once
#include "daisy_seed.h"
#include "Effect.h"
#include <cmath>
#include <cstring>

// Original eight-line FDN reverb voiced around the audible behavior of the
// Hall of Fame 2 modes. The proprietary TC algorithms are not replicated.
class TCHallOfFame2 : public Effect {
public:
	static constexpr int LINE_SIZE = 8192;
	static constexpr int PRE_SIZE = 8192;
	static constexpr int SHIMMER_SIZE = 2048;

	void Init(float sample_rate) override {
		sample_rate_ = sample_rate > 1000.0f ? sample_rate : 48000.0f;
		decay_ = 0.48f;
		tone_ = 0.48f;
		level_ = 0.28f;
		predelay_ = 0.16f;
		mode_ = 1.0f / 7.0f;
		mash_ = 0.0f;
		decay_z_ = decay_;
		tone_z_ = tone_;
		level_z_ = level_;
		predelay_z_ = predelay_;
		mash_z_ = mash_;
		active_mode_ = target_mode_ = ModeIndex(mode_);
		mode_crossfade_ = 1.0f;
		pre_write_ = shimmer_write_ = 0;
		shimmer_phase_ = 0.37f;
		lfo_phase_ = density_state_ = lofi_state_ = spring_lp_ = spring_bp1_ = spring_bp2_ = 0.0f;
		input_envelope_ = previous_envelope_ = 0.0f;
		smooth_coeff_ = Coeff(24.0f);
		for (int line = 0; line < 8; line++) {
			write_[line] = 0;
			damp_[line] = 0.0f;
			for (int index = 0; index < LINE_SIZE; index++) tank_[line][index] = 0.0f;
		}
		for (int index = 0; index < PRE_SIZE; index++) pre_buffer_[index] = 0.0f;
		for (int index = 0; index < SHIMMER_SIZE; index++) shimmer_buffer_[index] = 0.0f;
	}

	float Process(float in) override {
		Smooth(decay_z_, decay_);
		Smooth(tone_z_, tone_);
		Smooth(level_z_, level_);
		Smooth(predelay_z_, predelay_);
		Smooth(mash_z_, mash_);
		UpdateModeTransition();

		pre_buffer_[pre_write_] = FlushTiny(in);
		float predelay_samples = (0.002f + 0.118f * predelay_z_ * predelay_z_) * sample_rate_;
		float predelayed = ReadPre(predelay_samples);
		float early = EarlyReflections(predelay_samples, active_mode_);
		if (mode_crossfade_ < 1.0f) {
			early = Lerp(early, EarlyReflections(predelay_samples, target_mode_), mode_crossfade_);
		}

		float lfo = sinf(lfo_phase_ * 6.28318531f);
		float current[8];
		for (int line = 0; line < 8; line++) {
			float delay_a = DelaySeconds(active_mode_, line) * sample_rate_;
			float depth_a = ModDepth(active_mode_, mash_z_);
			delay_a += lfo * depth_a * sample_rate_ * ((line & 1) ? -1.0f : 1.0f);
			current[line] = ReadLine(line, delay_a);
			if (mode_crossfade_ < 1.0f) {
				float delay_b = DelaySeconds(target_mode_, line) * sample_rate_;
				float depth_b = ModDepth(target_mode_, mash_z_);
				delay_b += lfo * depth_b * sample_rate_ * ((line & 1) ? -1.0f : 1.0f);
				current[line] = Lerp(current[line], ReadLine(line, delay_b), mode_crossfade_);
			}
		}

		float wet = (current[0] - current[1] + current[2] + current[3] - current[4] + current[5] - current[6] + current[7]) * 0.125f;
		shimmer_buffer_[shimmer_write_] = wet;
		float shimmer = ReadShimmer();
		shimmer_write_ = (shimmer_write_ + 1) & (SHIMMER_SIZE - 1);
		float shimmer_amount = active_mode_ == 5 ? 0.20f + 0.34f * mash_z_ : 0.0f;
		if (mode_crossfade_ < 1.0f && target_mode_ == 5) shimmer_amount = Lerp(shimmer_amount, 0.20f + 0.34f * mash_z_, mode_crossfade_);

		float matrix[8];
		Hadamard(current, matrix);
		float feedback = FeedbackGain(active_mode_, decay_z_, mash_z_);
		float damping_hz = DampingHz(active_mode_, tone_z_, mash_z_);
		if (mode_crossfade_ < 1.0f) {
			feedback = Lerp(feedback, FeedbackGain(target_mode_, decay_z_, mash_z_), mode_crossfade_);
			damping_hz = Lerp(damping_hz, DampingHz(target_mode_, tone_z_, mash_z_), mode_crossfade_);
		}
		float damping_coeff = Coeff(damping_hz);
		float diffusion_hz = active_mode_ == 0 ? 2600.0f : 5100.0f;
		density_state_ += Coeff(diffusion_hz) * ((predelayed + early * 0.55f) - density_state_);
		float injection = predelayed * 0.56f + early * 0.44f + density_state_ * 0.16f + shimmer * shimmer_amount;

		for (int line = 0; line < 8; line++) {
			damp_[line] += damping_coeff * (matrix[line] - damp_[line]);
			float polarity = (line == 1 || line == 2 || line == 4 || line == 7) ? -1.0f : 1.0f;
			float write_value = damp_[line] * feedback + injection * polarity * 0.31f;
			tank_[line][write_[line]] = FlushTiny(SoftLimit(write_value));
			write_[line] = (write_[line] + 1) & (LINE_SIZE - 1);
		}

		int audible_mode = mode_crossfade_ < 0.5f ? active_mode_ : target_mode_;
		if (audible_mode == 2) wet = SpringVoice(wet, in, mash_z_);
		if (audible_mode == 7) wet = LoFiVoice(wet, mash_z_);
		if (audible_mode == 5) wet += shimmer * (0.12f + 0.18f * mash_z_);

		float rate = audible_mode == 6 ? 0.22f + 0.28f * mash_z_ : 0.11f;
		lfo_phase_ += rate / sample_rate_;
		if (lfo_phase_ >= 1.0f) lfo_phase_ -= 1.0f;
		pre_write_ = (pre_write_ + 1) & (PRE_SIZE - 1);
		float wet_gain = 1.45f * level_z_ * level_z_;
		return FlushTiny(in + wet * wet_gain);
	}

	const char* GetName() const override { return "TCHallOfFame2"; }
	EffectCategory GetCategory() const override { return EffectCategory::Time; }

	void SetParam(const char* name, float value) override {
		if (strcmp(name, "decay") == 0) decay_ = Clamp(value, 0.0f, 1.0f);
		else if (strcmp(name, "tone") == 0) tone_ = Clamp(value, 0.0f, 1.0f);
		else if (strcmp(name, "level") == 0) level_ = Clamp(value, 0.0f, 1.0f);
		else if (strcmp(name, "predelay") == 0) predelay_ = Clamp(value, 0.0f, 1.0f);
		else if (strcmp(name, "mode") == 0) mode_ = Clamp(value, 0.0f, 1.0f);
		else if (strcmp(name, "mash") == 0) mash_ = Clamp(value, 0.0f, 1.0f);
	}

	float GetParam(const char* name) override {
		if (strcmp(name, "decay") == 0) return decay_;
		if (strcmp(name, "tone") == 0) return tone_;
		if (strcmp(name, "level") == 0) return level_;
		if (strcmp(name, "predelay") == 0) return predelay_;
		if (strcmp(name, "mode") == 0) return mode_;
		if (strcmp(name, "mash") == 0) return mash_;
		return 0.0f;
	}

	const char* GetParamList() const override { return "decay,tone,level,predelay,mode,mash"; }
	int GetParamCount() const override { return 6; }
	bool GetParamInfo(int index, EffectParamInfo& info) const override {
		switch (index) {
			case 0: info = {"decay", "Hall of Fame 2 Decay", "", "float", "log", 0.0f, 1.0f, 0.48f, 0.01f}; return true;
			case 1: info = {"tone", "Hall of Fame 2 Tone", "", "float", "linear", 0.0f, 1.0f, 0.48f, 0.01f}; return true;
			case 2: info = {"level", "Hall of Fame 2 Level", "", "float", "log", 0.0f, 1.0f, 0.28f, 0.01f}; return true;
			case 3: info = {"predelay", "Hall of Fame 2 Pre-Delay", "", "float", "log", 0.0f, 1.0f, 0.16f, 0.01f}; return true;
			case 4: info = {"mode", "Hall of Fame 2 Mode", "", "switch", "linear", 0.0f, 1.0f, 0.142857f, 0.142857f, "Room,Hall,Spring,Plate,Church,Shimmer,Mod,LoFi"}; return true;
			case 5: info = {"mash", "Hall of Fame 2 MASH", "", "float", "linear", 0.0f, 1.0f, 0.0f, 0.01f}; return true;
			default: return false;
		}
	}

private:
	float Coeff(float hz) const { return Clamp(1.0f - expf(-6.28318531f * hz / sample_rate_), 0.00001f, 0.98f); }
	void Smooth(float& current, float target) { current += smooth_coeff_ * (target - current); }
	float Lerp(float a, float b, float amount) const { return a + amount * (b - a); }
	float FlushTiny(float value) const { return fabsf(value) < 1.0e-18f ? 0.0f : value; }
	float SoftLimit(float value) const { return value / (1.0f + 0.16f * fabsf(value)); }
	int ModeIndex(float value) const { return ClampInt(static_cast<int>(value * 7.0f + 0.5f), 0, 7); }

	void UpdateModeTransition() {
		int requested = ModeIndex(mode_);
		if (requested != target_mode_) {
			if (mode_crossfade_ >= 1.0f) active_mode_ = target_mode_;
			target_mode_ = requested;
			mode_crossfade_ = 0.0f;
		}
		if (mode_crossfade_ < 1.0f) {
			mode_crossfade_ += 1.0f / (sample_rate_ * 0.055f);
			if (mode_crossfade_ >= 1.0f) {
				mode_crossfade_ = 1.0f;
				active_mode_ = target_mode_;
			}
		}
	}

	float DelaySeconds(int mode, int line) const {
		static const float times[8][8] = {
			{0.0197f, 0.0241f, 0.0299f, 0.0353f, 0.0419f, 0.0473f, 0.0539f, 0.0611f},
			{0.0411f, 0.0533f, 0.0617f, 0.0719f, 0.0839f, 0.0971f, 0.1097f, 0.1277f},
			{0.0211f, 0.0277f, 0.0331f, 0.0397f, 0.0479f, 0.0563f, 0.0671f, 0.0799f},
			{0.0283f, 0.0359f, 0.0437f, 0.0521f, 0.0613f, 0.0719f, 0.0833f, 0.0979f},
			{0.0571f, 0.0719f, 0.0863f, 0.1019f, 0.1163f, 0.1319f, 0.1451f, 0.1583f},
			{0.0397f, 0.0503f, 0.0611f, 0.0733f, 0.0869f, 0.1013f, 0.1171f, 0.1361f},
			{0.0379f, 0.0491f, 0.0593f, 0.0713f, 0.0833f, 0.0961f, 0.1117f, 0.1291f},
			{0.0233f, 0.0301f, 0.0371f, 0.0449f, 0.0533f, 0.0629f, 0.0731f, 0.0851f},
		};
		return times[mode][line];
	}

	float FeedbackGain(int mode, float decay, float mash) const {
		static const float minimum[8] = {0.34f, 0.53f, 0.46f, 0.50f, 0.67f, 0.55f, 0.52f, 0.42f};
		static const float span[8] = {0.46f, 0.40f, 0.39f, 0.42f, 0.285f, 0.34f, 0.41f, 0.43f};
		float macro = (mode == 1 || mode == 4) ? mash * 0.035f : 0.0f;
		return Clamp(minimum[mode] + span[mode] * decay * decay + macro, 0.20f, mode == 4 ? 0.968f : 0.945f);
	}

	float DampingHz(int mode, float tone, float mash) const {
		static const float base[8] = {1250.0f, 1050.0f, 1450.0f, 2100.0f, 720.0f, 1250.0f, 1250.0f, 520.0f};
		static const float range[8] = {6800.0f, 7400.0f, 5100.0f, 11200.0f, 5600.0f, 7600.0f, 8200.0f, 3100.0f};
		float brightness = tone;
		if (mode == 3) brightness = Clamp(brightness + mash * 0.20f, 0.0f, 1.0f);
		return base[mode] + range[mode] * brightness * brightness;
	}

	float ModDepth(int mode, float mash) const {
		if (mode == 6) return 0.00045f + 0.00135f * mash;
		if (mode == 1 || mode == 4 || mode == 5) return 0.00018f;
		return 0.0f;
	}

	float ReadPre(float delay) const {
		delay = Clamp(delay, 1.0f, static_cast<float>(PRE_SIZE - 2));
		float position = static_cast<float>(pre_write_) - delay;
		while (position < 0.0f) position += PRE_SIZE;
		int first = static_cast<int>(position) & (PRE_SIZE - 1);
		int second = (first + 1) & (PRE_SIZE - 1);
		float fraction = position - floorf(position);
		return pre_buffer_[first] + fraction * (pre_buffer_[second] - pre_buffer_[first]);
	}

	float EarlyReflections(float predelay, int mode) const {
		float spacing = mode == 0 ? 0.0028f : (mode == 4 ? 0.0075f : 0.0047f);
		float first = ReadPre(predelay + spacing * sample_rate_);
		float second = ReadPre(predelay + spacing * 2.37f * sample_rate_);
		float third = ReadPre(predelay + spacing * 4.91f * sample_rate_);
		float gain = mode == 0 ? 0.74f : (mode == 3 ? 0.42f : 0.55f);
		return (first * 0.52f - second * 0.31f + third * 0.21f) * gain;
	}

	float ReadLine(int line, float delay) const {
		delay = Clamp(delay, 2.0f, static_cast<float>(LINE_SIZE - 2));
		float position = static_cast<float>(write_[line]) - delay;
		while (position < 0.0f) position += LINE_SIZE;
		int first = static_cast<int>(position) & (LINE_SIZE - 1);
		int second = (first + 1) & (LINE_SIZE - 1);
		float fraction = position - floorf(position);
		return tank_[line][first] + fraction * (tank_[line][second] - tank_[line][first]);
	}

	void Hadamard(const float* input, float* output) const {
		float a0 = input[0] + input[1], a1 = input[0] - input[1];
		float a2 = input[2] + input[3], a3 = input[2] - input[3];
		float a4 = input[4] + input[5], a5 = input[4] - input[5];
		float a6 = input[6] + input[7], a7 = input[6] - input[7];
		float b0 = a0 + a2, b1 = a1 + a3, b2 = a0 - a2, b3 = a1 - a3;
		float b4 = a4 + a6, b5 = a5 + a7, b6 = a4 - a6, b7 = a5 - a7;
		const float scale = 0.35355339f;
		output[0] = (b0 + b4) * scale; output[1] = (b1 + b5) * scale;
		output[2] = (b2 + b6) * scale; output[3] = (b3 + b7) * scale;
		output[4] = (b0 - b4) * scale; output[5] = (b1 - b5) * scale;
		output[6] = (b2 - b6) * scale; output[7] = (b3 - b7) * scale;
	}

	float ReadShimmerDelay(float delay) const {
		float position = static_cast<float>(shimmer_write_) - delay;
		while (position < 0.0f) position += SHIMMER_SIZE;
		int first = static_cast<int>(position) & (SHIMMER_SIZE - 1);
		int second = (first + 1) & (SHIMMER_SIZE - 1);
		float fraction = position - floorf(position);
		return shimmer_buffer_[first] + fraction * (shimmer_buffer_[second] - shimmer_buffer_[first]);
	}

	float ReadShimmer() {
		float window = Clamp(sample_rate_ * 0.024f, 720.0f, 1500.0f);
		float second_phase = shimmer_phase_ + 0.5f;
		if (second_phase >= 1.0f) second_phase -= 1.0f;
		float weight = 0.5f - 0.5f * cosf(shimmer_phase_ * 6.28318531f);
		float output = ReadShimmerDelay(24.0f + shimmer_phase_ * window) * weight;
		output += ReadShimmerDelay(24.0f + second_phase * window) * (1.0f - weight);
		shimmer_phase_ -= 1.0f / window;
		if (shimmer_phase_ < 0.0f) shimmer_phase_ += 1.0f;
		return output;
	}

	float SpringVoice(float wet, float input, float mash) {
		float envelope_coeff = fabsf(input) > input_envelope_ ? Coeff(2200.0f) : Coeff(35.0f);
		input_envelope_ += envelope_coeff * (fabsf(input) - input_envelope_);
		float transient = Clamp((input_envelope_ - previous_envelope_) * 110.0f, 0.0f, 1.0f);
		previous_envelope_ = input_envelope_;
		spring_lp_ += Coeff(780.0f) * (wet - spring_lp_);
		spring_bp1_ += Coeff(1850.0f) * ((wet - spring_lp_) - spring_bp1_);
		spring_bp2_ += Coeff(3150.0f) * ((wet - spring_lp_) - spring_bp2_);
		float drip = (spring_bp1_ - spring_bp2_ * 0.62f) * (0.34f + 0.52f * mash) * (1.0f + transient);
		return wet * 0.72f + drip;
	}

	float LoFiVoice(float wet, float mash) {
		float cutoff = 3600.0f - 2300.0f * mash;
		lofi_state_ += Coeff(cutoff) * (wet - lofi_state_);
		float steps = 96.0f - 72.0f * mash;
		return floorf(lofi_state_ * steps + 0.5f) / steps;
	}

	float sample_rate_ = 48000.0f;
	float decay_ = 0.48f, tone_ = 0.48f, level_ = 0.28f, predelay_ = 0.16f, mode_ = 0.142857f, mash_ = 0.0f;
	float decay_z_ = 0.48f, tone_z_ = 0.48f, level_z_ = 0.28f, predelay_z_ = 0.16f, mash_z_ = 0.0f;
	float smooth_coeff_ = 0.01f, mode_crossfade_ = 1.0f, shimmer_phase_ = 0.37f, lfo_phase_ = 0.0f;
	float density_state_ = 0.0f, lofi_state_ = 0.0f, spring_lp_ = 0.0f, spring_bp1_ = 0.0f, spring_bp2_ = 0.0f;
	float input_envelope_ = 0.0f, previous_envelope_ = 0.0f;
	float damp_[8] = {};
	int write_[8] = {}, pre_write_ = 0, shimmer_write_ = 0, active_mode_ = 1, target_mode_ = 1;
	static inline float DSY_SDRAM_BSS tank_[8][LINE_SIZE];
	static inline float DSY_SDRAM_BSS pre_buffer_[PRE_SIZE];
	static inline float DSY_SDRAM_BSS shimmer_buffer_[SHIMMER_SIZE];
};