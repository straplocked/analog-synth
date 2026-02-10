#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/AnalogSynthLookAndFeel.h"
#include "UI/MainPanel.h"

class AnalogSynthEditor : public juce::AudioProcessorEditor
{
public:
    explicit AnalogSynthEditor (AnalogSynthProcessor&);
    ~AnalogSynthEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    AnalogSynthProcessor& getProcessor() { return *static_cast<AnalogSynthProcessor*> (getAudioProcessor()); }

    WaveformDisplay& getWaveformDisplay() { return mainPanel.getWaveformDisplay(); }

    float getPitchBend() const { return mainPanel.getPitchBend(); }
    float getModWheel() const  { return mainPanel.getModWheel(); }

private:
    AnalogSynthLookAndFeel lookAndFeel;
    MainPanel mainPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalogSynthEditor)
};
