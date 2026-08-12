/*
 * Sideous Drums - rimshot: a short sine "tock" plus a short filtered noise
 * tick, both on fixed short decays (a rimshot doesn't ring on, unlike the
 * snare) - only Tune and Level are user-exposed, everything else is fixed
 * character constants, keeping this a deliberately tiny 2-knob column.
 */

#pragma once

#include <cmath>

#include "SineOscillator.hpp"
#include "Noise.hpp"
#include "Filter.hpp"
#include "ADSR.hpp"

namespace sideous {

class ClickVoice
{
public:
    static constexpr float kAttackSeconds = 0.0005f;
    static constexpr float kToneDecaySeconds = 0.035f;
    static constexpr float kNoiseDecaySeconds = 0.02f;
    static constexpr float kNoiseCutoffHz = 2500.0f;
    static constexpr float kToneMix = 0.6f; // fixed tone/noise blend

    void setSampleRate(double sampleRate) noexcept
    {
        fOsc.setSampleRate(sampleRate);
        fToneEnv.setSampleRate(sampleRate);
        fToneEnv.setAttack(kAttackSeconds);
        fToneEnv.setSustain(0.0f);
        fToneEnv.setDecay(kToneDecaySeconds);

        fNoiseFilter.setSampleRate(sampleRate);
        fNoiseFilter.setType(FilterType::Bandpass);
        fNoiseFilter.setResonance(0.25f);
        fNoiseEnv.setSampleRate(sampleRate);
        fNoiseEnv.setAttack(kAttackSeconds);
        fNoiseEnv.setSustain(0.0f);
        fNoiseEnv.setDecay(kNoiseDecaySeconds);
    }

    void setTune(float hz) noexcept { fTuneHz = hz; }

    void trigger(float velocity) noexcept
    {
        fVelocity = velocity;
        fOsc.resetPhase();
        fOsc.setFrequency(fTuneHz);
        fToneEnv.noteOn();
        fNoiseEnv.noteOn();
    }

    float process() noexcept
    {
        const float tone = fOsc.process() * fToneEnv.process();
        const float noise = fNoiseFilter.process(fNoise.process(), kNoiseCutoffHz) * fNoiseEnv.process();
        return (kToneMix * tone + (1.0f - kToneMix) * noise) * fVelocity;
    }

    // current envelope level (0..1), for UI activity indicators - the louder of the two layers
    float getLevel() const noexcept
    {
        const float tone = fToneEnv.getLevel();
        const float noise = fNoiseEnv.getLevel();
        return tone > noise ? tone : noise;
    }

private:
    SineOscillator fOsc;
    ADSR fToneEnv;

    Noise fNoise;
    Filter fNoiseFilter;
    ADSR fNoiseEnv;

    float fTuneHz = 400.0f;
    float fVelocity = 1.0f;
};

} // namespace sideous
