/*
 * Sideous Drums - handclap: the classic 808/909 trick of several fast
 * retriggered noise bursts ("flams") followed by one longer body pulse that
 * simulates the natural spread of real hands clapping together. All pulses
 * share one noise source and bandpass filter - they're just a scheduled
 * sequence of amplitude-envelope retriggers, not separate voices.
 *
 * Hands (1..kMaxHands) sets how many pulses fire per hit: the last one is
 * always the longer "body" pulse, everything before it is a short flam -
 * so Hands=1 is a single clean hit with no flam at all, higher counts spread
 * it out into more of a "crowd" clap. Tone sweeps the shared bandpass
 * center frequency.
 */

#pragma once

#include <algorithm>
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
    static constexpr float kResonance = 0.25f;
    static constexpr uint32_t kMaxHands = 7;

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
    void setTone(float hz) noexcept { fCutoffHz = hz; }

    // 1 = a single clean hit (no flam), up to kMaxHands = a full "crowd" clap
    void setHands(float hands) noexcept
    {
        fHandCount = (uint32_t)std::lround(std::clamp(hands, 1.0f, (float)kMaxHands));
    }

    // see CymbalVoice::setNoiseSeed - every Noise defaults to the same seed
    void setNoiseSeed(uint32_t seed) noexcept { fNoise.setSeed(seed); }

    void trigger(float velocity) noexcept
    {
        fVelocity = velocity;
        fPulsesRemaining = fHandCount; // snapshot Hands now - later knob turns
                                        // must not affect a hit already in flight
        fCountdown = 0; // fires the first pulse on the very next process() call
    }

    float process() noexcept
    {
        if (fPulsesRemaining > 0)
        {
            if (fCountdown == 0)
            {
                const bool isBody = fPulsesRemaining == 1; // last one = the body pulse
                fEnv.setDecay(isBody ? fBodyDecay : kFlamDecaySeconds);
                fEnv.noteOn();
                fCountdown = fSpacingSamples;
                --fPulsesRemaining;
            }
            else
            {
                --fCountdown;
            }
        }

        const float filtered = fFilter.process(fNoise.process(), fCutoffHz);
        return filtered * fEnv.process() * fVelocity;
    }

    // current envelope level (0..1), for UI activity indicators
    float getLevel() const noexcept { return fEnv.getLevel(); }

private:
    double fSampleRate = 44100.0;
    uint32_t fSpacingSamples = 617; // ~14ms @ 44.1kHz, recomputed in setSampleRate

    Noise fNoise;
    Filter fFilter;
    ADSR fEnv;

    float fBodyDecay = 0.15f;
    float fCutoffHz = 1200.0f;
    float fVelocity = 1.0f;

    uint32_t fHandCount = 4; // 3 flams + 1 body, matches the old fixed kFlamCount=3
    uint32_t fPulsesRemaining = 0; // idle until first trigger(); counts down, never
                                    // compared against the live (knob-mutable) fHandCount
    uint32_t fCountdown = 0;
};

} // namespace sideous
