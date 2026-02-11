#include "catch_amalgamated.hpp"
#include "DSP/SubOscillator.h"
#include <cmath>

static constexpr double kSampleRate = 44100.0;

TEST_CASE ("SubOscillator waveforms produce valid output", "[dsp]")
{
    SubOscillator sub;
    sub.prepare (kSampleRate);
    sub.setFrequency (440.0f);

    SECTION ("Sine waveform bounded to [-1, 1]")
    {
        sub.setWaveform (SubOscillator::Waveform::Sine);
        for (int i = 0; i < 10000; ++i)
        {
            float s = sub.processSample();
            REQUIRE (s >= -1.01f);
            REQUIRE (s <= 1.01f);
        }
    }

    SECTION ("Triangle waveform bounded")
    {
        sub.setWaveform (SubOscillator::Waveform::Triangle);
        for (int i = 0; i < 10000; ++i)
        {
            float s = sub.processSample();
            REQUIRE (s >= -1.1f);
            REQUIRE (s <= 1.1f);
        }
    }

    SECTION ("Square waveform bounded")
    {
        sub.setWaveform (SubOscillator::Waveform::Square);
        for (int i = 0; i < 10000; ++i)
        {
            float s = sub.processSample();
            REQUIRE (s >= -1.5f);
            REQUIRE (s <= 1.5f);
        }
    }

    SECTION ("Pulse waveform bounded")
    {
        sub.setWaveform (SubOscillator::Waveform::Pulse);
        sub.setPulseWidth (0.3f);
        for (int i = 0; i < 10000; ++i)
        {
            float s = sub.processSample();
            REQUIRE (s >= -1.5f);
            REQUIRE (s <= 1.5f);
        }
    }
}

TEST_CASE ("SubOscillator octave offset", "[dsp]")
{
    SECTION ("Octave -1 halves the frequency")
    {
        SubOscillator sub;
        sub.prepare (kSampleRate);
        sub.setWaveform (SubOscillator::Waveform::Sine);
        sub.setFrequency (440.0f);
        sub.setOctave (-1);

        // Count zero crossings in 1 second to estimate frequency
        int crossings = 0;
        float prev = sub.processSample();
        for (int i = 1; i < static_cast<int> (kSampleRate); ++i)
        {
            float curr = sub.processSample();
            if ((prev > 0.0f && curr <= 0.0f) || (prev < 0.0f && curr >= 0.0f))
                ++crossings;
            prev = curr;
        }
        // 220 Hz sine -> ~440 zero crossings per second (2 per cycle)
        REQUIRE (crossings >= 430);
        REQUIRE (crossings <= 450);
    }

    SECTION ("Octave -2 quarters the frequency")
    {
        SubOscillator sub;
        sub.prepare (kSampleRate);
        sub.setWaveform (SubOscillator::Waveform::Sine);
        sub.setFrequency (440.0f);
        sub.setOctave (-2);

        int crossings = 0;
        float prev = sub.processSample();
        for (int i = 1; i < static_cast<int> (kSampleRate); ++i)
        {
            float curr = sub.processSample();
            if ((prev > 0.0f && curr <= 0.0f) || (prev < 0.0f && curr >= 0.0f))
                ++crossings;
            prev = curr;
        }
        // 110 Hz sine -> ~220 zero crossings per second
        REQUIRE (crossings >= 215);
        REQUIRE (crossings <= 225);
    }
}
