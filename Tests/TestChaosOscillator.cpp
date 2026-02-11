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
#include "DSP/ChaosOscillator.h"
#include "DSP/ModMatrix.h"
#include <cmath>

TEST_CASE ("ChaosOscillator output is bounded", "[chaos]")
{
    ChaosOscillator chaos;
    chaos.prepare (44100.0);
    chaos.setRate (5.0f);
    chaos.setAmount (1.0f);

    for (int i = 0; i < 100000; ++i)
    {
        float val = chaos.processSample();
        REQUIRE (val >= -1.0f);
        REQUIRE (val <= 1.0f);
    }
}

TEST_CASE ("ChaosOscillator output varies over time", "[chaos]")
{
    ChaosOscillator chaos;
    chaos.prepare (44100.0);
    chaos.setRate (2.0f);
    chaos.setAmount (1.0f);

    // Collect samples over 1 second
    float minVal = 1.0f;
    float maxVal = -1.0f;
    for (int i = 0; i < 44100; ++i)
    {
        float val = chaos.processSample();
        minVal = std::min (minVal, val);
        maxVal = std::max (maxVal, val);
    }

    // The output should span a meaningful range (the Lorenz attractor is chaotic)
    REQUIRE ((maxVal - minVal) > 0.1f);
}

TEST_CASE ("ChaosOscillator amount scales output", "[chaos]")
{
    ChaosOscillator chaos;
    chaos.prepare (44100.0);
    chaos.setRate (3.0f);

    // Run with amount = 0 — output should be 0
    chaos.setAmount (0.0f);
    for (int i = 0; i < 10000; ++i)
    {
        float val = chaos.processSample();
        REQUIRE (val == Catch::Approx (0.0f).margin (1e-6f));
    }
}

TEST_CASE ("ChaosOscillator output is finite", "[chaos]")
{
    ChaosOscillator chaos;
    chaos.prepare (44100.0);
    chaos.setRate (10.0f);
    chaos.setAmount (1.0f);

    for (int i = 0; i < 200000; ++i)
    {
        float val = chaos.processSample();
        REQUIRE (std::isfinite (val));
    }
}

TEST_CASE ("ChaosOscillator as mod matrix source", "[chaos][modmatrix]")
{
    // Verify the Chaos source routes through the mod matrix correctly
    ModMatrix matrix;
    VoiceModValues voice;
    GlobalModValues global;

    global.chaos = 0.6f;

    matrix.setSlot (0, static_cast<int> (ModSource::Chaos),
                    static_cast<int> (ModDest::LPFCutoff), 0.5f);
    matrix.prepareDestinationMask();

    REQUIRE (matrix.hasModulationFor (ModDest::LPFCutoff));
    float result = matrix.computeModulation (ModDest::LPFCutoff, voice, global);
    REQUIRE (result == Catch::Approx (0.6f * 0.5f));
}
