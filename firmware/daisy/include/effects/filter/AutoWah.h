#pragma once
#include "Effect.h"
#include <cmath>
#include <cstring>

// Envelope-controlled wah that sweeps a resonant band-pass with pick strength.
// Separate detector attack/release times shape how quickly the pedal opens and
// closes. A topology-preserving state-variable filter is used because its
// resonance remains well behaved while cutoff moves; coefficients update at a
// low control rate to avoid an expensive tangent calculation on every sample.
class AutoWah : public Effect {
public:
    void Init(float sample_rate) override {
        sample_rate_ = sample_rate > 1000.0f ? sample_rate : 48000.0f;
        sensitivity_ = 0.65f;
        resonance_ = 0.55f;
        attack_ms_ = 8.0f;
        release_ms_ = 160.0f;
        mix_ = 1.0f;
        envelope_ = 0.0f;
        integrator_one_ = integrator_two_ = 0.0f;
        update_counter_ = 0;
        UpdateEnvelopeCoefficients();
        UpdateFilterCoefficient();
    }

    float Process(float in) override {
        const float magnitude = fabsf(in);
        const float detector_coeff = magnitude > envelope_ ? attack_coeff_ : release_coeff_;
        envelope_ = detector_coeff * envelope_ + (1.0f - detector_coeff) * magnitude;

        if (++update_counter_ >= 8) {
            update_counter_ = 0;
            UpdateFilterCoefficient();
        }

        // The TPT state update is algebraically delay-free, so changing cutoff
        // does not destabilize the two integrators. v1 is the band-pass output.
        const float v1 = filter_a1_ * (integrator_one_ + filter_g_ * (in - integrator_two_));
        const float v2 = integrator_two_ + filter_g_ * v1;
        integrator_one_ = 2.0f * v1 - integrator_one_;
        integrator_two_ = 2.0f * v2 - integrator_two_;
        const float wet = tanhf(v1 * 1.5f);
        return in * (1.0f - mix_) + wet * mix_;
    }

    const char* GetName() const override { return "autowah"; }
    EffectCategory GetCategory() const override { return EffectCategory::Filter; }

    void SetParam(const char* name, float value) override {
        if (strcmp(name, "sensitivity") == 0) { sensitivity_ = Clamp(value, 0.0f, 1.0f); UpdateFilterCoefficient(); }
        else if (strcmp(name, "resonance") == 0) { resonance_ = Clamp(value, 0.0f, 1.0f); UpdateFilterCoefficient(); }
        else if (strcmp(name, "attack") == 0) { attack_ms_ = Clamp(value, 1.0f, 100.0f); UpdateEnvelopeCoefficients(); }
        else if (strcmp(name, "release") == 0) { release_ms_ = Clamp(value, 10.0f, 1000.0f); UpdateEnvelopeCoefficients(); }
        else if (strcmp(name, "mix") == 0) mix_ = Clamp(value, 0.0f, 1.0f);
    }

    float GetParam(const char* name) override {
        if      (strcmp(name, "sensitivity") == 0) return sensitivity_;
        else if (strcmp(name, "resonance") == 0) return resonance_;
        else if (strcmp(name, "attack") == 0) return attack_ms_;
        else if (strcmp(name, "release") == 0) return release_ms_;
        else if (strcmp(name, "mix") == 0) return mix_;
        return 0.0f;
    }

    const char* GetParamList() const override { return "sensitivity,resonance,attack,release,mix"; }
    int GetParamCount() const override { return 5; }
    bool GetParamInfo(int index, EffectParamInfo& info) const override {
        switch (index) {
            case 0: info = {"sensitivity", "Sensitivity", "", "float", "linear", 0.0f, 1.0f, 0.65f, 0.01f}; return true;
            case 1: info = {"resonance", "Resonance", "", "float", "linear", 0.0f, 1.0f, 0.55f, 0.01f}; return true;
            case 2: info = {"attack", "Attack", "ms", "float", "log", 1.0f, 100.0f, 8.0f, 0.1f}; return true;
            case 3: info = {"release", "Release", "ms", "float", "log", 10.0f, 1000.0f, 160.0f, 1.0f}; return true;
            case 4: info = {"mix", "Mix", "", "float", "linear", 0.0f, 1.0f, 1.0f, 0.01f}; return true;
            default: return false;
        }
    }

private:
    void UpdateEnvelopeCoefficients() {
        attack_coeff_ = expf(-1.0f / (0.001f * attack_ms_ * sample_rate_));
        release_coeff_ = expf(-1.0f / (0.001f * release_ms_ * sample_rate_));
    }

    void UpdateFilterCoefficient() {
        const float normalized_envelope = envelope_ / (envelope_ + 0.08f);
        const float sweep = Clamp(normalized_envelope * sensitivity_ * 1.6f, 0.0f, 1.0f);
        const float cutoff = 180.0f + 4200.0f * sweep;
        filter_g_ = tanf(3.14159265f * cutoff / sample_rate_);
        const float damping = 2.0f - resonance_ * 1.8f;
        filter_a1_ = 1.0f / (1.0f + filter_g_ * (filter_g_ + damping));
    }

    float sample_rate_;
    float sensitivity_, resonance_, attack_ms_, release_ms_, mix_;
    float attack_coeff_, release_coeff_, envelope_;
    float filter_g_, filter_a1_;
    float integrator_one_, integrator_two_;
    int update_counter_;
};