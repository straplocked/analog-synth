#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>

class ReverbEffect
{
public:
    void prepare (double sampleRate, int samplesPerBlock)
    {
        juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 };
        reverb.prepare (spec);
        sr = sampleRate;
    }

    void setEnabled (bool e) { enabled = e; }
    void setType (int typeIndex) { reverbType = typeIndex; updateParams(); }
    void setSize (float s)    { size = s; updateParams(); }
    void setDamping (float d) { damping = d; updateParams(); }
    void setPreDelay (float /*ms*/) { /* TODO: implement pre-delay buffer */ }
    void setMix (float m)     { wetLevel = m; updateParams(); }

    void reset()
    {
        reverb.reset();
    }

    void processBlock (juce::AudioBuffer<float>& buffer)
    {
        if (! enabled) return;

        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> context (block);
        reverb.process (context);
    }

private:
    bool enabled = false;
    int reverbType = 0;
    float size = 0.5f;
    float damping = 0.5f;
    float wetLevel = 0.2f;
    double sr = 44100.0;

    juce::dsp::Reverb reverb;

    void updateParams()
    {
        juce::dsp::Reverb::Parameters params;

        // Adjust parameters based on reverb type
        switch (reverbType)
        {
            case 0: // Room
                params.roomSize = 0.2f + size * 0.4f;
                params.damping = 0.3f + damping * 0.5f;
                break;
            case 1: // Hall
                params.roomSize = 0.5f + size * 0.5f;
                params.damping = 0.1f + damping * 0.4f;
                break;
            case 2: // Plate
                params.roomSize = 0.4f + size * 0.4f;
                params.damping = 0.1f + damping * 0.3f;
                break;
            case 3: // Spring
                params.roomSize = 0.3f + size * 0.3f;
                params.damping = 0.2f + damping * 0.6f;
                break;
            default:
                break;
        }

        params.wetLevel = wetLevel;
        params.dryLevel = 1.0f - wetLevel;
        params.width = 1.0f;
        params.freezeMode = 0.0f;

        reverb.setParameters (params);
    }
};
