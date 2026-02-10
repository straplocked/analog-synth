#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "DistortionEffect.h"
#include "ChorusEffect.h"
#include "DelayEffect.h"
#include "ReverbEffect.h"

class FXChain
{
public:
    void prepare (double sampleRate, int samplesPerBlock)
    {
        distortion.prepare (sampleRate, samplesPerBlock);
        chorus.prepare (sampleRate, samplesPerBlock);
        delay.prepare (sampleRate, samplesPerBlock);
        reverb.prepare (sampleRate, samplesPerBlock);
    }

    void updateParameters (juce::AudioProcessorValueTreeState& apvts);

    void processBlock (juce::AudioBuffer<float>& buffer)
    {
        distortion.processBlock (buffer);
        chorus.processBlock (buffer);
        delay.processBlock (buffer);
        reverb.processBlock (buffer);

        // NaN safety: if any FX produced bad output, clear and reset
        bool hasNaN = false;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                if (std::isnan (data[i]) || std::isinf (data[i]))
                {
                    hasNaN = true;
                    data[i] = 0.0f;
                }
            }
        }
        if (hasNaN)
        {
            chorus.reset();
            delay.reset();
            reverb.reset();
        }
    }

private:
    DistortionEffect distortion;
    ChorusEffect chorus;
    DelayEffect delay;
    ReverbEffect reverb;
};
