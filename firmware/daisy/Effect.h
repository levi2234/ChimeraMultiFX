#pragma once
#include <cstddef>

// Base class for all ChimeraMultiFX effects.
// Each effect implements Init() and Process() and can be hot-swapped at runtime.
class Effect {
public:
    virtual ~Effect() {}

    // Called once when the effect is activated. sample_rate is in Hz.
    virtual void Init(float sample_rate) = 0;

    // Process a single mono sample and return the output.
    virtual float Process(float in) = 0;

    // Friendly name for display/debug purposes.
    virtual const char* GetName() const = 0;
};
