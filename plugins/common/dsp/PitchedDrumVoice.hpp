/*
 * Sideous Drums - sine + pitch-envelope + amp envelope drum voice: the
 * classic 808/909 kick/tom engine (same engine, different pitch range and
 * knob set, per sideous-drums-plan.md). One-shot: trigger() fires an
 * attack->decay->silence sweep, no note-off/sustain/release involved -
 * these are fixed drum voices, always processed, never stolen/pooled.
 */

#pragma once

#include <cmath>

#include "SineOscillator.hpp"
#include "PitchEnvelope.hpp"
#include "ADSR.hpp"
#include "Distortion.hpp"

namespace sideous {

class PitchedDrumVoice
{
public:
    static constexpr float kAttackSeconds = 0.001f; // effectively instant, not user-exposed

    void setSampleRate(double sampleRate) noexcept
    {
        fOsc.setSampleRate(sampleRate);
        fPitchEnv.setSampleRate(sampleRate);
        fAmpEnv.setSampleRate(sampleRate);
        fAmpEnv.setAttack(kAttackSeconds);
        fAmpEnv.setSustain(0.0f);
    }

    void setTune(float hz) noexcept { fTuneHz = hz; }
    void setPitchEnvDepth(float semitones) noexcept { fPitchEnv.setDepth(semitones); }
    void setPitchEnvDecay(float seconds) noexcept { fPitchEnv.setDecay(seconds); }
    void setDecay(float seconds) noexcept { fAmpEnv.setDecay(seconds); }
    void setDrive(float drive01) noexcept { fDrive.setDrive(drive01); fDrive.setMix(1.0f); }

    void trigger(float velocity) noexcept
    {
        fVelocity = velocity;
        fOsc.resetPhase();
        fPitchEnv.trigger();
        fAmpEnv.noteOn();
    }

    float process() noexcept
    {
        const float semitones = fPitchEnv.process();
        fOsc.setFrequency(fTuneHz * std::exp2(semitones / 12.0f));
        const float amp = fAmpEnv.process();
        return fDrive.process(fOsc.process() * amp * fVelocity);
    }

    // current envelope level (0..1), for UI activity indicators
    float getLevel() const noexcept { return fAmpEnv.getLevel(); }

private:
    SineOscillator fOsc;
    PitchEnvelope fPitchEnv;
    ADSR fAmpEnv;
    Distortion fDrive;
    float fTuneHz = 55.0f;
    float fVelocity = 1.0f;
};

} // namespace sideous
