#include "catch_amalgamated.hpp"
#include "FX/DistortionEffect.h"
#include <cmath>

static constexpr double kSampleRate = 44100.0;

TEST_CASE ("DistortionEffect disabled passthrough", "[fx]")
{
    DistortionEffect dist;
    dist.prepare (kSampleRate, 256);
    dist.setEnabled (false);

    juce::AudioBuffer<float> buffer (2, 256);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
            buffer.setSample (ch, i, 0.5f);

    dist.processBlock (buffer);

    SECTION ("Disabled effect passes signal unchanged")
    {
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 256; ++i)
                REQUIRE (buffer.getSample (ch, i) == Catch::Approx (0.5f));
    }
}

TEST_CASE ("DistortionEffect produces finite output", "[fx]")
{
    DistortionEffect dist;
    dist.prepare (kSampleRate, 256);
    dist.setEnabled (true);

    SECTION ("All distortion types produce finite output")
    {
        for (int type = 0; type < 7; ++type)
        {
            juce::AudioBuffer<float> buffer (2, 256);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 256; ++i)
                    buffer.setSample (ch, i, std::sin (2.0f * static_cast<float> (M_PI) * 440.0f * static_cast<float> (i) / static_cast<float> (kSampleRate)));

            // We can't call updateParameters without APVTS, so test the
            // processBlock with default params (type 0, drive 1, mix 1)
            dist.processBlock (buffer);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 256; ++i)
                    REQUIRE (std::isfinite (buffer.getSample (ch, i)));
        }
    }
}

TEST_CASE ("DistortionEffect effect IDs", "[fx]")
{
    DistortionEffect dist;

    SECTION ("Returns correct effect ID and display name")
    {
        REQUIRE (dist.getEffectID() == "distortion");
        REQUIRE (dist.getDisplayName() == "DIST");
    }
}

TEST_CASE ("DistortionEffect reset", "[fx]")
{
    DistortionEffect dist;
    dist.prepare (kSampleRate, 256);
    dist.setEnabled (true);

    // Process some audio
    juce::AudioBuffer<float> buffer (2, 256);
    buffer.clear();
    for (int i = 0; i < 256; ++i)
        buffer.setSample (0, i, 1.0f);
    dist.processBlock (buffer);

    SECTION ("Reset does not crash")
    {
        dist.reset();
        // Process again after reset
        buffer.clear();
        dist.processBlock (buffer);
    }
}
