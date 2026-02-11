/*
 * AnalogSynth - Virtual analog synthesizer plugin
 * Copyright (C) 2025 AnalogSynth contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <juce_core/juce_core.h>
#include <cstdint>
#include <cmath>

class PinkNoise
{
public:
    PinkNoise() = default;

    void prepare (uint32_t seed = 12345)
    {
        state = seed;
        b0 = 0.0f;
        b1 = 0.0f;
        b2 = 0.0f;
        b3 = 0.0f;
        b4 = 0.0f;
        b5 = 0.0f;
        b6 = 0.0f;
    }

    // Returns approximately [-1, +1] pink noise
    float processSample()
    {
        float white = whiteNoise();

        // Paul Kellet's pinking filter (public domain)
        b0 = 0.99886f * b0 + white * 0.0555179f;
        b1 = 0.99332f * b1 + white * 0.0750759f;
        b2 = 0.96900f * b2 + white * 0.1538520f;
        b3 = 0.86650f * b3 + white * 0.3104856f;
        b4 = 0.55000f * b4 + white * 0.5329522f;
        b5 = -0.7616f * b5 - white * 0.0168980f;

        float pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
        b6 = white * 0.115926f;

        return pink * 0.11f; // normalize to approximately [-1, 1]
    }

private:
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
    float b3 = 0.0f, b4 = 0.0f, b5 = 0.0f, b6 = 0.0f;

    uint32_t state = 12345;

    float whiteNoise()
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<float> (static_cast<int32_t> (state)) / static_cast<float> (INT32_MAX);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PinkNoise)
};
