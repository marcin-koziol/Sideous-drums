/*
 * Sideous Drums - decay-only pitch envelope, in semitones above the voice's
 * base tune. This is the classic "pitch sweep" trick that makes a kick/tom
 * feel like a drum instead of a static-pitch tone: starts at `depth`
 * semitones and decays exponentially back to 0 over `decaySeconds`.
 */

#pragma once

#include <cmath>
#include <algorithm>

namespace sideous {

class PitchEnvelope
{
public:
    void setSampleRate(double sampleRate) noexcept
    {
        fSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    }

    void setDepth(float semitones) noexcept { fDepth = semitones; }

    void setDecay(float seconds) noexcept
    {
        seconds = std::max(0.001f, seconds);
        // time constant tuned so the envelope is audibly "done" (< 0.1%
        // of depth) within roughly `seconds`, not just decayed by 1/e
        fCoeff = std::exp(-6.9f / ((float)fSampleRate * seconds));
    }

    void trigger() noexcept
    {
        fLevel = 1.0f;
    }

    // returns the current pitch offset in semitones
    float process() noexcept
    {
        const float semitones = fDepth * fLevel;
        fLevel *= fCoeff;
        if (fLevel < 0.0005f)
            fLevel = 0.0f;
        return semitones;
    }

private:
    double fSampleRate = 44100.0;
    float fDepth = 0.0f;
    float fCoeff = 0.999f;
    float fLevel = 0.0f;
};

} // namespace sideous
