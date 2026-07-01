#pragma once
#include <cstddef>
#include <cstring>

// Effect category tags — used for grouping and UI.
enum class EffectCategory {
    Distortion,
    Modulation,
    Time,
    Dynamics,
    Filter,
};

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

    // Which family this effect belongs to.
    virtual EffectCategory GetCategory() const = 0;

    // Parameter access by name — override in each effect for serial control
    virtual void  SetParam(const char* name, float value) { (void)name; (void)value; }
    virtual float GetParam(const char* name) { (void)name; return 0.f; }

    // Per-effect bypass
    bool IsEnabled() const     { return enabled_; }
    void SetEnabled(bool e)    { enabled_ = e; }

    // Convenience: wraps Process() with bypass check
    float Tick(float in) {
        return enabled_ ? Process(in) : in;
    }

protected:
    bool enabled_ = true;
};
