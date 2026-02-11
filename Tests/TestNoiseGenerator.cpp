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
