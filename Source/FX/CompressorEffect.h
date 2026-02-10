#pragma once

#include "FXBase.h"
#include "../ParameterIDs.h"
#include <cmath>

class CompressorEffect : public FXBase
{
public:
    CompressorEffect() = default;

    void prepare (double sampleRate, int /*samplesPerBlock*/) override
    {
        sr = sampleRate;
        envelope[0] = -100.0f;
        envelope[1] = -100.0f;
        computeCoefficients();
    }

    void processBlock (juce::AudioBuffer<float>& buffer) override
    {
        if (! enabled) return;

        int numSamples = buffer.getNumSamples();
        int numChannels = buffer.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* data = buffer.getWritePointer (ch);
                float dry = data[i];

                // Peak detection in dB
                float inputLevel = std::abs (dry);
                float levelDb = 20.0f * std::log10 (inputLevel + 1e-10f);

                // Envelope follower
                float coeff = (levelDb > envelope[ch]) ? attackCoeff : releaseCoeff;
                envelope[ch] += coeff * (levelDb - envelope[ch]);

                // Gain reduction
                float gr = computeGainReduction (envelope[ch]);
                float gainDb = -gr + makeupGain;
                float gainLinear = std::pow (10.0f, gainDb / 20.0f);

                float wet = dry * gainLinear;

                // Dry/wet mix
                data[i] = dry * (1.0f - mix) + wet * mix;
            }
        }
    }

    void reset() override
    {
        envelope[0] = -100.0f;
        envelope[1] = -100.0f;
    }

    void updateParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        setEnabled (apvts.getRawParameterValue (ParameterIDs::compEnabled)->load() > 0.5f);
        threshold = apvts.getRawParameterValue (ParameterIDs::compThreshold)->load();
        ratio = apvts.getRawParameterValue (ParameterIDs::compRatio)->load();
        attackMs = apvts.getRawParameterValue (ParameterIDs::compAttack)->load();
        releaseMs = apvts.getRawParameterValue (ParameterIDs::compRelease)->load();
        makeupGain = apvts.getRawParameterValue (ParameterIDs::compMakeup)->load();
        mix = apvts.getRawParameterValue (ParameterIDs::compMix)->load();
        computeCoefficients();
    }

    juce::String getEffectID() const override { return "compressor"; }
    juce::String getDisplayName() const override { return "COMP"; }

private:
    float threshold = -20.0f;
    float ratio = 4.0f;
    float attackMs = 10.0f;
    float releaseMs = 100.0f;
    float makeupGain = 0.0f;
    float mix = 1.0f;
    double sr = 44100.0;

    float envelope[2] = { -100.0f, -100.0f };
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    static constexpr float kneeWidth = 6.0f; // dB

    void computeCoefficients()
    {
        attackCoeff = 1.0f - std::exp (-1.0f / (attackMs * 0.001f * static_cast<float> (sr)));
        releaseCoeff = 1.0f - std::exp (-1.0f / (releaseMs * 0.001f * static_cast<float> (sr)));
    }

    float computeGainReduction (float envDb) const
    {
        float over = envDb - threshold;
        if (over <= -kneeWidth * 0.5f)
            return 0.0f;
        if (over >= kneeWidth * 0.5f)
            return over * (1.0f - 1.0f / ratio);
        // Soft knee region
        float x = over + kneeWidth * 0.5f;
        return (x * x) / (2.0f * kneeWidth) * (1.0f - 1.0f / ratio);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorEffect)
};
