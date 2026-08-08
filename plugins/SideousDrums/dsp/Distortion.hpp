/*
 * Sideous Drums - master saturation/grit stage. Same tanh-crossfade drive
 * trick as sideous's Filter.hpp ("in + drive*(tanh(in*gain)-in)"), applied
 * post-mix as a musical character control rather than just a safety net,
 * with a Mix knob so it can sit in subtly instead of being all-or-nothing.
 */

#pragma once

#include <cmath>
#include <algorithm>

namespace sideous {

class Distortion
{
public:
    void setDrive(float drive01) noexcept { fDrive = std::clamp(drive01, 0.0f, 1.0f); }
    void setMix(float mix01) noexcept { fMix = std::clamp(mix01, 0.0f, 1.0f); }

    float process(float in) noexcept
    {
        if (fDrive <= 0.0f)
            return in;

        const float driveGain = 1.0f + fDrive * 9.0f;
        const float saturated = std::tanh(in * driveGain);
        const float wet = in + fDrive * (saturated - in);
        return in + fMix * (wet - in);
    }

private:
    float fDrive = 0.0f;
    float fMix = 1.0f;
};

} // namespace sideous
