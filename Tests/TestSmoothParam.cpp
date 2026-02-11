#include "catch_amalgamated.hpp"
#include "Utility/SmoothParam.h"
#include <cmath>

TEST_CASE ("SmoothParam basics", "[utility]")
{
    SmoothParam sp;

    SECTION ("Default current value is 0")
    {
        REQUIRE (sp.getCurrentValue() == 0.0f);
    }

    SECTION ("Reset sets both current and target")
    {
        sp.reset (0.5f);
        REQUIRE (sp.getCurrentValue() == Catch::Approx (0.5f));
        // After reset, getNextValue should return ~0.5 (already at target)
        float val = sp.getNextValue();
        REQUIRE (val == Catch::Approx (0.5f).epsilon (0.01));
    }
}

TEST_CASE ("SmoothParam convergence", "[utility]")
{
    SmoothParam sp;
    sp.prepare (44100.0, 5.0f);
    sp.reset (0.0f);
    sp.setTargetValue (1.0f);

    SECTION ("Converges toward target over time")
    {
        float val = 0.0f;
        for (int i = 0; i < 44100; ++i)
            val = sp.getNextValue();

        // After 1 second at 5ms smoothing, should be very close to 1.0
        REQUIRE (val == Catch::Approx (1.0f).margin (0.001));
    }

    SECTION ("Moves toward target after first sample")
    {
        float first = sp.getNextValue();
        REQUIRE (first > 0.0f);
        REQUIRE (first < 1.0f);
    }
}

TEST_CASE ("SmoothParam smoothing time", "[utility]")
{
    SECTION ("Shorter smoothing time converges faster")
    {
        SmoothParam fast, slow;
        fast.prepare (44100.0, 1.0f);
        slow.prepare (44100.0, 50.0f);

        fast.reset (0.0f);
        slow.reset (0.0f);
        fast.setTargetValue (1.0f);
        slow.setTargetValue (1.0f);

        float fastVal = 0.0f, slowVal = 0.0f;
        for (int i = 0; i < 100; ++i)
        {
            fastVal = fast.getNextValue();
            slowVal = slow.getNextValue();
        }

        REQUIRE (fastVal > slowVal);
    }
}
