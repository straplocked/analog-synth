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

TEST_CASE ("NoiseGenerator pink noise", "[dsp]")
{
    NoiseGenerator noise;
    noise.setNoiseType (1); // Pink
    noise.prepare (44100.0);

    SECTION ("Output bounded and finite")
    {
        for (int i = 0; i < 100000; ++i)
        {
            float s = noise.processSample();
            REQUIRE (std::isfinite (s));
            REQUIRE (s >= -1.5f);
            REQUIRE (s <= 1.5f);
        }
    }

    SECTION ("Pink noise has less high-frequency energy than white")
    {
        // Compare variance of sample-to-sample differences (proxy for HF energy)
        NoiseGenerator white, pink;
        white.setSeed (42);
        white.setNoiseType (0);
        white.prepare (44100.0);
        pink.setSeed (42);
        pink.setNoiseType (1);
        pink.prepare (44100.0);

        int N = 50000;
        double whiteDiffVar = 0.0, pinkDiffVar = 0.0;
        float prevW = white.processSample(), prevP = pink.processSample();
        for (int i = 1; i < N; ++i)
        {
            float w = white.processSample();
            float p = pink.processSample();
            float dw = w - prevW;
            float dp = p - prevP;
            whiteDiffVar += dw * dw;
            pinkDiffVar += dp * dp;
            prevW = w;
            prevP = p;
        }
        // Pink should have lower HF energy (smaller difference variance)
        REQUIRE (pinkDiffVar < whiteDiffVar);
    }
}

TEST_CASE ("NoiseGenerator brown noise", "[dsp]")
{
    NoiseGenerator noise;
    noise.setNoiseType (2); // Brown
    noise.prepare (44100.0);

    SECTION ("Output bounded to [-1, 1]")
    {
        for (int i = 0; i < 100000; ++i)
        {
            float s = noise.processSample();
            REQUIRE (std::isfinite (s));
            REQUIRE (s >= -1.0f);
            REQUIRE (s <= 1.0f);
        }
    }

    SECTION ("Brown noise is smoother than white (lower HF energy)")
    {
        NoiseGenerator white, brown;
        white.setSeed (42);
        white.setNoiseType (0);
        white.prepare (44100.0);
        brown.setSeed (42);
        brown.setNoiseType (2);
        brown.prepare (44100.0);

        int N = 50000;
        double whiteDiffVar = 0.0, brownDiffVar = 0.0;
        float prevW = white.processSample(), prevB = brown.processSample();
        for (int i = 1; i < N; ++i)
        {
            float w = white.processSample();
            float b = brown.processSample();
            float dw = w - prevW;
            float db = b - prevB;
            whiteDiffVar += dw * dw;
            brownDiffVar += db * db;
            prevW = w;
            prevB = b;
        }
        REQUIRE (brownDiffVar < whiteDiffVar);
    }
}
