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
#include "FX/FXChain.h"
#include <cmath>
#include <limits>

static constexpr double kSampleRate = 44100.0;

TEST_CASE ("FXChain construction", "[fx]")
{
    FXChain chain;

    SECTION ("Has 6 effects")
    {
        for (int i = 0; i < FXChain::kNumEffects; ++i)
            REQUIRE (chain.getEffect (i) != nullptr);
    }

    SECTION ("Out of bounds returns nullptr")
    {
        REQUIRE (chain.getEffect (-1) == nullptr);
        REQUIRE (chain.getEffect (6) == nullptr);
    }

    SECTION ("Default processing order is identity")
    {
        auto order = chain.getProcessingOrder();
        for (int i = 0; i < FXChain::kNumEffects; ++i)
            REQUIRE (order[static_cast<std::size_t> (i)] == i);
    }
}

TEST_CASE ("FXChain effect identities", "[fx]")
{
    FXChain chain;

    SECTION ("Effects are in expected slots")
    {
        REQUIRE (chain.getEffect (FXChain::kDistortion)->getEffectID() == "distortion");
        REQUIRE (chain.getEffect (FXChain::kCompressor)->getEffectID() == "compressor");
        REQUIRE (chain.getEffect (FXChain::kEQ)->getEffectID() == "eq");
        REQUIRE (chain.getEffect (FXChain::kChorus)->getEffectID() == "chorus");
        REQUIRE (chain.getEffect (FXChain::kDelay)->getEffectID() == "delay");
        REQUIRE (chain.getEffect (FXChain::kReverb)->getEffectID() == "reverb");
    }
}

TEST_CASE ("FXChain processBlock with all disabled", "[fx]")
{
    FXChain chain;
    chain.prepare (kSampleRate, 256);

    // All effects disabled by default
    juce::AudioBuffer<float> buffer (2, 256);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
            buffer.setSample (ch, i, 0.3f);

    chain.processBlock (buffer);

    SECTION ("Signal passes through unchanged when all effects disabled")
    {
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 256; ++i)
                REQUIRE (buffer.getSample (ch, i) == Catch::Approx (0.3f));
    }
}

TEST_CASE ("FXChain NaN safety", "[fx]")
{
    FXChain chain;
    chain.prepare (kSampleRate, 256);

    SECTION ("NaN in buffer is replaced with 0")
    {
        juce::AudioBuffer<float> buffer (2, 256);
        buffer.clear();
        buffer.setSample (0, 100, std::numeric_limits<float>::quiet_NaN());
        buffer.setSample (1, 200, std::numeric_limits<float>::infinity());

        chain.processBlock (buffer);

        // After processing, NaN/Inf should be zeroed
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 256; ++i)
            {
                float s = buffer.getSample (ch, i);
                REQUIRE (std::isfinite (s));
            }
    }
}

TEST_CASE ("FXChain silence passthrough", "[fx]")
{
    FXChain chain;
    chain.prepare (kSampleRate, 256);

    SECTION ("Zero input produces zero output")
    {
        juce::AudioBuffer<float> buffer (2, 256);
        buffer.clear();

        chain.processBlock (buffer);

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 256; ++i)
                REQUIRE (buffer.getSample (ch, i) == 0.0f);
    }
}
