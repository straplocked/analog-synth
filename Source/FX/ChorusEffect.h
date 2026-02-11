#pragma once

#include "FXBase.h"
#include "../ParameterIDs.h"
#include <juce_dsp/juce_dsp.h>
#include <cmath>

class ChorusEffect : public FXBase
{
public:
    ChorusEffect() = default;

    void prepare (double sampleRate, int samplesPerBlock) override
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

        bbdLPState[0] = 0.0f;
        bbdLPState[1] = 0.0f;
    }

    void processBlock (juce::AudioBuffer<float>& buffer) override
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

                // BBD input companding: gentle compression before delay
                float compressedDry = std::tanh (dry * 1.2f) * (1.0f / 1.2f);

                delayLine[ch].pushSample (ch, compressedDry);
                float wet = delayLine[ch].popSample (ch, delaySamples);

                // BBD lowpass: cutoff tracks inversely with delay time
                // Longer delay = lower BBD clock rate = more HF loss
                float bbdCutoffHz = static_cast<float> (sr) / (delaySamples * 0.5f);
                bbdCutoffHz = std::clamp (bbdCutoffHz, 2000.0f, 14000.0f);
                float bbdCoeff = 1.0f - std::exp (-2.0f * static_cast<float> (M_PI) * bbdCutoffHz / static_cast<float> (sr));
                bbdLPState[ch] += bbdCoeff * (wet - bbdLPState[ch]);
                wet = bbdLPState[ch];

                // BBD clock feedthrough noise (very subtle, ~-72dB)
                wet += bbdNoise() * 0.00025f * depth;

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

    void reset() override
    {
        for (int ch = 0; ch < 2; ++ch)
            delayLine[ch].reset();
        lfoPhase[0] = 0.0;
        lfoPhase[1] = 0.25;
        bbdLPState[0] = 0.0f;
        bbdLPState[1] = 0.0f;
    }

    void updateParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        setEnabled (apvts.getRawParameterValue (ParameterIDs::chorusEnabled)->load() > 0.5f);
        rate = apvts.getRawParameterValue (ParameterIDs::chorusRate)->load();
        depth = apvts.getRawParameterValue (ParameterIDs::chorusDepth)->load();
        mix = apvts.getRawParameterValue (ParameterIDs::chorusMix)->load();
        flangerMode = apvts.getRawParameterValue (ParameterIDs::chorusFlanger)->load() > 0.5f;
    }

    juce::String getEffectID() const override { return "chorus"; }
    juce::String getDisplayName() const override { return "CHORUS"; }

private:
    float rate = 1.0f;
    float depth = 0.5f;
    float mix = 0.5f;
    bool flangerMode = false;
    double sr = 44100.0;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine[2];
    double lfoPhase[2] = {};

    // BBD modeling state
    float bbdLPState[2] = {};
    uint32_t bbdNoiseState = 54321;

    float bbdNoise()
    {
        bbdNoiseState ^= bbdNoiseState << 13;
        bbdNoiseState ^= bbdNoiseState >> 17;
        bbdNoiseState ^= bbdNoiseState << 5;
        return static_cast<float> (static_cast<int32_t> (bbdNoiseState)) / static_cast<float> (INT32_MAX);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChorusEffect)
};
