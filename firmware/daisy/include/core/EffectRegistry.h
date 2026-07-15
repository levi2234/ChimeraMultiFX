#pragma once

#include "Effect.h"
#include "effects/distortion/TanhDistortion.h"
#include "effects/distortion/Bitcrusher.h"
#include "effects/distortion/Overdrive.h"
#include "effects/distortion/Boost.h"
#include "effects/distortion/TubeScreamer.h"
#include "effects/distortion/KlonCentaur.h"
#include "effects/distortion/ProCoRat.h"
#include "effects/distortion/BigMuffPi.h"
#include "effects/modulation/Chorus.h"
#include "effects/modulation/Tremolo.h"
#include "effects/modulation/BossCE2.h"
#include "effects/modulation/MXRPhase90.h"
#include "effects/modulation/BossBF2.h"
#include "effects/time/Delay.h"
#include "effects/time/DeluxeMemoryMan.h"
#include "effects/time/StrymonBlueSky.h"
#include "effects/dynamics/Compressor.h"
#include "effects/dynamics/NoiseGate.h"
#include "effects/dynamics/Sustain.h"
#include "effects/filter/LowPass.h"
#include "effects/filter/HighPass.h"
#include "effects/filter/BandPass.h"
#include "effects/filter/Notch.h"
#include "effects/filter/Equalizer.h"
#include "effects/filter/AutoWah.h"
#include "effects/filter/CryBabyMutron.h"

class EffectRegistry {
private:
    using Factory = Effect* (*)();

    struct Descriptor {
        const char* name;
        EffectCategory category;
        Factory factory;
    };

    template <typename EffectType>
    static Effect* Instantiate() {
        return new EffectType();
    }

    static const Descriptor* Descriptors(int& count) {
        static const Descriptor descriptors[] = {
            {"distortion", EffectCategory::Distortion, &Instantiate<TanhDistortion>},
            {"bitcrusher", EffectCategory::Distortion, &Instantiate<Bitcrusher>},
            {"overdrive", EffectCategory::Distortion, &Instantiate<Overdrive>},
            {"boost", EffectCategory::Distortion, &Instantiate<Boost>},
            {"TubeScreamer", EffectCategory::Distortion, &Instantiate<TubeScreamer>},
            {"KlonCentaur", EffectCategory::Distortion, &Instantiate<KlonCentaur>},
            {"ProcoRAT", EffectCategory::Distortion, &Instantiate<ProCoRat>},
            {"BigMuffPi", EffectCategory::Distortion, &Instantiate<BigMuffPi>},
            {"chorus", EffectCategory::Modulation, &Instantiate<Chorus>},
            {"tremolo", EffectCategory::Modulation, &Instantiate<Tremolo>},
            {"boss_ce2", EffectCategory::Modulation, &Instantiate<BossCE2>},
            {"mxr_phase90", EffectCategory::Modulation, &Instantiate<MXRPhase90>},
            {"boss_bf2", EffectCategory::Modulation, &Instantiate<BossBF2>},
            {"delay", EffectCategory::Time, &Instantiate<Delay>},
            {"DeluxeMemoryMan", EffectCategory::Time, &Instantiate<DeluxeMemoryMan>},
            {"StrymonBluesky", EffectCategory::Time, &Instantiate<StrymonBlueSky>},
            {"compressor", EffectCategory::Dynamics, &Instantiate<Compressor>},
            {"noisegate", EffectCategory::Dynamics, &Instantiate<NoiseGate>},
            {"sustain", EffectCategory::Dynamics, &Instantiate<Sustain>},
            {"lowpass", EffectCategory::Filter, &Instantiate<LowPass>},
            {"highpass", EffectCategory::Filter, &Instantiate<HighPass>},
            {"bandpass", EffectCategory::Filter, &Instantiate<BandPass>},
            {"notch", EffectCategory::Filter, &Instantiate<Notch>},
            {"eq", EffectCategory::Filter, &Instantiate<Equalizer>},
            {"autowah", EffectCategory::Filter, &Instantiate<AutoWah>},
            {"CryBabyMutron", EffectCategory::Filter, &Instantiate<CryBabyMutron>},
        };
        count = sizeof(descriptors) / sizeof(descriptors[0]);
        return descriptors;
    }

public:
    static Effect* Create(const char* name, float sample_rate) {
        int count = 0;
        const Descriptor* descriptors = Descriptors(count);
        for (int index = 0; index < count; index++) {
            if (strcmp(name, descriptors[index].name) == 0) {
                Effect* effect = descriptors[index].factory();
                effect->Init(sample_rate);
                return effect;
            }
        }
        return nullptr;
    }

    static int Count() {
        int count = 0;
        Descriptors(count);
        return count;
    }

    static const char* NameAt(int index) {
        int count = 0;
        const Descriptor* descriptors = Descriptors(count);
        return index >= 0 && index < count ? descriptors[index].name : "";
    }

    static EffectCategory CategoryAt(int index) {
        int count = 0;
        const Descriptor* descriptors = Descriptors(count);
        return index >= 0 && index < count ? descriptors[index].category : EffectCategory::Distortion;
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