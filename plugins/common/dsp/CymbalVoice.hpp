/*
 * Sideous Drums - metallic FM-ish cymbal: 6 square oscillators at fixed
 * inharmonic frequency ratios summed together (the classic 808/909 hi-hat/
 * cymbal trick), blended against plain white noise via the Metal knob (1 =
 * fully metallic/buzzy FM stack, 0 = the "chip-simple" filtered-noise
 * alternative from the plan doc), through a highpass filter (Tone knob =
 * cutoff) into an amp envelope. Shared by Crash, Ride, and both Hi-Hats -
 * they differ only in decay range/defaults, highpass sweep range (set via
 * setToneRange) and whether Metal is user-exposed (only the hi-hats expose
 * it; crash/ride stay at the default fMetal=1, fully metallic). choke()
 * lets the hi-hat closed/open pair form a choke group.
 */

#pragma once

#include <cmath>
#include <algorithm>

#include "Oscillator.hpp"
#include "Noise.hpp"
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
    // 1 = fully metallic FM stack (buzzy/jangly), 0 = plain filtered noise
    void setMetal(float metal01) noexcept { fMetal = std::clamp(metal01, 0.0f, 1.0f); }

    void trigger(float velocity) noexcept
    {
        fVelocity = velocity;
        fAmpEnv.noteOn();
    }

    // hi-hat choke group: closed-hat hits cut off a still-ringing open hat
    void choke() noexcept { fAmpEnv.choke(); }

    float process() noexcept
    {
        float fmSum = 0.0f;
        for (Oscillator& osc : fOscs)
            fmSum += osc.process();
        fmSum *= 1.0f / 6.0f;

        // blend the raw sources before filtering (not two separate filtered
        // signals mixed after) so there's only one filter call per sample
        // and the blend itself stays cheap
        const float raw = fMetal * fmSum + (1.0f - fMetal) * fNoise.process();

        const float cutoff = fToneMinHz * std::pow(fToneMaxHz / fToneMinHz, fTone);
        const float filtered = fFilter.process(raw, cutoff);

        return filtered * fAmpEnv.process() * fVelocity;
    }

private:
    Oscillator fOscs[6];
    Noise fNoise;
    Filter fFilter;
    ADSR fAmpEnv;

    float fToneMinHz = 2500.0f;
    float fToneMaxHz = 9000.0f;
    float fTone = 0.5f;
    float fMetal = 1.0f;
    float fVelocity = 1.0f;
};

} // namespace sideous
