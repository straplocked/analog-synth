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

#include "catch_amalgamated.hpp"
#include "DSP/NoiseGenerator.h"
#include <cmath>

TEST_CASE ("NoiseGenerator output properties", "[dsp]")
{
    NoiseGenerator noise;

    SECTION ("Output bounded to [-1, 1]")
    {
        for (int i = 0; i < 100000; ++i)
        {
            float s = noise.processSample();
            REQUIRE (s >= -1.0f);
            REQUIRE (s <= 1.0f);
        }
    }

    SECTION ("Mean near zero (white noise property)")
    {
        double sum = 0.0;
        int N = 100000;
        for (int i = 0; i < N; ++i)
            sum += noise.processSample();
        double mean = sum / N;
        REQUIRE (std::abs (mean) < 0.01);
    }

    SECTION ("Variance is reasonable (spread across range)")
    {
        double sum = 0.0;
        double sumSq = 0.0;
        int N = 100000;
        for (int i = 0; i < N; ++i)
        {
            float s = noise.processSample();
            sum += s;
            sumSq += s * s;
        }
        double mean = sum / N;
        double variance = sumSq / N - mean * mean;
        // Uniform[-1,1] has variance 1/3. xorshift32 should be close.
        REQUIRE (variance > 0.2);
        REQUIRE (variance < 0.5);
    }
}

TEST_CASE ("NoiseGenerator deterministic with seed", "[dsp]")
{
    SECTION ("Same seed produces same sequence")
    {
        NoiseGenerator a, b;
        a.setSeed (42);
        b.setSeed (42);

        for (int i = 0; i < 100; ++i)
            REQUIRE (a.processSample() == b.processSample());
    }

    SECTION ("Different seeds produce different sequences")
    {
        NoiseGenerator a, b;
        a.setSeed (42);
        b.setSeed (99);

        bool anyDifferent = false;
        for (int i = 0; i < 100; ++i)
        {
            if (a.processSample() != b.processSample())
                anyDifferent = true;
        }
        REQUIRE (anyDifferent);
    }
}

TEST_CASE ("NoiseGenerator renderBlock", "[dsp]")
{
    SECTION ("renderBlock matches processSample")
    {
        NoiseGenerator a, b;
        a.setSeed (12345);
        b.setSeed (12345);

        constexpr int N = 256;
        float blockOutput[N];
        a.renderBlock (blockOutput, N);

        for (int i = 0; i < N; ++i)
            REQUIRE (blockOutput[i] == b.processSample());
    }
}
