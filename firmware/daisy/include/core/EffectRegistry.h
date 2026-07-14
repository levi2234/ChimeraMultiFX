#pragma once

#include "../../Effect.h"
#include "../../effects/distortion/TanhDistortion.h"
#include "../../effects/distortion/Bitcrusher.h"
#include "../../effects/distortion/overdrive.h"
#include "../../effects/modulation/Chorus.h"
#include "../../effects/modulation/Tremolo.h"
#include "../../effects/time/Delay.h"
#include "../../effects/dynamics/Compressor.h"
#include "../../effects/filter/LowPass.h"

class EffectRegistry {
public:
    static Effect* Create(const char* name, float sample_rate) {
        Effect* effect = nullptr;
        if      (strcmp(name, "distortion") == 0) effect = new TanhDistortion();
        else if (strcmp(name, "bitcrusher") == 0) effect = new Bitcrusher();
        else if (strcmp(name, "overdrive") == 0)  effect = new Overdrive();
        else if (strcmp(name, "chorus") == 0)     effect = new Chorus();
        else if (strcmp(name, "tremolo") == 0)    effect = new Tremolo();
        else if (strcmp(name, "delay") == 0)      effect = new Delay();
        else if (strcmp(name, "compressor") == 0) effect = new Compressor();
        else if (strcmp(name, "lowpass") == 0)    effect = new LowPass();
        if (effect) effect->Init(sample_rate);
        return effect;
    }

    static const char* NamesJson() {
        return "[\"distortion\",\"bitcrusher\",\"overdrive\",\"chorus\",\"tremolo\",\"delay\",\"compressor\",\"lowpass\"]";
    }

    static const char* CategoryName(EffectCategory category) {
        switch (category) {
            case EffectCategory::Distortion: return "distortion";
            case EffectCategory::Modulation: return "modulation";
            case EffectCategory::Time:       return "time";
            case EffectCategory::Dynamics:   return "dynamics";
            case EffectCategory::Filter:     return "filter";
            default: return "unknown";
        }
    }
};