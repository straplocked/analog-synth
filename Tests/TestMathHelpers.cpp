#include "catch_amalgamated.hpp"
#include "Utility/MathHelpers.h"
#include <cmath>

TEST_CASE ("MathHelpers::noteToFrequency", "[math]")
{
    SECTION ("A4 = 440 Hz at MIDI note 69")
    {
        REQUIRE (MathHelpers::noteToFrequency (69.0f) == Catch::Approx (440.0f));
    }

    SECTION ("Middle C (C4) = ~261.63 Hz at MIDI note 60")
    {
        REQUIRE (MathHelpers::noteToFrequency (60.0f) == Catch::Approx (261.6256f).epsilon (0.001));
    }

    SECTION ("Octave relationship: note+12 doubles frequency")
    {
        float f1 = MathHelpers::noteToFrequency (60.0f);
        float f2 = MathHelpers::noteToFrequency (72.0f);
        REQUIRE (f2 == Catch::Approx (f1 * 2.0f).epsilon (0.001));
    }

    SECTION ("Custom tuning A4 = 432 Hz")
    {
        REQUIRE (MathHelpers::noteToFrequency (69.0f, 432.0f) == Catch::Approx (432.0f));
    }

    SECTION ("Low note: MIDI 0")
    {
        float f = MathHelpers::noteToFrequency (0.0f);
        REQUIRE (f > 0.0f);
        REQUIRE (f == Catch::Approx (8.17580f).epsilon (0.001));
    }
}

TEST_CASE ("MathHelpers::frequencyToNote", "[math]")
{
    SECTION ("440 Hz = MIDI note 69")
    {
        REQUIRE (MathHelpers::frequencyToNote (440.0f) == Catch::Approx (69.0f));
    }

    SECTION ("Roundtrip: noteToFreq -> freqToNote")
    {
        for (float note = 20.0f; note <= 108.0f; note += 1.0f)
        {
            float freq = MathHelpers::noteToFrequency (note);
            float result = MathHelpers::frequencyToNote (freq);
            REQUIRE (result == Catch::Approx (note).epsilon (0.001));
        }
    }
}

TEST_CASE ("MathHelpers::dbToGain", "[math]")
{
    SECTION ("0 dB = unity gain")
    {
        REQUIRE (MathHelpers::dbToGain (0.0f) == Catch::Approx (1.0f));
    }

    SECTION ("-6 dB ~ 0.5")
    {
        REQUIRE (MathHelpers::dbToGain (-6.0206f) == Catch::Approx (0.5f).epsilon (0.001));
    }

    SECTION ("+20 dB = 10x")
    {
        REQUIRE (MathHelpers::dbToGain (20.0f) == Catch::Approx (10.0f).epsilon (0.001));
    }

    SECTION ("-infinity dB approaches 0")
    {
        REQUIRE (MathHelpers::dbToGain (-200.0f) < 1e-8f);
    }
}

TEST_CASE ("MathHelpers::gainToDb", "[math]")
{
    SECTION ("Unity gain = 0 dB")
    {
        REQUIRE (MathHelpers::gainToDb (1.0f) == Catch::Approx (0.0f));
    }

    SECTION ("Roundtrip: dbToGain -> gainToDb")
    {
        for (float db = -60.0f; db <= 20.0f; db += 5.0f)
        {
            float gain = MathHelpers::dbToGain (db);
            float result = MathHelpers::gainToDb (gain);
            REQUIRE (result == Catch::Approx (db).epsilon (0.01));
        }
    }

    SECTION ("Zero gain returns very negative dB")
    {
        REQUIRE (MathHelpers::gainToDb (0.0f) < -100.0f);
    }
}

TEST_CASE ("MathHelpers::centsToRatio", "[math]")
{
    SECTION ("0 cents = ratio 1.0")
    {
        REQUIRE (MathHelpers::centsToRatio (0.0f) == Catch::Approx (1.0f));
    }

    SECTION ("1200 cents = 2x (one octave)")
    {
        REQUIRE (MathHelpers::centsToRatio (1200.0f) == Catch::Approx (2.0f).epsilon (0.001));
    }

    SECTION ("100 cents = one semitone")
    {
        float expected = std::pow (2.0f, 1.0f / 12.0f);
        REQUIRE (MathHelpers::centsToRatio (100.0f) == Catch::Approx (expected).epsilon (0.001));
    }

    SECTION ("-1200 cents = 0.5x")
    {
        REQUIRE (MathHelpers::centsToRatio (-1200.0f) == Catch::Approx (0.5f).epsilon (0.001));
    }
}
