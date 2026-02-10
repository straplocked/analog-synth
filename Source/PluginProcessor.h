#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/Arpeggiator.h"
#include "DSP/VoiceManager.h"
#include "FX/FXChain.h"
#include "ParameterIDs.h"
#include "PresetManager.h"

class AnalogSynthProcessor : public juce::AudioProcessor
{
public:
    AnalogSynthProcessor();
    ~AnalogSynthProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    PresetManager presetManager { apvts };
    juce::MidiKeyboardState keyboardState;

private:
    VoiceManager voiceManager;
    Arpeggiator arpeggiator;
    FXChain fxChain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalogSynthProcessor)
};
