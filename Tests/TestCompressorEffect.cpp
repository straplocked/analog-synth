#include "catch_amalgamated.hpp"
#include "FX/CompressorEffect.h"
#include <cmath>

static constexpr double kSampleRate = 44100.0;

TEST_CASE ("CompressorEffect disabled passthrough", "[fx]")
{
    CompressorEffect comp;
    comp.prepare (kSampleRate, 256);
    comp.setEnabled (false);

    juce::AudioBuffer<float> buffer (2, 256);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
            buffer.setSample (ch, i, 0.5f);

    comp.processBlock (buffer);

    SECTION ("Disabled effect passes signal unchanged")
    {
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 256; ++i)
                REQUIRE (buffer.getSample (ch, i) == Catch::Approx (0.5f));
    }
}

TEST_CASE ("CompressorEffect finite output", "[fx]")
{
    CompressorEffect comp;
    comp.prepare (kSampleRate, 256);
    comp.setEnabled (true);

    SECTION ("Produces finite output with loud signal")
    {
        juce::AudioBuffer<float> buffer (2, 256);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 256; ++i)
                buffer.setSample (ch, i, 0.9f * std::sin (2.0f * static_cast<float> (M_PI) * 440.0f * static_cast<float> (i) / static_cast<float> (kSampleRate)));

        comp.processBlock (buffer);

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 256; ++i)
                REQUIRE (std::isfinite (buffer.getSample (ch, i)));
    }
}

TEST_CASE ("CompressorEffect effect IDs", "[fx]")
{
    CompressorEffect comp;
    REQUIRE (comp.getEffectID() == "compressor");
    REQUIRE (comp.getDisplayName() == "COMP");
}

TEST_CASE ("CompressorEffect reset", "[fx]")
{
    CompressorEffect comp;
    comp.prepare (kSampleRate, 256);

    SECTION ("Reset does not crash")
    {
        comp.reset();
    }
}
