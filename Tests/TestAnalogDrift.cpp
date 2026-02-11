#include "catch_amalgamated.hpp"
#include "DSP/AnalogDrift.h"
#include <cmath>

static constexpr double kSampleRate = 44100.0;

TEST_CASE ("AnalogDrift per-voice uniqueness", "[dsp]")
{
    SECTION ("Different voice indices produce different offsets")
    {
        AnalogDrift d0, d1;
        d0.prepare (kSampleRate, 0);
        d1.prepare (kSampleRate, 1);
        d0.setAmount (1.0f);
        d1.setAmount (1.0f);

        REQUIRE (d0.getFilterOffsetFactor() != Catch::Approx (d1.getFilterOffsetFactor()));
    }
}

TEST_CASE ("AnalogDrift amount scaling", "[dsp]")
{
    AnalogDrift drift;
    drift.prepare (kSampleRate, 0);

    SECTION ("Zero amount produces no drift")
    {
        drift.setAmount (0.0f);
        drift.advance (1000);

        REQUIRE (drift.getPitchDriftCents() == Catch::Approx (0.0f).margin (1e-6));
        REQUIRE (drift.getFilterOffsetFactor() == Catch::Approx (1.0f));
        REQUIRE (drift.getEnvTimeOffsetFactor() == Catch::Approx (1.0f));
        REQUIRE (drift.getDCOffset() == Catch::Approx (0.0f));
    }

    SECTION ("Full amount produces bounded drift")
    {
        drift.setAmount (1.0f);
        drift.advance (1000);

        // Pitch drift should be within +-3 cents at max
        REQUIRE (std::abs (drift.getPitchDriftCents()) <= 3.1f);

        // Filter offset should be within +-5%
        float fof = drift.getFilterOffsetFactor();
        REQUIRE (fof >= 0.94f);
        REQUIRE (fof <= 1.06f);

        // Env time offset within +-5%
        float eof = drift.getEnvTimeOffsetFactor();
        REQUIRE (eof >= 0.94f);
        REQUIRE (eof <= 1.06f);

        // Resonance offset within +-4%
        float rof = drift.getResonanceOffsetFactor();
        REQUIRE (rof >= 0.95f);
        REQUIRE (rof <= 1.05f);

        // Drive offset within +-3%
        float dof = drift.getDriveOffsetFactor();
        REQUIRE (dof >= 0.96f);
        REQUIRE (dof <= 1.04f);

        // LFO rate offset within +-5%
        float lof = drift.getLFORateOffsetFactor();
        REQUIRE (lof >= 0.94f);
        REQUIRE (lof <= 1.06f);

        // DC offset within +-0.005
        REQUIRE (std::abs (drift.getDCOffset()) <= 0.006f);
    }
}

TEST_CASE ("AnalogDrift changes over time", "[dsp]")
{
    AnalogDrift drift;
    drift.prepare (kSampleRate, 0);
    drift.setAmount (1.0f);

    SECTION ("Pitch drift changes after advancing")
    {
        drift.advance (1);
        float first = drift.getPitchDriftCents();
        drift.advance (static_cast<int> (kSampleRate));
        float later = drift.getPitchDriftCents();
        REQUIRE (first != Catch::Approx (later).margin (0.001));
    }
}

TEST_CASE ("AnalogDrift processSample compatibility", "[dsp]")
{
    AnalogDrift drift;
    drift.prepare (kSampleRate, 2);
    drift.setAmount (0.5f);

    SECTION ("processSample returns finite values")
    {
        for (int i = 0; i < 10000; ++i)
        {
            float s = drift.processSample();
            REQUIRE (std::isfinite (s));
        }
    }
}
