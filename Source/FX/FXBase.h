#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

class FXBase
{
public:
    virtual ~FXBase() = default;

    virtual void prepare (double sampleRate, int samplesPerBlock) = 0;
    virtual void processBlock (juce::AudioBuffer<float>& buffer) = 0;
    virtual void reset() = 0;
    virtual void updateParameters (juce::AudioProcessorValueTreeState& apvts) = 0;

    virtual juce::String getEffectID() const = 0;
    virtual juce::String getDisplayName() const = 0;

    void setEnabled (bool e) { enabled = e; }
    bool isEnabled() const { return enabled; }

protected:
    bool enabled = false;
};
