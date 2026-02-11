#include "catch_amalgamated.hpp"
#include "DSP/PolyBLEPOscillator.h"
#include <cmath>
#include <numeric>
#include <vector>

static constexpr double kSampleRate = 44100.0;

TEST_CASE ("PolyBLEPOscillator preparation", "[dsp]")
{
    PolyBLEPOscillator osc;
    osc.prepare (kSampleRate);
    osc.setFrequency (440.0f);
    osc.setWaveform (PolyBLEPOscillator::Waveform::Saw);

    SECTION ("Produces non-zero output")
    {
        bool hasNonZero = false;
        for (int i = 0; i < 1000; ++i)
        {
            if (osc.processSample() != 0.0f)
                hasNonZero = true;
        }
        REQUIRE (hasNonZero);
    }
}

TEST_CASE ("PolyBLEP Saw waveform", "[dsp]")
{
    PolyBLEPOscillator osc;
    osc.prepare (kSampleRate);
    osc.setFrequency (440.0f);
    osc.setWaveform (PolyBLEPOscillator::Waveform::Saw);

    SECTION ("Output bounded to [-1, 1]")
    {
        for (int i = 0; i < 10000; ++i)
        {
            float s = osc.processSample();
            REQUIRE (s >= -1.1f);
            REQUIRE (s <= 1.1f);
        }
    }

    SECTION ("DC offset near zero over full cycles")
    {
        int samplesPerCycle = static_cast<int> (kSampleRate / 440.0);
        int totalSamples = samplesPerCycle * 100;
        double sum = 0.0;
        for (int i = 0; i < totalSamples; ++i)
            sum += osc.processSample();
        double dc = sum / totalSamples;
        REQUIRE (std::abs (dc) < 0.02);
    }
}

TEST_CASE ("PolyBLEP Square waveform", "[dsp]")
{
    PolyBLEPOscillator osc;
    osc.prepare (kSampleRate);
    osc.setFrequency (440.0f);
    osc.setWaveform (PolyBLEPOscillator::Waveform::Square);

    SECTION ("Output bounded")
    {
        for (int i = 0; i < 10000; ++i)
        {
            float s = osc.processSample();
            REQUIRE (s >= -1.2f);
            REQUIRE (s <= 1.2f);
        }
    }

    SECTION ("DC offset near zero")
    {
        int samplesPerCycle = static_cast<int> (kSampleRate / 440.0);
        int totalSamples = samplesPerCycle * 100;
        double sum = 0.0;
        for (int i = 0; i < totalSamples; ++i)
            sum += osc.processSample();
        double dc = sum / totalSamples;
        REQUIRE (std::abs (dc) < 0.05);
    }
}

TEST_CASE ("PolyBLEP Triangle waveform", "[dsp]")
{
    PolyBLEPOscillator osc;
    osc.prepare (kSampleRate);
    osc.setFrequency (440.0f);
    osc.setWaveform (PolyBLEPOscillator::Waveform::Triangle);

    SECTION ("Output bounded to [-1, 1]")
    {
        for (int i = 0; i < 10000; ++i)
        {
            float s = osc.processSample();
            REQUIRE (s >= -1.1f);
            REQUIRE (s <= 1.1f);
        }
    }
}

TEST_CASE ("PolyBLEP Pulse waveform", "[dsp]")
{
    PolyBLEPOscillator osc;
    osc.prepare (kSampleRate);
    osc.setFrequency (440.0f);
    osc.setWaveform (PolyBLEPOscillator::Waveform::Pulse);
    osc.setPulseWidth (0.25f);

    SECTION ("Output bounded")
    {
        for (int i = 0; i < 10000; ++i)
        {
            float s = osc.processSample();
            REQUIRE (s >= -1.5f);
            REQUIRE (s <= 1.5f);
        }
    }

    SECTION ("Pulse width affects DC offset")
    {
        // Narrow pulse should have negative DC offset
        int samplesPerCycle = static_cast<int> (kSampleRate / 440.0);
        int totalSamples = samplesPerCycle * 100;
        double sum = 0.0;
        for (int i = 0; i < totalSamples; ++i)
            sum += osc.processSample();
        double dc = sum / totalSamples;
        // 25% pulse width: roughly (0.25 - 0.75) = -0.5 DC in naive case
        REQUIRE (dc < 0.0);
    }
}

TEST_CASE ("PolyBLEP phase wrap detection", "[dsp]")
{
    PolyBLEPOscillator osc;
    osc.prepare (kSampleRate);
    osc.setFrequency (440.0f);
    osc.setWaveform (PolyBLEPOscillator::Waveform::Saw);

    SECTION ("Phase wraps occur at expected rate")
    {
        int wraps = 0;
        int totalSamples = static_cast<int> (kSampleRate); // 1 second
        for (int i = 0; i < totalSamples; ++i)
        {
            osc.processSample();
            if (osc.didPhaseWrap())
                ++wraps;
        }
        // 440 Hz -> should wrap ~440 times per second
        REQUIRE (wraps >= 438);
        REQUIRE (wraps <= 442);
    }
}

TEST_CASE ("PolyBLEP pulse width clamping", "[dsp]")
{
    PolyBLEPOscillator osc;
    osc.prepare (kSampleRate);
    osc.setFrequency (440.0f);
    osc.setWaveform (PolyBLEPOscillator::Waveform::Pulse);

    SECTION ("Extreme pulse widths are clamped")
    {
        osc.setPulseWidth (0.0f);
        // Should not crash, output should be valid
        for (int i = 0; i < 1000; ++i)
        {
            float s = osc.processSample();
            REQUIRE (std::isfinite (s));
        }

        osc.setPulseWidth (1.0f);
        for (int i = 0; i < 1000; ++i)
        {
            float s = osc.processSample();
            REQUIRE (std::isfinite (s));
        }
    }
}

TEST_CASE ("PolyBLEP renderBlock", "[dsp]")
{
    PolyBLEPOscillator osc;
    osc.prepare (kSampleRate);
    osc.setFrequency (440.0f);
    osc.setWaveform (PolyBLEPOscillator::Waveform::Saw);

    SECTION ("renderBlock matches processSample")
    {
        PolyBLEPOscillator osc2;
        osc2.prepare (kSampleRate);
        osc2.setFrequency (440.0f);
        osc2.setWaveform (PolyBLEPOscillator::Waveform::Saw);

        constexpr int N = 256;
        float blockOutput[N];
        osc.renderBlock (blockOutput, N);

        for (int i = 0; i < N; ++i)
        {
            float sampleOutput = osc2.processSample();
            REQUIRE (blockOutput[i] == Catch::Approx (sampleOutput));
        }
    }
}
