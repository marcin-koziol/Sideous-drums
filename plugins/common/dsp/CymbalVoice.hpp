/*
 * Sideous Drums - metallic FM-ish cymbal: 6 square oscillators at fixed
 * inharmonic frequency ratios summed together (the classic 808/909 hi-hat/
 * cymbal trick), blended against plain white noise via the Metal knob (1 =
 * fully metallic/buzzy FM stack, 0 = the "chip-simple" filtered-noise
 * alternative from the plan doc), through a highpass filter (Tone knob =
 * cutoff) into an amp envelope. Shared by Crash, Ride, and both Hi-Hats -
 * they differ in decay range/defaults, highpass sweep range (set via
 * setToneRange), partial spread (setPartials - defaults to the shared
 * inharmonic set below, Ride gets a tighter/more harmonic one, see
 * DrumEngine) and whether Metal is user-exposed (hi-hats always did; Crash
 * and Ride now do too, see Params.hpp). choke() lets the hi-hat closed/open
 * pair form a choke group.
 *
 * Optional Bell layer: a single sine partial with its own envelope, mixed
 * in on top of everything else (not filtered, not gated by the main amp
 * envelope) - the classic ride "ping" that rings independently of the
 * noisy wash. Mix defaults to 0 (silent/no-op) so Crash and both hi-hats
 * are unaffected unless a plugin explicitly dials it in via setBellMix().
 *
 * Tune (semitones, setTune()) transposes the whole partial set - and the
 * Bell, so it stays musically related to the wash - by a common multiplier,
 * the same trick real 808/909-style cymbal "tune" pots use: it keeps the
 * inharmonic *ratios* between partials fixed while shifting the whole
 * metallic stack up/down, which is what actually changes its perceived size
 * (Tone only sweeps the highpass, it doesn't retune anything).
 */

#pragma once

#include <cmath>
#include <algorithm>

#include "Oscillator.hpp"
#include "SineOscillator.hpp"
#include "Noise.hpp"
#include "Filter.hpp"
#include "ADSR.hpp"

namespace sideous {

class CymbalVoice
{
public:
    static constexpr float kAttackSeconds = 0.002f;
    // classic 808/909-style inharmonic partial set (Hz) - the shared default
    static constexpr float kDefaultPartials[6] = { 205.3f, 304.4f, 369.6f, 522.7f, 540.0f, 800.0f };
    // bell rings longer than the main wash - not user-exposed, just character
    static constexpr float kBellDecayMultiplier = 1.6f;
    // bell pitch at Tune=0 (semitones); scales with Tune same as the partials
    static constexpr float kBellBaseHz = 600.0f;

    CymbalVoice() noexcept
    {
        for (size_t i = 0; i < 6; ++i)
            fPartials[i] = kDefaultPartials[i];
    }

    void setSampleRate(double sampleRate) noexcept
    {
        for (size_t i = 0; i < 6; ++i)
        {
            fOscs[i].setSampleRate(sampleRate);
            fOscs[i].setWaveform(Waveform::Pulse);
            fOscs[i].setPulseWidth(0.5f);
            fOscs[i].setFrequency(fPartials[i]);
        }

        fFilter.setSampleRate(sampleRate);
        fFilter.setType(FilterType::Highpass);
        fFilter.setResonance(0.15f);

        fAmpEnv.setSampleRate(sampleRate);
        fAmpEnv.setAttack(kAttackSeconds);
        fAmpEnv.setSustain(0.0f);

        fBell.setSampleRate(sampleRate);
        fBellEnv.setSampleRate(sampleRate);
        fBellEnv.setAttack(kAttackSeconds);
        fBellEnv.setSustain(0.0f);
    }

    // overrides the default inharmonic spread - e.g. a tighter, more
    // harmonically-related cluster gives a more "pitched" character
    // (used for Ride, see DrumEngine); must be called before setSampleRate()
    void setPartials(const float (&partials)[6]) noexcept
    {
        for (size_t i = 0; i < 6; ++i)
            fPartials[i] = partials[i];
    }

    // Tone knob (0..1) sweeps the highpass cutoff between these two Hz bounds
    void setToneRange(float minHz, float maxHz) noexcept { fToneMinHz = minHz; fToneMaxHz = maxHz; }
    void setTone(float tone01) noexcept { fTone = std::clamp(tone01, 0.0f, 1.0f); }
    void setDecay(float seconds) noexcept
    {
        fAmpEnv.setDecay(seconds);
        fBellEnv.setDecay(seconds * kBellDecayMultiplier);
    }
    // 1 = fully metallic FM stack (buzzy/jangly), 0 = plain filtered noise
    void setMetal(float metal01) noexcept { fMetal = std::clamp(metal01, 0.0f, 1.0f); }

    // bell "ping" layer: 0 = silent (default, off for Crash/hi-hats), 1 = full mix
    void setBellMix(float mix01) noexcept { fBellMix = std::clamp(mix01, 0.0f, 1.0f); }

    // transposes the partial set (and the bell) together, see class comment
    void setTune(float semitones) noexcept
    {
        fTuneMult = std::exp2(semitones / 12.0f);
        fBellTuneHz = kBellBaseHz * fTuneMult;
    }

    void trigger(float velocity) noexcept
    {
        fVelocity = velocity;
        fAmpEnv.noteOn();
        fBell.resetPhase();
        fBellEnv.noteOn();
    }

    // hi-hat choke group: closed-hat hits cut off a still-ringing open hat
    void choke() noexcept { fAmpEnv.choke(); fBellEnv.choke(); }

    float process() noexcept
    {
        float fmSum = 0.0f;
        for (size_t i = 0; i < 6; ++i)
        {
            fOscs[i].setFrequency(fPartials[i] * fTuneMult);
            fmSum += fOscs[i].process();
        }
        fmSum *= 1.0f / 6.0f;

        // blend the raw sources before filtering (not two separate filtered
        // signals mixed after) so there's only one filter call per sample
        // and the blend itself stays cheap
        const float raw = fMetal * fmSum + (1.0f - fMetal) * fNoise.process();

        const float cutoff = fToneMinHz * std::pow(fToneMaxHz / fToneMinHz, fTone);
        const float filtered = fFilter.process(raw, cutoff) * fAmpEnv.process();

        fBell.setFrequency(fBellTuneHz);
        const float bell = fBell.process() * fBellEnv.process() * fBellMix;

        return (filtered + bell) * fVelocity;
    }

    // current envelope level (0..1), for UI activity indicators - the louder of the
    // main wash and the (possibly still-ringing) bell layer
    float getLevel() const noexcept
    {
        const float amp = fAmpEnv.getLevel();
        const float bell = fBellEnv.getLevel();
        return amp > bell ? amp : bell;
    }

private:
    float fPartials[6];
    Oscillator fOscs[6];
    Noise fNoise;
    Filter fFilter;
    ADSR fAmpEnv;

    SineOscillator fBell;
    ADSR fBellEnv;
    float fBellMix = 0.0f;
    float fBellTuneHz = kBellBaseHz;

    float fToneMinHz = 2500.0f;
    float fToneMaxHz = 9000.0f;
    float fTone = 0.5f;
    float fMetal = 1.0f;
    float fTuneMult = 1.0f;
    float fVelocity = 1.0f;
};

} // namespace sideous
