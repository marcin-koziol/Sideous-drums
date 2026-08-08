/*
 * Sideous Drums - handclap: the classic 808/909 trick of several fast
 * retriggered noise bursts ("flams") followed by one longer body pulse that
 * simulates the natural spread of real hands clapping together. All pulses
 * share one noise source and bandpass filter - they're just a scheduled
 * sequence of amplitude-envelope retriggers, not separate voices.
 */

#pragma once

#include <cstdint>

#include "Noise.hpp"
#include "Filter.hpp"
#include "ADSR.hpp"

namespace sideous {

class ClapVoice
{
public:
    static constexpr float kAttackSeconds = 0.0005f;
    static constexpr float kFlamDecaySeconds = 0.012f;
    static constexpr float kFlamSpacingSeconds = 0.014f;
    static constexpr uint32_t kFlamCount = 3; // short pulses before the longer body pulse
    static constexpr float kCutoffHz = 1200.0f;
    static constexpr float kResonance = 0.25f;

    void setSampleRate(double sampleRate) noexcept
    {
        fSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
        fSpacingSamples = (uint32_t)(kFlamSpacingSeconds * fSampleRate);

        fFilter.setSampleRate(sampleRate);
        fFilter.setType(FilterType::Bandpass);
        fFilter.setResonance(kResonance);

        fEnv.setSampleRate(sampleRate);
        fEnv.setAttack(kAttackSeconds);
        fEnv.setSustain(0.0f);
    }

    void setDecay(float seconds) noexcept { fBodyDecay = seconds; }

    void trigger(float velocity) noexcept
    {
        fVelocity = velocity;
        fStep = 0;
        fCountdown = 0; // fires the first flam on the very next process() call
    }

    float process() noexcept
    {
        if (fStep <= kFlamCount)
        {
            if (fCountdown == 0)
            {
                fEnv.setDecay(fStep < kFlamCount ? kFlamDecaySeconds : fBodyDecay);
                fEnv.noteOn();
                fCountdown = fSpacingSamples;
                ++fStep;
            }
            else
            {
                --fCountdown;
            }
        }

        const float filtered = fFilter.process(fNoise.process(), kCutoffHz);
        return filtered * fEnv.process() * fVelocity;
    }

private:
    double fSampleRate = 44100.0;
    uint32_t fSpacingSamples = 617; // ~14ms @ 44.1kHz, recomputed in setSampleRate

    Noise fNoise;
    Filter fFilter;
    ADSR fEnv;

    float fBodyDecay = 0.15f;
    float fVelocity = 1.0f;

    uint32_t fStep = kFlamCount + 1; // idle until first trigger()
    uint32_t fCountdown = 0;
};

} // namespace sideous
