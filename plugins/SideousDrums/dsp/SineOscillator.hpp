/*
 * Sideous Drums - plain sine oscillator. sideous's Oscillator.hpp only has
 * Saw/Pulse/Triangle; drum tone bodies (kick/tom/rimshot) want a clean sine,
 * the standard 808/909 choice for a punchy low end with no extra harmonics.
 */

#pragma once

#include <cmath>

namespace sideous {

class SineOscillator
{
public:
    void setSampleRate(double sampleRate) noexcept
    {
        fSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    }

    void setFrequency(float hz) noexcept
    {
        fIncrement = (double)hz / fSampleRate;
    }

    void resetPhase(float phase = 0.0f) noexcept { fPhase = phase; }

    float process() noexcept
    {
        const float out = (float)std::sin(2.0 * M_PI * fPhase);
        fPhase += fIncrement;
        if (fPhase >= 1.0)
            fPhase -= 1.0;
        return out;
    }

private:
    double fSampleRate = 44100.0;
    double fPhase = 0.0;
    double fIncrement = 0.0;
};

} // namespace sideous
