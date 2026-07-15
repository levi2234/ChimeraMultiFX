#pragma once

#include "Effect.h"
#include "effects/distortion/TanhDistortion.h"
#include "effects/distortion/Bitcrusher.h"
#include "effects/distortion/Overdrive.h"
#include "effects/distortion/Boost.h"
#include "effects/modulation/Chorus.h"
#include "effects/modulation/Tremolo.h"
#include "effects/time/Delay.h"
#include "effects/dynamics/Compressor.h"
#include "effects/dynamics/NoiseGate.h"
#include "effects/dynamics/Sustain.h"
#include "effects/filter/LowPass.h"
#include "effects/filter/HighPass.h"
#include "effects/filter/BandPass.h"
#include "effects/filter/Notch.h"
#include "effects/filter/Equalizer.h"
#include "effects/filter/AutoWah.h"

class EffectRegistry {
public:
    static Effect* Create(const char* name, float sample_rate) {
        Effect* effect = nullptr;
        if      (strcmp(name, "distortion") == 0) effect = new TanhDistortion();
        else if (strcmp(name, "bitcrusher") == 0) effect = new Bitcrusher();
        else if (strcmp(name, "overdrive") == 0)  effect = new Overdrive();
        else if (strcmp(name, "boost") == 0)      effect = new Boost();
        else if (strcmp(name, "chorus") == 0)     effect = new Chorus();
        else if (strcmp(name, "tremolo") == 0)    effect = new Tremolo();
        else if (strcmp(name, "delay") == 0)      effect = new Delay();
        else if (strcmp(name, "compressor") == 0) effect = new Compressor();
        else if (strcmp(name, "noisegate") == 0)  effect = new NoiseGate();
        else if (strcmp(name, "sustain") == 0)    effect = new Sustain();
        else if (strcmp(name, "lowpass") == 0)    effect = new LowPass();
        else if (strcmp(name, "highpass") == 0)   effect = new HighPass();
        else if (strcmp(name, "bandpass") == 0)   effect = new BandPass();
        else if (strcmp(name, "notch") == 0)      effect = new Notch();
        else if (strcmp(name, "eq") == 0)         effect = new Equalizer();
        else if (strcmp(name, "autowah") == 0)    effect = new AutoWah();
        if (effect) effect->Init(sample_rate);
        return effect;
    }

    static const char* NamesJson() {
        return "[\"distortion\",\"bitcrusher\",\"overdrive\",\"boost\",\"chorus\",\"tremolo\",\"delay\",\"compressor\",\"noisegate\",\"sustain\",\"lowpass\",\"highpass\",\"bandpass\",\"notch\",\"eq\",\"autowah\"]";
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