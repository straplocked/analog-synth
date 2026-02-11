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

#include <cstdint>

class NoiseGenerator
{
public:
    void setSeed (uint32_t newSeed) { state = newSeed; }

    float processSample()
    {
        // xorshift32 — fast, decent quality white noise
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;

        // Convert to float in range [-1, 1]
        return static_cast<float> (static_cast<int32_t> (state)) / static_cast<float> (INT32_MAX);
    }

    void renderBlock (float* output, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
            output[i] = processSample();
    }

private:
    uint32_t state = 12345;
};
