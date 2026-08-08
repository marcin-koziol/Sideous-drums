/*
 * Sideous Drums - maracas: a bright highpass-filtered noise burst, the
 * simplest voice in the kit - only Decay and Level are exposed.
 */

#pragma once

#include "Noise.hpp"
#include "Filter.hpp"
#include "ADSR.hpp"

namespace sideous {

class NoiseVoice
{
public:
    static constexpr float kAttackSeconds = 0.0005f;
    static constexpr float kCutoffHz = 4000.0f;

    void setSampleRate(double sampleRate) noexcept
    {
        fFilter.setSampleRate(sampleRate);
        fFilter.setType(FilterType::Highpass);
        fFilter.setResonance(0.1f);
        fEnv.setSampleRate(sampleRate);
        fEnv.setAttack(kAttackSeconds);
        fEnv.setSustain(0.0f);
    }

    void setDecay(float seconds) noexcept { fEnv.setDecay(seconds); }

    void trigger(float velocity) noexcept
    {
        fVelocity = velocity;
        fEnv.noteOn();
    }

    float process() noexcept
    {
        const float filtered = fFilter.process(fNoise.process(), kCutoffHz);
        return filtered * fEnv.process() * fVelocity;
    }

private:
    Noise fNoise;
    Filter fFilter;
    ADSR fEnv;
    float fVelocity = 1.0f;
};

} // namespace sideous
