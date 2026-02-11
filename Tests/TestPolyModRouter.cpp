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
#include "DSP/PolyModRouter.h"

TEST_CASE ("PolyModAmounts::isActive", "[dsp]")
{
    SECTION ("Default (all zero) is inactive")
    {
        PolyModAmounts pma;
        REQUIRE_FALSE (pma.isActive());
    }

    SECTION ("Any non-zero filter env source activates")
    {
        PolyModAmounts pma;
        pma.fenvToOsc1Freq = 0.1f;
        REQUIRE (pma.isActive());
    }

    SECTION ("Any non-zero osc2 source activates")
    {
        PolyModAmounts pma;
        pma.osc2ToLPF = 0.5f;
        REQUIRE (pma.isActive());
    }

    SECTION ("Multiple sources: all must be zero for inactive")
    {
        PolyModAmounts pma;
        pma.fenvToOsc1Shape = 0.1f;
        pma.osc2ToOsc1PW = 0.2f;
        REQUIRE (pma.isActive());

        pma.fenvToOsc1Shape = 0.0f;
        pma.osc2ToOsc1PW = 0.0f;
        REQUIRE_FALSE (pma.isActive());
    }
}

TEST_CASE ("LFOAmounts::isActive", "[dsp]")
{
    SECTION ("Default (all zero) is inactive")
    {
        LFOAmounts la;
        REQUIRE_FALSE (la.isActive());
    }

    SECTION ("Any non-zero amount activates")
    {
        {
            LFOAmounts la;
            la.toOscFreq = 0.1f;
            REQUIRE (la.isActive());
        }
        {
            LFOAmounts la;
            la.toOscShape = 0.1f;
            REQUIRE (la.isActive());
        }
        {
            LFOAmounts la;
            la.toFilter = 0.1f;
            REQUIRE (la.isActive());
        }
        {
            LFOAmounts la;
            la.toAmp = 0.1f;
            REQUIRE (la.isActive());
        }
    }

    SECTION ("All zero is inactive")
    {
        LFOAmounts la;
        la.toOscFreq = 0.0f;
        la.toOscShape = 0.0f;
        la.toFilter = 0.0f;
        la.toAmp = 0.0f;
        REQUIRE_FALSE (la.isActive());
    }
}
