/*
 * Sideous Drums - maracas: a cluster of fast retriggered noise grains
 * (the same flam-retrigger trick ClapVoice uses for its hand claps, just
 * faster/denser) through a resonant bandpass instead of a plain highpass -
 * that's what gives it a papery "shake" character distinct from a hi-hat's
 * bright, single-burst hiss. Rattle (0..1) controls how many grains fire
 * per hit: 1 = old behavior (single smooth burst), higher = a real shaken
 * texture. Decay still controls each grain's own envelope.
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <cmath>

#include "Noise.hpp"
#include "Filter.hpp"
#include "ADSR.hpp"

namespace sideous {

class NoiseVoice
{
public:
    static constexpr float kAttackSeconds = 0.0005f;
    static constexpr float kCenterHz = 3500.0f; // bandpass center, not lowpass/highpass cutoff
    static constexpr float kResonance = 0.3f;
    static constexpr float kGrainSpacingSeconds = 0.02f;
    static constexpr uint32_t kMaxGrains = 6;

    void setSampleRate(double sampleRate) noexcept
    {
        fSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
        fSpacingSamples = (uint32_t)(kGrainSpacingSeconds * fSampleRate);

        fFilter.setSampleRate(sampleRate);
        fFilter.setType(FilterType::Bandpass);
        fFilter.setResonance(kResonance);
        fEnv.setSampleRate(sampleRate);
        fEnv.setAttack(kAttackSeconds);
        fEnv.setSustain(0.0f);
    }

    void setDecay(float seconds) noexcept { fEnv.setDecay(seconds); }

    // 0 = a single grain (old behavior), 1 = the full kMaxGrains-grain shake
    void setRattle(float rattle01) noexcept
    {
        const float r = std::clamp(rattle01, 0.0f, 1.0f);
        fGrainCount = 1 + (uint32_t)std::lround(r * (float)(kMaxGrains - 1));
    }

    void trigger(float velocity) noexcept
    {
        fVelocity = velocity;
        fGrainsRemaining = fGrainCount;
        fCountdown = 0; // fires the first grain on the very next process() call
    }

    float process() noexcept
    {
        if (fGrainsRemaining > 0)
        {
            if (fCountdown == 0)
            {
                fEnv.noteOn();
                fCountdown = fSpacingSamples;
                --fGrainsRemaining;
            }
            else
            {
                --fCountdown;
            }
        }

        const float filtered = fFilter.process(fNoise.process(), kCenterHz);
        return filtered * fEnv.process() * fVelocity;
    }

    // current envelope level (0..1), for UI activity indicators
    float getLevel() const noexcept { return fEnv.getLevel(); }

private:
    double fSampleRate = 44100.0;
    uint32_t fSpacingSamples = 882; // ~20ms @ 44.1kHz, recomputed in setSampleRate

    Noise fNoise;
    Filter fFilter;
    ADSR fEnv;
    float fVelocity = 1.0f;

    uint32_t fGrainCount = 1;
    uint32_t fGrainsRemaining = 0;
    uint32_t fCountdown = 0;
};

} // namespace sideous
