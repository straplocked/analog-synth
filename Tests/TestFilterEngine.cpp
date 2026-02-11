#include "catch_amalgamated.hpp"
#include "DSP/FilterEngine.h"
#include <cmath>

static constexpr double kSampleRate = 44100.0;

TEST_CASE ("FilterEngine type switching", "[dsp]")
{
    FilterEngine engine;
    engine.prepare (kSampleRate);
    engine.setResonance (0.0f);
    engine.setDrive (1.0f);

    SECTION ("All 5 filter types produce finite output")
    {
        for (int type = 0; type < 5; ++type)
        {
            engine.reset();
            engine.setType (type);
            engine.setCutoff (1000.0f);

            for (int i = 0; i < 5000; ++i)
            {
                float input = std::sin (2.0f * static_cast<float> (M_PI) * 440.0f * static_cast<float> (i) / static_cast<float> (kSampleRate));
                float output = engine.processSample (input);
                REQUIRE (std::isfinite (output));
            }
        }
    }
}

TEST_CASE ("FilterEngine slope switching", "[dsp]")
{
    FilterEngine engine;
    engine.prepare (kSampleRate);
    engine.setResonance (0.0f);
    engine.setDrive (1.0f);

    SECTION ("12dB slope has less attenuation than 24dB for 2-pole types")
    {
        // Test with Sequential (SVF) type
        engine.setType (2); // Sequential

        auto measureHighFreqAtten = [&] (int slope) {
            engine.reset();
            engine.setSlope (slope);
            engine.setCutoff (500.0f);
            float maxOut = 0.0f;
            for (int i = 0; i < 20000; ++i)
            {
                float input = std::sin (2.0f * static_cast<float> (M_PI) * 5000.0f * static_cast<float> (i) / static_cast<float> (kSampleRate));
                float output = engine.processSample (input);
                if (i > 10000)
                    maxOut = std::max (maxOut, std::abs (output));
            }
            return maxOut;
        };

        float twelveDBAmp = measureHighFreqAtten (0);
        float twentyFourDBAmp = measureHighFreqAtten (1);
        REQUIRE (twelveDBAmp > twentyFourDBAmp);
    }
}

TEST_CASE ("FilterEngine silence passthrough", "[dsp]")
{
    FilterEngine engine;
    engine.prepare (kSampleRate);
    engine.setCutoff (1000.0f);
    engine.setResonance (0.0f);
    engine.setDrive (1.0f);

    SECTION ("Zero input produces zero output")
    {
        for (int i = 0; i < 1000; ++i)
            REQUIRE (engine.processSample (0.0f) == Catch::Approx (0.0f).margin (1e-6));
    }
}

TEST_CASE ("FilterEngine reset clears state", "[dsp]")
{
    FilterEngine engine;
    engine.prepare (kSampleRate);
    engine.setCutoff (1000.0f);
    engine.setResonance (0.5f);

    // Push signal through
    for (int i = 0; i < 1000; ++i)
        engine.processSample (1.0f);

    engine.reset();

    SECTION ("Output zero after reset with zero input")
    {
        float out = engine.processSample (0.0f);
        REQUIRE (out == Catch::Approx (0.0f).margin (1e-5));
    }
}
