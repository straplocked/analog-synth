#include "catch_amalgamated.hpp"
#include "DSP/LadderFilter.h"
#include <cmath>

static constexpr double kSampleRate = 44100.0;

TEST_CASE ("LadderFilter basic operation", "[dsp]")
{
    LadderFilter filter;
    filter.prepare (kSampleRate);
    filter.setCutoff (1000.0f);
    filter.setResonance (0.0f);
    filter.setDrive (1.0f);

    SECTION ("Silence in -> silence out")
    {
        for (int i = 0; i < 1000; ++i)
            REQUIRE (filter.processSample (0.0f) == Catch::Approx (0.0f).margin (1e-10));
    }

    SECTION ("Output is finite for normal input")
    {
        for (int i = 0; i < 10000; ++i)
        {
            float input = std::sin (2.0f * static_cast<float> (M_PI) * 440.0f * static_cast<float> (i) / static_cast<float> (kSampleRate));
            float output = filter.processSample (input);
            REQUIRE (std::isfinite (output));
        }
    }
}

TEST_CASE ("LadderFilter low-pass behavior", "[dsp]")
{
    LadderFilter filter;
    filter.prepare (kSampleRate);
    filter.setCutoff (500.0f);
    filter.setResonance (0.0f);
    filter.setDrive (1.0f);

    SECTION ("Passes low frequencies, attenuates high frequencies")
    {
        // Low freq (100 Hz) should pass through mostly unchanged
        auto measureAmplitude = [&] (float freq, int numSamples) {
            filter.reset();
            filter.setCutoff (500.0f);
            float maxOut = 0.0f;
            for (int i = 0; i < numSamples; ++i)
            {
                float input = std::sin (2.0f * static_cast<float> (M_PI) * freq * static_cast<float> (i) / static_cast<float> (kSampleRate));
                float output = filter.processSample (input);
                if (i > numSamples / 2) // skip transient
                    maxOut = std::max (maxOut, std::abs (output));
            }
            return maxOut;
        };

        float lowAmp = measureAmplitude (100.0f, 20000);
        float highAmp = measureAmplitude (5000.0f, 20000);

        // Low frequency should be louder than high frequency
        REQUIRE (lowAmp > highAmp * 2.0f);
    }
}

TEST_CASE ("LadderFilter resonance", "[dsp]")
{
    LadderFilter filter;
    filter.prepare (kSampleRate);
    filter.setCutoff (1000.0f);
    filter.setDrive (1.0f);

    SECTION ("Higher resonance boosts near cutoff")
    {
        auto measureAtCutoff = [&] (float res) {
            filter.reset();
            filter.setResonance (res);
            float maxOut = 0.0f;
            for (int i = 0; i < 20000; ++i)
            {
                float input = std::sin (2.0f * static_cast<float> (M_PI) * 1000.0f * static_cast<float> (i) / static_cast<float> (kSampleRate));
                float output = filter.processSample (input);
                if (i > 10000)
                    maxOut = std::max (maxOut, std::abs (output));
            }
            return maxOut;
        };

        float lowRes = measureAtCutoff (0.0f);
        float highRes = measureAtCutoff (0.8f);
        REQUIRE (highRes > lowRes);
    }
}

TEST_CASE ("LadderFilter 2-pole mode", "[dsp]")
{
    LadderFilter filter;
    filter.prepare (kSampleRate);
    filter.setCutoff (1000.0f);
    filter.setResonance (0.0f);
    filter.setDrive (1.0f);

    SECTION ("2-pole has less attenuation than 4-pole")
    {
        auto measureHighFreqAtten = [&] (bool twoPole) {
            filter.reset();
            filter.setTwoPole (twoPole);
            float maxOut = 0.0f;
            for (int i = 0; i < 20000; ++i)
            {
                float input = std::sin (2.0f * static_cast<float> (M_PI) * 5000.0f * static_cast<float> (i) / static_cast<float> (kSampleRate));
                float output = filter.processSample (input);
                if (i > 10000)
                    maxOut = std::max (maxOut, std::abs (output));
            }
            return maxOut;
        };

        float twoPoleAmp = measureHighFreqAtten (true);
        float fourPoleAmp = measureHighFreqAtten (false);
        REQUIRE (twoPoleAmp > fourPoleAmp);
    }
}

TEST_CASE ("LadderFilter reset", "[dsp]")
{
    LadderFilter filter;
    filter.prepare (kSampleRate);
    filter.setCutoff (1000.0f);
    filter.setResonance (0.5f);

    // Push signal through
    for (int i = 0; i < 1000; ++i)
        filter.processSample (1.0f);

    filter.reset();

    SECTION ("Output is zero after reset with zero input")
    {
        REQUIRE (filter.processSample (0.0f) == Catch::Approx (0.0f).margin (1e-6));
    }
}
