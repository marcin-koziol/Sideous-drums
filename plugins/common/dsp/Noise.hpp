/*
 * Sideous Drums - white noise generator (xorshift32), used as the raw
 * source for every noise-layer drum (snare/cymbals/rimshot/maracas), always
 * shaped afterwards by a Filter.hpp stage to get the actual voice's color.
 */

#pragma once

#include <cstdint>

namespace sideous {

class Noise
{
public:
    void setSeed(uint32_t seed) noexcept { fState = seed != 0 ? seed : 0x9e3779b9u; }

    // returns a value in -1..1
    float process() noexcept
    {
        fState ^= fState << 13;
        fState ^= fState >> 17;
        fState ^= fState << 5;
        return ((float)(fState >> 8) / (float)(1u << 24)) * 2.0f - 1.0f;
    }

private:
    uint32_t fState = 0x9e3779b9u;
};

} // namespace sideous
