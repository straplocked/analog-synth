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
#include "FX/PhaserEffect.h"
#include "FX/FlangerEffect.h"
#include "FX/HyperEffect.h"
#include <cmath>

static constexpr double kSampleRate = 44100.0;

// ============================================================
// PhaserEffect
// ============================================================

TEST_CASE ("PhaserEffect passthrough when disabled", "[fx]")
{
    PhaserEffect phaser;
    phaser.prepare (kSampleRate, 256);

    juce::AudioBuffer<float> buffer (2, 256);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
            buffer.setSample (ch, i, 0.5f);

    phaser.processBlock (buffer);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
            REQUIRE (buffer.getSample (ch, i) == Catch::Approx (0.5f));
}

TEST_CASE ("PhaserEffect produces finite output when enabled", "[fx]")
{
    PhaserEffect phaser;
    phaser.prepare (kSampleRate, 256);
    phaser.setEnabled (true);

    juce::AudioBuffer<float> buffer (2, 256);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
            buffer.setSample (ch, i, std::sin (static_cast<float> (i) * 0.1f) * 0.5f);

    phaser.processBlock (buffer);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
        {
            float s = buffer.getSample (ch, i);
            REQUIRE (std::isfinite (s));
            REQUIRE (s >= -4.0f); // with feedback, could be larger than 1
            REQUIRE (s <= 4.0f);
        }
}

TEST_CASE ("PhaserEffect identity", "[fx]")
{
    PhaserEffect phaser;
    REQUIRE (phaser.getEffectID() == "phaser");
    REQUIRE (phaser.getDisplayName() == "PHASER");
}

// ============================================================
// FlangerEffect
// ============================================================

TEST_CASE ("FlangerEffect passthrough when disabled", "[fx]")
{
    FlangerEffect flanger;
    flanger.prepare (kSampleRate, 256);

    juce::AudioBuffer<float> buffer (2, 256);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
            buffer.setSample (ch, i, 0.3f);

    flanger.processBlock (buffer);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
            REQUIRE (buffer.getSample (ch, i) == Catch::Approx (0.3f));
}

TEST_CASE ("FlangerEffect produces finite output when enabled", "[fx]")
{
    FlangerEffect flanger;
    flanger.prepare (kSampleRate, 256);
    flanger.setEnabled (true);

    juce::AudioBuffer<float> buffer (2, 256);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
            buffer.setSample (ch, i, std::sin (static_cast<float> (i) * 0.05f) * 0.5f);

    flanger.processBlock (buffer);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
        {
            float s = buffer.getSample (ch, i);
            REQUIRE (std::isfinite (s));
        }
}

TEST_CASE ("FlangerEffect identity", "[fx]")
{
    FlangerEffect flanger;
    REQUIRE (flanger.getEffectID() == "flanger");
    REQUIRE (flanger.getDisplayName() == "FLANGER");
}

// ============================================================
// HyperEffect
// ============================================================

TEST_CASE ("HyperEffect passthrough when disabled", "[fx]")
{
    HyperEffect hyper;
    hyper.prepare (kSampleRate, 256);

    juce::AudioBuffer<float> buffer (2, 256);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
            buffer.setSample (ch, i, 0.4f);

    hyper.processBlock (buffer);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
            REQUIRE (buffer.getSample (ch, i) == Catch::Approx (0.4f));
}

TEST_CASE ("HyperEffect produces finite output when enabled", "[fx]")
{
    HyperEffect hyper;
    hyper.prepare (kSampleRate, 256);
    hyper.setEnabled (true);

    juce::AudioBuffer<float> buffer (2, 256);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
            buffer.setSample (ch, i, std::sin (static_cast<float> (i) * 0.08f) * 0.5f);

    hyper.processBlock (buffer);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
        {
            float s = buffer.getSample (ch, i);
            REQUIRE (std::isfinite (s));
        }
}

TEST_CASE ("HyperEffect identity", "[fx]")
{
    HyperEffect hyper;
    REQUIRE (hyper.getEffectID() == "hyper");
    REQUIRE (hyper.getDisplayName() == "HYPER");
}

TEST_CASE ("HyperEffect reset does not crash", "[fx]")
{
    HyperEffect hyper;
    hyper.prepare (kSampleRate, 256);
    hyper.setEnabled (true);

    juce::AudioBuffer<float> buffer (2, 256);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
            buffer.setSample (ch, i, 0.3f);

    hyper.processBlock (buffer);
    hyper.reset();

    // Process again after reset
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
            buffer.setSample (ch, i, 0.3f);

    hyper.processBlock (buffer);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
            REQUIRE (std::isfinite (buffer.getSample (ch, i)));
}
