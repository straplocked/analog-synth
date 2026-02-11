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

TEST_CASE ("ADSREnvelope hold stage", "[dsp]")
{
    ADSREnvelope env;
    env.prepare (kSampleRate);
    env.setAttack (0.001f);
    env.setHold (0.01f); // 10ms hold
    env.setDecay (0.01f);
    env.setSustain (0.5f);
    env.setRelease (0.01f);

    env.noteOn (1.0f);

    SECTION ("Hold keeps level at 1.0 before decay")
    {
        // Run through attack
        for (int i = 0; i < 500; ++i)
            env.processSample();

        // Should be in Hold stage with level near 1.0
        bool foundHold = false;
        for (int i = 0; i < 500; ++i)
        {
            float s = env.processSample();
            if (env.getStage() == ADSREnvelope::Stage::Hold)
            {
                REQUIRE (s == Catch::Approx (1.0f).margin (0.01f));
                foundHold = true;
            }
        }
        REQUIRE (foundHold);
    }
}

TEST_CASE ("ADSREnvelope loop mode", "[dsp]")
{
    ADSREnvelope env;
    env.prepare (kSampleRate);
    env.setAttack (0.005f);
    env.setDecay (0.005f);
    env.setSustain (0.3f);
    env.setRelease (0.01f);

    SECTION ("Loop back to Attack after Decay (loopEnd=0)")
    {
        env.setLoopEnabled (true);
        env.setLoopStart (0);  // Attack
        env.setLoopEnd (0);    // After Decay

        env.noteOn (1.0f);

        // Run for 1 second — should cycle multiple times
        int attackCount = 0;
        ADSREnvelope::Stage prevStage = ADSREnvelope::Stage::Idle;
        for (int i = 0; i < static_cast<int> (kSampleRate); ++i)
        {
            float s = env.processSample();
            REQUIRE (std::isfinite (s));
            REQUIRE (s >= 0.0f);
            REQUIRE (s <= 1.01f);

            auto curStage = env.getStage();
            if (curStage == ADSREnvelope::Stage::Attack && prevStage != ADSREnvelope::Stage::Attack)
                ++attackCount;
            prevStage = curStage;
        }

        // Should have cycled at least 3 times in 1 second with these fast times
        REQUIRE (attackCount >= 3);
    }

    SECTION ("Loop back to Decay at Sustain (loopEnd=1)")
    {
        env.setLoopEnabled (true);
        env.setLoopStart (2);  // Decay
        env.setLoopEnd (1);    // At Sustain

        env.noteOn (1.0f);

        // Run for 1 second
        int decayCount = 0;
        ADSREnvelope::Stage prevStage = ADSREnvelope::Stage::Idle;
        for (int i = 0; i < static_cast<int> (kSampleRate); ++i)
        {
            float s = env.processSample();
            REQUIRE (std::isfinite (s));
            REQUIRE (s >= 0.0f);
            REQUIRE (s <= 1.01f);

            auto curStage = env.getStage();
            if (curStage == ADSREnvelope::Stage::Decay && prevStage != ADSREnvelope::Stage::Decay)
                ++decayCount;
            prevStage = curStage;
        }

        // Should have cycled through decay multiple times
        REQUIRE (decayCount >= 3);
    }

    SECTION ("Loop disabled does not cycle")
    {
        env.setLoopEnabled (false);
        env.noteOn (1.0f);

        // Run past attack+decay time
        for (int i = 0; i < static_cast<int> (kSampleRate); ++i)
            env.processSample();

        // Should be in Sustain (not cycling)
        REQUIRE (env.getStage() == ADSREnvelope::Stage::Sustain);
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
