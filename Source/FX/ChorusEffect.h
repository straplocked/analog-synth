#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

class ChorusEffect
{
public:
    void prepare (double sampleRate, int samplesPerBlock)
    {
        sr = sampleRate;

        for (int ch = 0; ch < 2; ++ch)
        {
            delayLine[ch].reset();
            delayLine[ch].setMaximumDelayInSamples (static_cast<int> (sampleRate * 0.05)); // 50ms max
        }

        juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 };
        for (int ch = 0; ch < 2; ++ch)
            delayLine[ch].prepare (spec);

        lfoPhase[0] = 0.0;
        lfoPhase[1] = 0.25; // 90 degrees offset for stereo
    }

    void setEnabled (bool e) { enabled = e; }
    void setRate (float r)   { rate = r; }
    void setDepth (float d)  { depth = d; }
    void setMix (float m)    { mix = m; }
    void setFlangerMode (bool f) { flangerMode = f; }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
            delayLine[ch].reset();
        lfoPhase[0] = 0.0;
        lfoPhase[1] = 0.25;
    }

    void processBlock (juce::AudioBuffer<float>& buffer)
    {
        if (! enabled) return;

        int numChannels = std::min (buffer.getNumChannels(), 2);
        int numSamples = buffer.getNumSamples();

        float baseDelay = flangerMode ? 0.002f : 0.007f; // 2ms flanger, 7ms chorus
        float modDepth = flangerMode ? 0.002f : 0.003f;  // modulation depth in seconds

        for (int i = 0; i < numSamples; ++i)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* data = buffer.getWritePointer (ch);
                float dry = data[i];

                // LFO modulation
                float lfoVal = std::sin (static_cast<float> (lfoPhase[ch] * 2.0 * M_PI));
                float delaySamples = (baseDelay + depth * modDepth * lfoVal) * static_cast<float> (sr);
                delaySamples = std::max (1.0f, delaySamples);

                delayLine[ch].pushSample (ch, dry);
                float wet = delayLine[ch].popSample (ch, delaySamples);

                data[i] = dry * (1.0f - mix) + wet * mix;
            }

            // Advance LFO phases
            double phaseInc = rate / sr;
            lfoPhase[0] += phaseInc;
            lfoPhase[1] += phaseInc;
            if (lfoPhase[0] >= 1.0) lfoPhase[0] -= 1.0;
            if (lfoPhase[1] >= 1.0) lfoPhase[1] -= 1.0;
        }
    }

private:
    bool enabled = false;
    float rate = 1.0f;
    float depth = 0.5f;
    float mix = 0.5f;
    bool flangerMode = false;
    double sr = 44100.0;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine[2];
    double lfoPhase[2] = {};
};
