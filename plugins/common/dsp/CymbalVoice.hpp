/*
 * Sideous Drums - metallic FM-ish cymbal, shared by Crash, Ride, and both
 * Hi-Hats. Three layers on top of the original 808/909 "6 square oscillators
 * at fixed inharmonic ratios" trick:
 *
 *  - Ring-mod cascade: the 6 partials are still summed directly (the
 *    original "fmSum" layer), but also ring-modulated against each other in
 *    adjacent pairs (osc0*osc1, osc2*osc3, osc4*osc5) and blended in. Ring
 *    mod produces new sum/difference frequencies from its inputs, so this
 *    adds real inharmonic density from the same 6 oscillators instead of
 *    needing more of them - the classic cheap trick for a busier, more
 *    "metallic" spectrum than a plain sum gives.
 *  - True stereo: L and R each get their own oscillator bank, filter, and
 *    noise source - R runs at a small fixed detune (see kStereoDetune) from
 *    L instead of just being a duplicate. Previously this voice returned a
 *    single mono value that both build's run() loops copied into both
 *    channels; a real cymbal wash spread across the stereo field reads as
 *    far more expensive/natural than a mono blob doubled to L/R.
 *  - Shimmer: a slow (sub-1Hz), tiny, always-running pitch LFO (not user
 *    exposed) modulates the oscillator/bell frequencies, in opposite phase
 *    between L and R. Real cymbals have a decay that keeps living/breathing
 *    as partials drift against each other; a static filtered-noise tail
 *    doesn't. The L/R phase offset also very subtly widens the stereo image
 *    over the decay.
 *
 * Blended against plain filtered noise via the Metal knob (1 = fully
 * metallic/buzzy stack, 0 = the "chip-simple" filtered-noise alternative)
 * through a highpass filter (Tone knob = cutoff) into an amp envelope.
 *
 * Optional Bell layer: a sine partial (also stereo-detuned) with its own
 * envelope, mixed in on top of everything else (not filtered, not gated by
 * the main amp envelope) - the classic ride "ping" that rings independently
 * of the noisy wash. Mix defaults to 0 (silent/no-op) so Crash and both
 * hi-hats are unaffected unless a plugin explicitly dials it in.
 *
 * Tune (semitones, setTune()) transposes the whole partial set - and the
 * Bell, so it stays musically related to the wash - by a common multiplier,
 * the same trick real 808/909-style cymbal "tune" pots use: it keeps the
 * inharmonic *ratios* between partials fixed while shifting the whole
 * metallic stack up/down (Tone only sweeps the highpass, it doesn't retune
 * anything). setPartials() lets Ride use a tighter, more harmonic cluster
 * than Crash/hi-hats' wide inharmonic spread, see DrumEngine.
 *
 * Attack (setAttack()) controls the main/bell envelopes' rise time - was a
 * fixed 2ms with no user control at all. Separately, a fixed (not exposed)
 * broadband noise "click" fires on every trigger regardless of Attack: real
 * cymbals have an instant, full-spectrum strike transient at the stick
 * contact point that a slow-rising *band-limited* wash alone can't fake
 * (the FM stack only covers its own partials, however fast it opens) - the
 * click is what actually reads as "being hit" rather than "fading in".
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
    struct StereoSample { float l, r; };

    static constexpr float kDefaultAttackSeconds = 0.002f;
    // fixed strike-transient click layer, not user-exposed - see class comment
    static constexpr float kClickAttackSeconds = 0.0003f;
    static constexpr float kClickDecaySeconds = 0.004f;
    static constexpr float kClickAmount = 0.35f;
    // classic 808/909-style inharmonic partial set (Hz) - the shared default
    static constexpr float kDefaultPartials[6] = { 205.3f, 304.4f, 369.6f, 522.7f, 540.0f, 800.0f };
    // bell rings longer than the main wash - not user-exposed, just character
    static constexpr float kBellDecayMultiplier = 1.6f;
    // bell pitch at Tune=0 (semitones); scales with Tune same as the partials
    static constexpr float kBellBaseHz = 600.0f;
    // R channel runs this much sharper than L - small enough to read as
    // "wide" rather than "detuned"/out of tune
    static constexpr float kStereoDetune = 1.006f;
    static constexpr float kBellStereoDetune = 1.0015f; // subtler: a single sine beats more obviously than a dense stack
    // slow always-on pitch wobble so the decay keeps moving instead of sitting static
    static constexpr float kShimmerRateHz = 0.35f;
    static constexpr float kShimmerDepth = 0.006f; // +-0.6% frequency

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

            fOscsR[i].setSampleRate(sampleRate);
            fOscsR[i].setWaveform(Waveform::Pulse);
            fOscsR[i].setPulseWidth(0.5f);
            fOscsR[i].setFrequency(fPartials[i] * kStereoDetune);
        }

        fFilter.setSampleRate(sampleRate);
        fFilter.setType(FilterType::Highpass);
        fFilter.setResonance(0.15f);
        fFilterR.setSampleRate(sampleRate);
        fFilterR.setType(FilterType::Highpass);
        fFilterR.setResonance(0.15f);

        fAmpEnv.setSampleRate(sampleRate);
        fAmpEnv.setAttack(kDefaultAttackSeconds);
        fAmpEnv.setSustain(0.0f);

        fBell.setSampleRate(sampleRate);
        fBellR.setSampleRate(sampleRate);
        fBellEnv.setSampleRate(sampleRate);
        fBellEnv.setAttack(kDefaultAttackSeconds);
        fBellEnv.setSustain(0.0f);

        fClickEnv.setSampleRate(sampleRate);
        fClickEnv.setAttack(kClickAttackSeconds);
        fClickEnv.setDecay(kClickDecaySeconds);
        fClickEnv.setSustain(0.0f);

        fShimmerInc = kShimmerRateHz / (float)sampleRate;
    }

    // overrides the default inharmonic spread - e.g. a tighter, more
    // harmonically-related cluster gives a more "pitched" character
    // (used for Ride, see DrumEngine); must be called before setSampleRate()
    void setPartials(const float (&partials)[6]) noexcept
    {
        for (size_t i = 0; i < 6; ++i)
            fPartials[i] = partials[i];
    }

    // every Noise instance defaults to the same seed (see Noise.hpp) - give
    // each voice a distinct one, or their noise layers are literally
    // identical sample-for-sample and audibly comb-filter/cancel when they
    // play together (see DrumEngine's constructor). The R channel derives
    // its own seed internally so callers only need to pass one.
    void setNoiseSeed(uint32_t seed) noexcept
    {
        fNoise.setSeed(seed);
        fNoiseR.setSeed(seed ^ 0x5bd1e995u);
        fClickNoise.setSeed(seed ^ 0x27d4eb2fu);
        fClickNoiseR.setSeed(seed ^ 0x165667b1u);
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

    // rise time of the main wash and bell envelopes - was fixed at 2ms
    void setAttack(float seconds) noexcept
    {
        fAmpEnv.setAttack(seconds);
        fBellEnv.setAttack(seconds);
    }

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
        fBellR.resetPhase();
        fBellEnv.noteOn();
        fClickEnv.noteOn();
    }

    // hi-hat choke group: closed-hat hits cut off a still-ringing open hat
    void choke() noexcept { fAmpEnv.choke(); fBellEnv.choke(); fClickEnv.choke(); }

    StereoSample process() noexcept
    {
        fShimmerPhase += fShimmerInc;
        if (fShimmerPhase >= 1.0f)
            fShimmerPhase -= 1.0f;
        const float shimmerL = 1.0f + kShimmerDepth * std::sin(2.0f * (float)M_PI * fShimmerPhase);
        const float shimmerR = 1.0f + kShimmerDepth * std::sin(2.0f * (float)M_PI * fShimmerPhase + (float)M_PI * 0.5f);

        const float denseL = computeDenseFM(fOscs,  fTuneMult * shimmerL);
        const float denseR = computeDenseFM(fOscsR, fTuneMult * kStereoDetune * shimmerR);

        const float rawL = fMetal * denseL + (1.0f - fMetal) * fNoise.process();
        const float rawR = fMetal * denseR + (1.0f - fMetal) * fNoiseR.process();

        const float cutoff = fToneMinHz * std::pow(fToneMaxHz / fToneMinHz, fTone);
        const float env = fAmpEnv.process(); // one shared envelope, applied to both channels
        const float filteredL = fFilter.process(rawL, cutoff) * env;
        const float filteredR = fFilterR.process(rawR, cutoff) * env;

        fBell.setFrequency(fBellTuneHz * shimmerL);
        fBellR.setFrequency(fBellTuneHz * kBellStereoDetune * shimmerR);
        const float bellEnv = fBellEnv.process(); // one shared envelope
        const float bellL = fBell.process()  * bellEnv * fBellMix;
        const float bellR = fBellR.process() * bellEnv * fBellMix;

        // unfiltered broadband strike transient, own fast envelope - see class comment
        const float clickEnv = fClickEnv.process();
        const float clickL = fClickNoise.process()  * clickEnv * kClickAmount;
        const float clickR = fClickNoiseR.process() * clickEnv * kClickAmount;

        return { (filteredL + bellL + clickL) * fVelocity, (filteredR + bellR + clickR) * fVelocity };
    }

    // current envelope level (0..1), for UI activity indicators - the loudest of the
    // main wash, the (possibly still-ringing) bell layer, and the strike click
    float getLevel() const noexcept
    {
        const float amp = fAmpEnv.getLevel();
        const float bell = fBellEnv.getLevel();
        const float click = fClickEnv.getLevel();
        return std::max(amp, std::max(bell, click));
    }

private:
    // sums the 6 partials directly, ring-mods them in adjacent pairs for
    // extra inharmonic density, and blends the two - see class comment
    float computeDenseFM(Oscillator (&oscs)[6], float freqMult) noexcept
    {
        float o[6];
        for (size_t i = 0; i < 6; ++i)
        {
            oscs[i].setFrequency(fPartials[i] * freqMult);
            o[i] = oscs[i].process();
        }

        const float fmSum = (o[0] + o[1] + o[2] + o[3] + o[4] + o[5]) * (1.0f / 6.0f);
        const float crossMod = (o[0] * o[1] + o[2] * o[3] + o[4] * o[5]) * (1.0f / 3.0f);
        return 0.6f * fmSum + 0.4f * crossMod;
    }

    float fPartials[6];
    Oscillator fOscs[6], fOscsR[6];
    Noise fNoise, fNoiseR;
    Filter fFilter, fFilterR;
    ADSR fAmpEnv;

    SineOscillator fBell, fBellR;
    ADSR fBellEnv;
    float fBellMix = 0.0f;
    float fBellTuneHz = kBellBaseHz;

    Noise fClickNoise, fClickNoiseR;
    ADSR fClickEnv;

    float fShimmerPhase = 0.0f;
    float fShimmerInc = 0.0f;

    float fToneMinHz = 2500.0f;
    float fToneMaxHz = 9000.0f;
    float fTone = 0.5f;
    float fMetal = 1.0f;
    float fTuneMult = 1.0f;
    float fVelocity = 1.0f;
};

} // namespace sideous
