#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

class DelayEffect
{
public:
    void prepare (double sampleRate, int samplesPerBlock)
    {
        sr = sampleRate;

        for (int ch = 0; ch < 2; ++ch)
        {
            delayLine[ch].reset();
            delayLine[ch].setMaximumDelayInSamples (static_cast<int> (sampleRate * 2.5)); // 2.5s max
        }

        juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock), 1 };
        for (int ch = 0; ch < 2; ++ch)
            delayLine[ch].prepare (spec);

        // BBD low-pass filter in feedback
        bbdFilterState[0] = 0.0f;
        bbdFilterState[1] = 0.0f;
        bbdFilterCoeff = 1.0f - std::exp (-2.0f * static_cast<float> (M_PI) * 3000.0f / static_cast<float> (sampleRate));
    }

    void setEnabled (bool e)     { enabled = e; }
    void setDelayTime (float ms) { delayTimeMs = ms; }
    void setFeedback (float fb)  { feedback = fb; }
    void setMix (float m)        { mix = m; }
    void setPingPong (bool pp)   { pingPong = pp; }
    void setBBD (bool b)         { bbdMode = b; }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
            delayLine[ch].reset();
        bbdFilterState[0] = 0.0f;
        bbdFilterState[1] = 0.0f;
    }

    void processBlock (juce::AudioBuffer<float>& buffer)
    {
        if (! enabled) return;

        int numSamples = buffer.getNumSamples();
        float delaySamples = delayTimeMs * 0.001f * static_cast<float> (sr);
        delaySamples = std::max (1.0f, delaySamples);

        float* left  = buffer.getWritePointer (0);
        float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

        for (int i = 0; i < numSamples; ++i)
        {
            float dryL = left[i];
            float dryR = right ? right[i] : dryL;

            float wetL = delayLine[0].popSample (0, delaySamples);
            float wetR = delayLine[1].popSample (0, delaySamples);

            // Apply BBD low-pass filter to feedback signal
            float fbL = wetL * feedback;
            float fbR = wetR * feedback;

            if (bbdMode)
            {
                bbdFilterState[0] += bbdFilterCoeff * (fbL - bbdFilterState[0]);
                fbL = bbdFilterState[0];
                bbdFilterState[1] += bbdFilterCoeff * (fbR - bbdFilterState[1]);
                fbR = bbdFilterState[1];
            }

            if (pingPong)
            {
                // Cross-feed: left delay feeds right, right feeds left
                delayLine[0].pushSample (0, dryL + fbR);
                delayLine[1].pushSample (0, dryR + fbL);
            }
            else
            {
                delayLine[0].pushSample (0, dryL + fbL);
                delayLine[1].pushSample (0, dryR + fbR);
            }

            left[i] = dryL * (1.0f - mix) + wetL * mix;
            if (right)
                right[i] = dryR * (1.0f - mix) + wetR * mix;
        }
    }

private:
    bool enabled = false;
    float delayTimeMs = 400.0f;
    float feedback = 0.4f;
    float mix = 0.3f;
    bool pingPong = false;
    bool bbdMode = false;
    double sr = 44100.0;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine[2];

    // BBD low-pass filter
    float bbdFilterState[2] = {};
    float bbdFilterCoeff = 0.1f;
};
