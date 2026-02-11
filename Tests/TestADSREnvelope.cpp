#include "catch_amalgamated.hpp"
#include "DSP/ADSREnvelope.h"
#include <cmath>

static constexpr double kSampleRate = 44100.0;

TEST_CASE ("ADSREnvelope initial state", "[dsp]")
{
    ADSREnvelope env;
    env.prepare (kSampleRate);

    SECTION ("Starts in Idle stage")
    {
        REQUIRE (env.getStage() == ADSREnvelope::Stage::Idle);
    }

    SECTION ("Output is zero when idle")
    {
        REQUIRE (env.processSample() == 0.0f);
    }

    SECTION ("isActive returns false when idle")
    {
        REQUIRE_FALSE (env.isActive());
    }
}

TEST_CASE ("ADSREnvelope note on triggers attack", "[dsp]")
{
    ADSREnvelope env;
    env.prepare (kSampleRate);
    env.setAttack (0.01f);
    env.setDecay (0.1f);
    env.setSustain (0.5f);
    env.setRelease (0.1f);

    env.noteOn (1.0f);

    SECTION ("Enters Attack stage")
    {
        REQUIRE (env.getStage() == ADSREnvelope::Stage::Attack);
    }

    SECTION ("isActive returns true")
    {
        REQUIRE (env.isActive());
    }

    SECTION ("Output rises during attack")
    {
        float first = env.processSample();
        float second = env.processSample();
        REQUIRE (second > first);
    }
}

TEST_CASE ("ADSREnvelope reaches sustain", "[dsp]")
{
    ADSREnvelope env;
    env.prepare (kSampleRate);
    env.setAttack (0.005f);
    env.setDecay (0.05f);
    env.setSustain (0.6f);
    env.setRelease (0.1f);

    env.noteOn (1.0f);

    // Run through attack and decay (~2.5s worth of samples to be safe)
    float level = 0.0f;
    for (int i = 0; i < static_cast<int> (kSampleRate * 2.0); ++i)
        level = env.processSample();

    SECTION ("Reaches sustain level")
    {
        REQUIRE (level == Catch::Approx (0.6f).margin (0.01));
    }

    SECTION ("Stage is Sustain")
    {
        REQUIRE (env.getStage() == ADSREnvelope::Stage::Sustain);
    }
}

TEST_CASE ("ADSREnvelope release", "[dsp]")
{
    ADSREnvelope env;
    env.prepare (kSampleRate);
    env.setAttack (0.005f);
    env.setDecay (0.01f);
    env.setSustain (0.5f);
    env.setRelease (0.05f);

    env.noteOn (1.0f);

    // Get to sustain
    for (int i = 0; i < static_cast<int> (kSampleRate * 0.5); ++i)
        env.processSample();

    env.noteOff();

    SECTION ("Enters Release stage")
    {
        REQUIRE (env.getStage() == ADSREnvelope::Stage::Release);
    }

    SECTION ("Output decreases during release")
    {
        float a = env.processSample();
        float b = env.processSample();
        REQUIRE (b < a);
    }

    SECTION ("Eventually returns to idle")
    {
        for (int i = 0; i < static_cast<int> (kSampleRate * 2.0); ++i)
            env.processSample();
        REQUIRE (env.getStage() == ADSREnvelope::Stage::Idle);
    }
}

TEST_CASE ("ADSREnvelope velocity scaling", "[dsp]")
{
    SECTION ("Half velocity produces half output at sustain")
    {
        ADSREnvelope fullVel, halfVel;
        fullVel.prepare (kSampleRate);
        halfVel.prepare (kSampleRate);

        for (auto* e : { &fullVel, &halfVel })
        {
            e->setAttack (0.005f);
            e->setDecay (0.01f);
            e->setSustain (0.8f);
            e->setRelease (0.1f);
        }

        fullVel.noteOn (1.0f);
        halfVel.noteOn (0.5f);

        float fLevel = 0.0f, hLevel = 0.0f;
        for (int i = 0; i < static_cast<int> (kSampleRate); ++i)
        {
            fLevel = fullVel.processSample();
            hLevel = halfVel.processSample();
        }

        REQUIRE (hLevel == Catch::Approx (fLevel * 0.5f).margin (0.01));
    }
}

TEST_CASE ("ADSREnvelope reset", "[dsp]")
{
    ADSREnvelope env;
    env.prepare (kSampleRate);
    env.noteOn (1.0f);
    for (int i = 0; i < 1000; ++i)
        env.processSample();

    env.reset();

    SECTION ("Returns to idle")
    {
        REQUIRE (env.getStage() == ADSREnvelope::Stage::Idle);
    }

    SECTION ("Output is zero")
    {
        REQUIRE (env.processSample() == 0.0f);
    }
}

TEST_CASE ("ADSREnvelope output is always finite", "[dsp]")
{
    ADSREnvelope env;
    env.prepare (kSampleRate);
    env.setAttack (0.001f);
    env.setDecay (0.001f);
    env.setSustain (0.0f);
    env.setRelease (0.001f);

    env.noteOn (1.0f);
    for (int i = 0; i < 10000; ++i)
    {
        float s = env.processSample();
        REQUIRE (std::isfinite (s));
        REQUIRE (s >= 0.0f);
        REQUIRE (s <= 1.01f);
    }
    env.noteOff();
    for (int i = 0; i < 10000; ++i)
    {
        float s = env.processSample();
        REQUIRE (std::isfinite (s));
        REQUIRE (s >= 0.0f);
    }
}
