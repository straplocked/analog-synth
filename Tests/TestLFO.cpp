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
#include "DSP/LFO.h"
#include <cmath>

static constexpr double kSampleRate = 44100.0;

TEST_CASE ("LFO Triangle shape", "[dsp]")
{
    LFO lfo;
    lfo.prepare (kSampleRate);
    lfo.setShape (static_cast<int> (LFO::Shape::Triangle));
    lfo.setRate (2.0f);

    SECTION ("Output bounded to [-1, 1]")
    {
        for (int i = 0; i < 100000; ++i)
        {
            float s = lfo.processSample();
            REQUIRE (s >= -1.01f);
            REQUIRE (s <= 1.01f);
        }
    }

    SECTION ("Reaches both extremes")
    {
        float minVal = 1.0f, maxVal = -1.0f;
        for (int i = 0; i < static_cast<int> (kSampleRate); ++i)
        {
            float s = lfo.processSample();
            minVal = std::min (minVal, s);
            maxVal = std::max (maxVal, s);
        }
        REQUIRE (maxVal > 0.9f);
        REQUIRE (minVal < -0.9f);
    }
}

TEST_CASE ("LFO Sine shape", "[dsp]")
{
    LFO lfo;
    lfo.prepare (kSampleRate);
    lfo.setShape (static_cast<int> (LFO::Shape::Sine));
    lfo.setRate (1.0f);

    SECTION ("Output bounded to [-1, 1]")
    {
        for (int i = 0; i < 100000; ++i)
        {
            float s = lfo.processSample();
            REQUIRE (s >= -1.01f);
            REQUIRE (s <= 1.01f);
        }
    }
}

TEST_CASE ("LFO Saw shape", "[dsp]")
{
    LFO lfo;
    lfo.prepare (kSampleRate);
    lfo.setShape (static_cast<int> (LFO::Shape::Saw));
    lfo.setRate (5.0f);

    SECTION ("Output bounded to [-1, 1]")
    {
        for (int i = 0; i < 100000; ++i)
        {
            float s = lfo.processSample();
            REQUIRE (s >= -1.01f);
            REQUIRE (s <= 1.01f);
        }
    }
}

TEST_CASE ("LFO Square shape", "[dsp]")
{
    LFO lfo;
    lfo.prepare (kSampleRate);
    lfo.setShape (static_cast<int> (LFO::Shape::Square));
    lfo.setRate (2.0f);

    SECTION ("Output near +/-1 (with slew)")
    {
        // Square uses slew, so it won't be exactly +/-1 but should reach near it
        float minVal = 1.0f, maxVal = -1.0f;
        for (int i = 0; i < static_cast<int> (kSampleRate); ++i)
        {
            float s = lfo.processSample();
            minVal = std::min (minVal, s);
            maxVal = std::max (maxVal, s);
        }
        REQUIRE (maxVal > 0.9f);
        REQUIRE (minVal < -0.9f);
    }
}

TEST_CASE ("LFO Random (S&H) shape", "[dsp]")
{
    LFO lfo;
    lfo.prepare (kSampleRate);
    lfo.setShape (static_cast<int> (LFO::Shape::Random));
    lfo.setRate (10.0f);

    SECTION ("Output bounded")
    {
        for (int i = 0; i < 100000; ++i)
        {
            float s = lfo.processSample();
            REQUIRE (s >= -1.5f);
            REQUIRE (s <= 1.5f);
        }
    }
}

TEST_CASE ("LFO rate affects frequency", "[dsp]")
{
    SECTION ("Higher rate means more cycles per second")
    {
        LFO slow, fast;
        slow.prepare (kSampleRate);
        fast.prepare (kSampleRate);
        slow.setShape (static_cast<int> (LFO::Shape::Triangle));
        fast.setShape (static_cast<int> (LFO::Shape::Triangle));
        slow.setRate (1.0f);
        fast.setRate (10.0f);

        // Count zero crossings as proxy for frequency
        auto countCrossings = [] (LFO& lfo, int samples) {
            int crossings = 0;
            float prev = lfo.processSample();
            for (int i = 1; i < samples; ++i)
            {
                float curr = lfo.processSample();
                if ((prev > 0.0f && curr <= 0.0f) || (prev < 0.0f && curr >= 0.0f))
                    ++crossings;
                prev = curr;
            }
            return crossings;
        };

        int slowCrossings = countCrossings (slow, static_cast<int> (kSampleRate));
        int fastCrossings = countCrossings (fast, static_cast<int> (kSampleRate));

        REQUIRE (fastCrossings > slowCrossings * 5);
    }
}

TEST_CASE ("LFO Custom shape", "[dsp]")
{
    LFO lfo;
    lfo.prepare (kSampleRate);
    lfo.setShape (static_cast<int> (LFO::Shape::Custom));
    lfo.setRate (2.0f);

    SECTION ("Default ramp outputs bounded values")
    {
        // Default custom shape is ramp from -1 to +1
        for (int i = 0; i < 100000; ++i)
        {
            float s = lfo.processSample();
            REQUIRE (s >= -1.01f);
            REQUIRE (s <= 1.01f);
        }
    }

    SECTION ("Custom shape with 3 points interpolates correctly")
    {
        // Set up a V shape: (-1 at 0, +1 at 0.5, -1 at 1.0)
        lfo.setCustomNumPoints (3);
        lfo.setCustomPoint (0, 0.0f, -1.0f);
        lfo.setCustomPoint (1, 0.5f, 1.0f);
        lfo.setCustomPoint (2, 1.0f, -1.0f);

        float minVal = 1.0f, maxVal = -1.0f;
        for (int i = 0; i < static_cast<int> (kSampleRate); ++i)
        {
            float s = lfo.processSample();
            minVal = std::min (minVal, s);
            maxVal = std::max (maxVal, s);
            REQUIRE (s >= -1.01f);
            REQUIRE (s <= 1.01f);
        }
        REQUIRE (maxVal > 0.9f);
        REQUIRE (minVal < -0.9f);
    }
}

TEST_CASE ("LFO StepSeq shape", "[dsp]")
{
    LFO lfo;
    lfo.prepare (kSampleRate);
    lfo.setShape (static_cast<int> (LFO::Shape::StepSeq));
    lfo.setRate (2.0f);

    SECTION ("Step values are output correctly")
    {
        // Set alternating high/low pattern
        lfo.setStepSeqNumSteps (4);
        lfo.setStepValue (0, 1.0f);
        lfo.setStepValue (1, -1.0f);
        lfo.setStepValue (2, 0.5f);
        lfo.setStepValue (3, -0.5f);

        float minVal = 1.0f, maxVal = -1.0f;
        // Output uses slew, so values will approach but may not exactly match step values
        for (int i = 0; i < static_cast<int> (kSampleRate); ++i)
        {
            float s = lfo.processSample();
            minVal = std::min (minVal, s);
            maxVal = std::max (maxVal, s);
            REQUIRE (s >= -1.5f);
            REQUIRE (s <= 1.5f);
        }
        REQUIRE (maxVal > 0.4f);
        REQUIRE (minVal < -0.4f);
    }

    SECTION ("Step smooth mode outputs bounded values")
    {
        lfo.setStepSmooth (true);
        lfo.setStepSeqNumSteps (8);
        for (int i = 0; i < 8; ++i)
            lfo.setStepValue (i, static_cast<float> (i) / 7.0f * 2.0f - 1.0f);

        for (int i = 0; i < 100000; ++i)
        {
            float s = lfo.processSample();
            REQUIRE (s >= -1.5f);
            REQUIRE (s <= 1.5f);
        }
    }
}

TEST_CASE ("LFO voice phase offset", "[dsp]")
{
    SECTION ("Different offsets produce different phases")
    {
        LFO lfo1, lfo2;
        lfo1.prepare (kSampleRate);
        lfo2.prepare (kSampleRate);
        lfo1.setShape (static_cast<int> (LFO::Shape::Sine));
        lfo2.setShape (static_cast<int> (LFO::Shape::Sine));
        lfo1.setRate (1.0f);
        lfo2.setRate (1.0f);

        lfo1.setVoicePhaseOffset (0.0);
        lfo2.setVoicePhaseOffset (0.25);
        lfo1.resetPhase();
        lfo2.resetPhase();

        // Advance a few samples so phases diverge from sin(0)/sin(pi/2) symmetry
        for (int i = 0; i < 100; ++i)
        {
            lfo1.processSample();
            lfo2.processSample();
        }

        float s1 = lfo1.processSample();
        float s2 = lfo2.processSample();

        REQUIRE (s1 != Catch::Approx (s2).margin (0.01));
    }
}
