#include "../include/effects/distortion/FulltoneOCD.h"
#include "../include/effects/distortion/BossBD2.h"
#include "../include/effects/modulation/TCSubNUp.h"
#include <cassert>
#include <cmath>
#include <cstdio>

static constexpr float SAMPLE_RATE = 48000.0f;
static constexpr float PI = 3.14159265359f;

template <typename EffectType>
void CheckMetadataAndBypass(EffectType& effect) {
	for (int index = 0; index < effect.GetParamCount(); index++) {
		EffectParamInfo info;
		assert(effect.GetParamInfo(index, info));
		assert(info.min >= 0.0f && info.max <= 1.0f);
		assert(info.default_value >= info.min && info.default_value <= info.max);
	}
	effect.SetEnabled(false);
	assert(effect.Tick(0.173f) == 0.173f);
	effect.SetEnabled(true);
}

template <typename EffectType>
void CheckDrive(EffectType& effect, const char* gain_name) {
	effect.Init(SAMPLE_RATE);
	CheckMetadataAndBypass(effect);
	float peak = 0.0f;
	float mean = 0.0f;
	float previous = 0.0f;
	for (int sample = 0; sample < 96000; sample++) {
		if (sample == 24000) effect.SetParam(gain_name, 1.0f);
		if (sample == 48000) effect.SetParam("tone", 0.05f);
		float input = 0.16f * sinf(2.0f * PI * 220.0f * sample / SAMPLE_RATE);
		float output = effect.Process(input);
		assert(std::isfinite(output));
		assert(fabsf(output - previous) < 1.5f);
		peak = fmaxf(peak, fabsf(output));
		if (sample >= 72000) mean += output / 24000.0f;
		previous = output;
	}
	assert(peak < 2.0f);
	assert(fabsf(mean) < 0.002f);
}

void CheckOctaver() {
	TCSubNUp effect;
	effect.Init(SAMPLE_RATE);
	CheckMetadataAndBypass(effect);
	effect.SetParam("dry", 0.0f);
	effect.SetParam("up", 0.0f);
	effect.SetParam("sub", 0.75f);
	effect.SetParam("sub2", 0.0f);
	effect.SetParam("mode", 1.0f);
	float peak = 0.0f;
	float locked_energy = 0.0f;
	float recovered_energy = 0.0f;
	for (int sample = 0; sample < 120000; sample++) {
		bool silence = sample >= 50000 && sample < 76000;
		float input = silence ? 0.0f : 0.11f * sinf(2.0f * PI * 110.0f * sample / SAMPLE_RATE);
		float output = effect.Process(input);
		assert(std::isfinite(output));
		peak = fmaxf(peak, fabsf(output));
		if (sample >= 30000 && sample < 48000) locked_energy += output * output;
		if (sample >= 92000 && sample < 110000) recovered_energy += output * output;
	}
	assert(peak < 2.0f);
	assert(locked_energy > 1.0f);
	assert(recovered_energy > 1.0f);
}

int main() {
	FulltoneOCD ocd;
	BossBD2 bd2;
	CheckDrive(ocd, "drive");
	CheckDrive(bd2, "gain");
	CheckOctaver();
	std::puts("DSP model smoke tests passed");
	return 0;
}