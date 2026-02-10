#pragma once

#include "FXBase.h"
#include "../ParameterIDs.h"
#include <juce_dsp/juce_dsp.h>
#include <cmath>

class DelayEffect : public FXBase
{
public:
    DelayEffect() = default;

    void prepare (double sampleRate, int samplesPerBlock) override
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

        // Ping-pong high-frequency rolloff filter
        ppFilterState[0] = 0.0f;
        ppFilterState[1] = 0.0f;
        ppFilterCoeff = 1.0f - std::exp (-2.0f * static_cast<float> (M_PI) * 6000.0f / static_cast<float> (sampleRate));
    }

    void setBpm (double newBpm) { bpm = newBpm; }

    void reset() override
    {
        for (int ch = 0; ch < 2; ++ch)
            delayLine[ch].reset();
        bbdFilterState[0] = 0.0f;
        bbdFilterState[1] = 0.0f;
        ppFilterState[0] = 0.0f;
        ppFilterState[1] = 0.0f;
    }

    void processBlock (juce::AudioBuffer<float>& buffer) override
    {
        if (! enabled) return;

        int numSamples = buffer.getNumSamples();

        // Compute effective delay time
        float effectiveDelayMs = delayTimeMs;
        if (syncEnabled && bpm > 0.0)
        {
            double beats = syncDivisionToBeats (syncDivIndex);
            effectiveDelayMs = static_cast<float> ((60000.0 / bpm) * beats);
            effectiveDelayMs = juce::jlimit (1.0f, 2500.0f, effectiveDelayMs);
        }

        float delaySamples = effectiveDelayMs * 0.001f * static_cast<float> (sr);
        delaySamples = std::max (1.0f, delaySamples);

        float* left  = buffer.getWritePointer (0);
        float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

        for (int i = 0; i < numSamples; ++i)
        {
            float dryL = left[i];
            float dryR = right ? right[i] : dryL;

            float wetL = delayLine[0].popSample (0, delaySamples);
            float wetR = delayLine[1].popSample (0, delaySamples);

            if (pingPong)
            {
                // True ping-pong: mono sum → left delay, left output → right delay
                float monoIn = (dryL + dryR) * 0.5f;

                // Apply feedback with cross-feed
                float fbL = wetL * feedback;
                float fbR = wetR * feedback;

                // Per-bounce high-frequency rolloff (always active in ping-pong)
                if (bbdMode)
                {
                    // Harsher filtering with BBD (3kHz)
                    bbdFilterState[0] += bbdFilterCoeff * (fbR - bbdFilterState[0]);
                    fbR = bbdFilterState[0];
                    bbdFilterState[1] += bbdFilterCoeff * (fbL - bbdFilterState[1]);
                    fbL = bbdFilterState[1];
                }
                else
                {
                    // Gentle rolloff (6kHz) for natural darkening
                    ppFilterState[0] += ppFilterCoeff * (fbR - ppFilterState[0]);
                    fbR = ppFilterState[0];
                    ppFilterState[1] += ppFilterCoeff * (fbL - ppFilterState[1]);
                    fbL = ppFilterState[1];
                }

                // Left delay: mono input + right's feedback (cross-feed)
                delayLine[0].pushSample (0, monoIn + fbR);
                // Right delay: fed only by left delay output (true alternating bounce)
                delayLine[1].pushSample (0, fbL);

                // Width controls stereo spread of wet signal
                // width=1: full L/R separation, width=0: mono center
                float panL = 0.5f - width * 0.5f;
                float panR = 0.5f + width * 0.5f;
                float wetOutL = wetL * panR + wetR * panL;
                float wetOutR = wetR * panR + wetL * panL;

                left[i]  = dryL * (1.0f - mix) + wetOutL * mix;
                if (right)
                    right[i] = dryR * (1.0f - mix) + wetOutR * mix;
            }
            else
            {
                // Standard stereo delay
                float fbL = wetL * feedback;
                float fbR = wetR * feedback;

                if (bbdMode)
                {
                    bbdFilterState[0] += bbdFilterCoeff * (fbL - bbdFilterState[0]);
                    fbL = bbdFilterState[0];
                    bbdFilterState[1] += bbdFilterCoeff * (fbR - bbdFilterState[1]);
                    fbR = bbdFilterState[1];
                }

                delayLine[0].pushSample (0, dryL + fbL);
                delayLine[1].pushSample (0, dryR + fbR);

                left[i] = dryL * (1.0f - mix) + wetL * mix;
                if (right)
                    right[i] = dryR * (1.0f - mix) + wetR * mix;
            }
        }
    }

    void updateParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        setEnabled (apvts.getRawParameterValue (ParameterIDs::delayEnabled)->load() > 0.5f);
        delayTimeMs = apvts.getRawParameterValue (ParameterIDs::delayTime)->load();
        feedback = apvts.getRawParameterValue (ParameterIDs::delayFeedback)->load();
        mix = apvts.getRawParameterValue (ParameterIDs::delayMix)->load();
        syncEnabled = apvts.getRawParameterValue (ParameterIDs::delaySync)->load() > 0.5f;
        syncDivIndex = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::delaySyncDiv)->load());
        pingPong = apvts.getRawParameterValue (ParameterIDs::delayPingPong)->load() > 0.5f;
        bbdMode = apvts.getRawParameterValue (ParameterIDs::delayBBD)->load() > 0.5f;
        width = apvts.getRawParameterValue (ParameterIDs::delayWidth)->load();
    }

    juce::String getEffectID() const override { return "delay"; }
    juce::String getDisplayName() const override { return "DELAY"; }

private:
    float delayTimeMs = 400.0f;
    float feedback = 0.4f;
    float mix = 0.3f;
    bool syncEnabled = false;
    int syncDivIndex = 6;
    bool pingPong = false;
    bool bbdMode = false;
    float width = 1.0f;
    double sr = 44100.0;
    double bpm = 120.0;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine[2];

    // BBD low-pass filter (3kHz)
    float bbdFilterState[2] = {};
    float bbdFilterCoeff = 0.1f;

    // Ping-pong gentle rolloff filter (6kHz)
    float ppFilterState[2] = {};
    float ppFilterCoeff = 0.1f;

    static double syncDivisionToBeats (int divIndex)
    {
        switch (divIndex)
        {
            case 0: return 0.125;       // 1/32
            case 1: return 1.0 / 6.0;   // 1/16T
            case 2: return 0.25;         // 1/16
            case 3: return 1.0 / 3.0;   // 1/8T
            case 4: return 0.5;          // 1/8
            case 5: return 2.0 / 3.0;   // 1/4T
            case 6: return 1.0;          // 1/4
            case 7: return 2.0;          // 1/2
            case 8: return 4.0;          // 1 Bar
            default: return 1.0;
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayEffect)
};
