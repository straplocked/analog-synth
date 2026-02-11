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
#include "DSP/FastMath.h"
#include <cmath>

TEST_CASE ("FastMath::exp2 accuracy", "[math]")
{
    SECTION ("Matches std::pow(2,x) within 0.1% relative error")
    {
        for (float x = -10.0f; x <= 10.0f; x += 0.1f)
        {
            float approx = FastMath::exp2 (x);
            float exact = std::pow (2.0f, x);
            float relError = std::abs (approx - exact) / (std::abs (exact) + 1e-30f);
            REQUIRE (relError < 0.001f);
        }
    }

    SECTION ("exp2(0) = 1")
    {
        REQUIRE (FastMath::exp2 (0.0f) == Catch::Approx (1.0f));
    }

    SECTION ("exp2(1) = 2")
    {
        REQUIRE (FastMath::exp2 (1.0f) == Catch::Approx (2.0f).epsilon (1e-5));
    }

    SECTION ("exp2(-1) = 0.5")
    {
        REQUIRE (FastMath::exp2 (-1.0f) == Catch::Approx (0.5f).epsilon (1e-5));
    }

    SECTION ("Handles large positive values")
    {
        float result = FastMath::exp2 (10.0f);
        REQUIRE (result == Catch::Approx (1024.0f).epsilon (0.001));
    }

    SECTION ("Handles large negative values")
    {
        float result = FastMath::exp2 (-10.0f);
        REQUIRE (result == Catch::Approx (1.0f / 1024.0f).epsilon (0.001));
    }

    SECTION ("Clamps at +-126")
    {
        float result = FastMath::exp2 (200.0f);
        float expected = FastMath::exp2 (126.0f);
        REQUIRE (result == expected);
    }
}

TEST_CASE ("FastMath::exp accuracy", "[math]")
{
    SECTION ("Matches std::exp within reasonable error")
    {
        for (float x = -5.0f; x <= 5.0f; x += 0.25f)
        {
            float approx = FastMath::exp (x);
            float exact = std::exp (x);
            float relError = std::abs (approx - exact) / (std::abs (exact) + 1e-30f);
            REQUIRE (relError < 1e-3f);
        }
    }

    SECTION ("exp(0) = 1")
    {
        REQUIRE (FastMath::exp (0.0f) == Catch::Approx (1.0f).epsilon (1e-5));
    }
}

TEST_CASE ("FastMath::tan accuracy", "[math]")
{
    SECTION ("Matches std::tan within 0.3% for |x| < 1.0")
    {
        for (float x = -0.95f; x <= 0.95f; x += 0.05f)
        {
            float approx = FastMath::tan (x);
            float exact = std::tan (x);
            float relError = std::abs (approx - exact) / (std::abs (exact) + 1e-10f);
            REQUIRE (relError < 0.003f);
        }
    }

    SECTION ("tan(0) = 0")
    {
        REQUIRE (FastMath::tan (0.0f) == Catch::Approx (0.0f).margin (1e-10));
    }
}
