#include "PluginEditor.h"

AnalogSynthEditor::AnalogSynthEditor (AnalogSynthProcessor& p)
    : AudioProcessorEditor (p)
{
    setLookAndFeel (&lookAndFeel);
    setSize (1200, 750);

    mainPanel.attach (p.apvts);
    mainPanel.setPresetManager (&p.presetManager);
    mainPanel.setKeyboardState (&p.keyboardState);
    addAndMakeVisible (mainPanel);
}

AnalogSynthEditor::~AnalogSynthEditor()
{
    setLookAndFeel (nullptr);
}

void AnalogSynthEditor::paint (juce::Graphics& g)
{
    g.fillAll (SynthColours::background);
}

void AnalogSynthEditor::resized()
{
    mainPanel.setBounds (getLocalBounds());
}
