/*
 * Sideous Drums - metallic FM-ish cymbal: 6 square oscillators at fixed
 * inharmonic frequency ratios summed together (the classic 808/909 hi-hat/
 * cymbal trick), through a highpass filter (Tone knob = cutoff) into an amp
 * envelope. Shared by Crash, Ride, and both Hi-Hats - they differ only in
 * decay range/defaults and highpass sweep range (set via setToneRange), not
 * engine. choke() lets the hi-hat closed/open pair form a choke group.
 */

#pragma once

#include <cmath>
#include <algorithm>

#include "Oscillator.hpp"
#include "Filter.hpp"
#include "ADSR.hpp"

namespace sideous {

class CymbalVoice
{
public:
    static constexpr float kAttackSeconds = 0.002f;
    // classic 808/909-style inharmonic partial set (Hz)
    static constexpr float kPartials[6] = { 205.3f, 304.4f, 369.6f, 522.7f, 540.0f, 800.0f };

    void setSampleRate(double sampleRate) noexcept
    {
        for (Oscillator& osc : fOscs)
        {
            osc.setSampleRate(sampleRate);
            osc.setWaveform(Waveform::Pulse);
            osc.setPulseWidth(0.5f);
        }
        for (size_t i = 0; i < 6; ++i)
            fOscs[i].setFrequency(kPartials[i]);

        fFilter.setSampleRate(sampleRate);
        fFilter.setType(FilterType::Highpass);
        fFilter.setResonance(0.15f);

        fAmpEnv.setSampleRate(sampleRate);
        fAmpEnv.setAttack(kAttackSeconds);
        fAmpEnv.setSustain(0.0f);
    }

    // Tone knob (0..1) sweeps the highpass cutoff between these two Hz bounds
    void setToneRange(float minHz, float maxHz) noexcept { fToneMinHz = minHz; fToneMaxHz = maxHz; }
    void setTone(float tone01) noexcept { fTone = std::clamp(tone01, 0.0f, 1.0f); }
    void setDecay(float seconds) noexcept { fAmpEnv.setDecay(seconds); }

    void trigger(float velocity) noexcept
    {
        fVelocity = velocity;
        fAmpEnv.noteOn();
    }

    // hi-hat choke group: closed-hat hits cut off a still-ringing open hat
    void choke() noexcept { fAmpEnv.choke(); }

    float process() noexcept
    {
        float sum = 0.0f;
        for (Oscillator& osc : fOscs)
            sum += osc.process();
        sum *= 1.0f / 6.0f;

        const float cutoff = fToneMinHz * std::pow(fToneMaxHz / fToneMinHz, fTone);
        const float filtered = fFilter.process(sum, cutoff);

        return filtered * fAmpEnv.process() * fVelocity;
    }

private:
    Oscillator fOscs[6];
    Filter fFilter;
    ADSR fAmpEnv;

    float fToneMinHz = 2500.0f;
    float fToneMaxHz = 9000.0f;
    float fTone = 0.5f;
    float fVelocity = 1.0f;
};

} // namespace sideous
