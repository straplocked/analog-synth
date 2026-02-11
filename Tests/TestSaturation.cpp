#include "catch_amalgamated.hpp"
#include "DSP/Saturation.h"
#include <cmath>

TEST_CASE ("Saturation::softClip", "[dsp]")
{
    SECTION ("Zero input -> zero output")
    {
        REQUIRE (Saturation::softClip (0.0f) == Catch::Approx (0.0f));
    }

    SECTION ("Output bounded to [-1, 1]")
    {
        REQUIRE (Saturation::softClip (100.0f) <= 1.0f);
        REQUIRE (Saturation::softClip (100.0f) > 0.99f);
        REQUIRE (Saturation::softClip (-100.0f) >= -1.0f);
        REQUIRE (Saturation::softClip (-100.0f) < -0.99f);
    }

    SECTION ("Odd symmetry: f(-x) = -f(x)")
    {
        for (float x = 0.1f; x <= 3.0f; x += 0.3f)
            REQUIRE (Saturation::softClip (-x) == Catch::Approx (-Saturation::softClip (x)));
    }

    SECTION ("Is monotonically increasing")
    {
        float prev = Saturation::softClip (-5.0f);
        for (float x = -4.5f; x <= 5.0f; x += 0.5f)
        {
            float val = Saturation::softClip (x);
            REQUIRE (val > prev);
            prev = val;
        }
    }
}

TEST_CASE ("Saturation::warmSaturate", "[dsp]")
{
    SECTION ("Zero input -> zero output")
    {
        REQUIRE (Saturation::warmSaturate (0.0f, 1.0f) == Catch::Approx (0.0f));
    }

    SECTION ("Output exceeds tanh at moderate levels due to extra term")
    {
        // warmSaturate = tanh(driven) + 0.05 * driven * |driven|
        // For positive input, both terms are positive, so output > tanh alone
        float warm = Saturation::warmSaturate (0.3f, 1.0f);
        float plain = std::tanh (0.3f);
        REQUIRE (warm > plain);
    }

    SECTION ("Drive increases saturation")
    {
        float lowDrive = std::abs (Saturation::warmSaturate (0.8f, 1.0f));
        float highDrive = std::abs (Saturation::warmSaturate (0.8f, 4.0f));
        // Higher drive saturates more (output doesn't scale linearly)
        REQUIRE (highDrive > lowDrive);
    }
}

TEST_CASE ("Saturation::tapeStyle", "[dsp]")
{
    SECTION ("Zero input -> zero output")
    {
        REQUIRE (Saturation::tapeStyle (0.0f, 1.0f) == Catch::Approx (0.0f));
    }

    SECTION ("Continuity at x*drive = +-1 boundary")
    {
        float drive = 1.0f;
        float below = Saturation::tapeStyle (0.999f, drive);
        float above = Saturation::tapeStyle (1.001f, drive);
        // Should be approximately continuous
        REQUIRE (std::abs (below - above) < 0.01f);
    }

    SECTION ("Output clamps at +-2/3 for large inputs")
    {
        REQUIRE (Saturation::tapeStyle (5.0f, 1.0f) == Catch::Approx (2.0f / 3.0f));
        REQUIRE (Saturation::tapeStyle (-5.0f, 1.0f) == Catch::Approx (-2.0f / 3.0f));
    }

    SECTION ("Odd symmetry")
    {
        for (float x = 0.1f; x <= 2.0f; x += 0.2f)
            REQUIRE (Saturation::tapeStyle (-x, 1.0f) == Catch::Approx (-Saturation::tapeStyle (x, 1.0f)));
    }
}
