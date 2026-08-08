/*
 * Sideous Drums - snare: a short pitch-enveloped sine "tone" layer (like a
 * fast mini-kick, gives the snare its drum-ness) independently mixed with a
 * highpassed noise "snap" layer (the actual snare-wire hiss - highpass
 * rather than bandpass so the Bright knob can open up into real top-end
 * shimmer instead of staying capped at a fixed, dull mid-range peak).
 * Tone Mix blends between the two layers; both have their own decay.
 */

#pragma once

#include <cmath>

#include "SineOscillator.hpp"
#include "PitchEnvelope.hpp"
#include "Noise.hpp"
#include "Filter.hpp"
#include "ADSR.hpp"

namespace sideous {

class SnareVoice
{
public:
    static constexpr float kAttackSeconds = 0.001f;
    static constexpr float kTonePitchEnvDepth = 24.0f;  // fixed, not user-exposed
    static constexpr float kTonePitchEnvDecay = 0.02f;
    static constexpr float kNoiseResonance = 0.15f;

    void setSampleRate(double sampleRate) noexcept
    {
        fOsc.setSampleRate(sampleRate);
        fPitchEnv.setSampleRate(sampleRate);
        fPitchEnv.setDepth(kTonePitchEnvDepth);
        fPitchEnv.setDecay(kTonePitchEnvDecay);
        fToneEnv.setSampleRate(sampleRate);
        fToneEnv.setAttack(kAttackSeconds);
        fToneEnv.setSustain(0.0f);
        fNoiseFilter.setSampleRate(sampleRate);
        fNoiseFilter.setType(FilterType::Highpass);
        fNoiseFilter.setResonance(kNoiseResonance);
        fNoiseEnv.setSampleRate(sampleRate);
        fNoiseEnv.setAttack(kAttackSeconds);
        fNoiseEnv.setSustain(0.0f);
    }

    void setTune(float hz) noexcept { fTuneHz = hz; }
    void setToneMix(float mix01) noexcept { fToneMix = mix01; }
    void setToneDecay(float seconds) noexcept { fToneEnv.setDecay(seconds); }
    void setSnap(float seconds) noexcept { fNoiseEnv.setDecay(seconds); }
    void setBright(float hz) noexcept { fBrightHz = hz; }

    void trigger(float velocity) noexcept
    {
        fVelocity = velocity;
        fOsc.resetPhase();
        fPitchEnv.trigger();
        fToneEnv.noteOn();
        fNoiseEnv.noteOn();
    }

    float process() noexcept
    {
        const float semitones = fPitchEnv.process();
        fOsc.setFrequency(fTuneHz * std::exp2(semitones / 12.0f));
        const float tone = fOsc.process() * fToneEnv.process();

        const float noiseRaw = fNoiseFilter.process(fNoise.process(), fBrightHz);
        const float noise = noiseRaw * fNoiseEnv.process();

        return (fToneMix * tone + (1.0f - fToneMix) * noise) * fVelocity;
    }

private:
    SineOscillator fOsc;
    PitchEnvelope fPitchEnv;
    ADSR fToneEnv;

    Noise fNoise;
    Filter fNoiseFilter;
    ADSR fNoiseEnv;

    float fTuneHz = 180.0f;
    float fToneMix = 0.5f;
    float fBrightHz = 2500.0f;
    float fVelocity = 1.0f;
};

} // namespace sideous
