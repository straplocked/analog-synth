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
#include "UI/FilterResponseComputer.h"
#include <cmath>

static constexpr float kSR = 44100.0f;

TEST_CASE ("Ladder filter passband near 0 dB", "[response]")
{
    // At well below cutoff, magnitude should be near 0 dB
    float db = FilterResponseComputer::ladderMagnitudeDB (8000.0f, 0.0f, kSR, false, 100.0f);
    REQUIRE (db > -3.0f);
    REQUIRE (db <= 0.1f);
}

TEST_CASE ("Ladder filter stopband is attenuated", "[response]")
{
    // Well above cutoff (1 kHz cutoff, query at 10 kHz) — should be significantly attenuated
    float db = FilterResponseComputer::ladderMagnitudeDB (1000.0f, 0.0f, kSR, false, 10000.0f);
    REQUIRE (db < -40.0f);
}

TEST_CASE ("Ladder 12 dB slope less steep than 24 dB", "[response]")
{
    float db12 = FilterResponseComputer::ladderMagnitudeDB (1000.0f, 0.0f, kSR, true, 4000.0f);
    float db24 = FilterResponseComputer::ladderMagnitudeDB (1000.0f, 0.0f, kSR, false, 4000.0f);
    // 2-pole (12 dB/oct) should be less attenuated than 4-pole (24 dB/oct)
    REQUIRE (db12 > db24);
}

TEST_CASE ("HPF passes high frequencies", "[response]")
{
    float db = FilterResponseComputer::hpfMagnitudeDB (200.0f, kSR, 5000.0f);
    REQUIRE (db > -1.0f);
}

TEST_CASE ("HPF attenuates low frequencies", "[response]")
{
    float db = FilterResponseComputer::hpfMagnitudeDB (1000.0f, kSR, 50.0f);
    REQUIRE (db < -20.0f);
}

TEST_CASE ("Unity biquad gives 0 dB", "[response]")
{
    // Unity filter: b0=1, b1=0, b2=0, a0=1, a1=0, a2=0
    float coeffs[] = { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
    float pi = 3.14159265358979323846f;

    // Test at several frequencies
    for (float freq : { 100.0f, 1000.0f, 5000.0f, 15000.0f })
    {
        float w = 2.0f * pi * freq / kSR;
        float db = FilterResponseComputer::biquadMagnitudeDB (coeffs, w);
        REQUIRE (std::abs (db) < 0.01f);
    }
}

TEST_CASE ("EQ flat response gives ~0 dB", "[response]")
{
    // Unity coefficients for all three bands
    float unity[] = { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };

    for (float freq : { 100.0f, 1000.0f, 10000.0f })
    {
        float db = FilterResponseComputer::eqMagnitudeDB (unity, unity, unity, freq, kSR);
        REQUIRE (std::abs (db) < 0.01f);
    }
}
