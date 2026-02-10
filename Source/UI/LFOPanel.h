#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "AnalogSynthLookAndFeel.h"
#include "RotaryKnob.h"
#include "../ParameterIDs.h"

class LFOPanel : public juce::Component
{
public:
    LFOPanel()
    {
        shapeSelector.addItemList ({"Triangle", "Saw", "Square", "S&H", "Sine"}, 1);
        shapeSelector.setSelectedId (1, juce::dontSendNotification);
        addAndMakeVisible (shapeSelector);

        rateKnob     = std::make_unique<RotaryKnob> ("RATE", "Hz");
        toOscFreq    = std::make_unique<RotaryKnob> ("OSC FREQ", "");
        toOscShape   = std::make_unique<RotaryKnob> ("OSC SHAPE", "");
        toFilter     = std::make_unique<RotaryKnob> ("FILTER", "");
        toAmp        = std::make_unique<RotaryKnob> ("AMP", "");

        for (auto* k : getKnobs())
        {
            k->setAccentColour (SynthColours::sectionLfo);
            addAndMakeVisible (*k);
        }

        syncButton.setButtonText ("SYNC");
        syncButton.setClickingTogglesState (true);
        syncButton.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        syncButton.setColour (juce::TextButton::buttonOnColourId, SynthColours::sectionLfo.withAlpha (0.3f));
        addAndMakeVisible (syncButton);

        keySyncButton.setButtonText ("KEY SYNC");
        keySyncButton.setClickingTogglesState (true);
        keySyncButton.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        keySyncButton.setColour (juce::TextButton::buttonOnColourId, SynthColours::sectionLfo.withAlpha (0.3f));
        addAndMakeVisible (keySyncButton);

        syncDivSelector.addItemList ({"1/1", "1/2", "1/4", "1/8", "1/16", "1/32"}, 1);
        syncDivSelector.setSelectedId (3, juce::dontSendNotification);
        addAndMakeVisible (syncDivSelector);
    }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        shapeAttach   = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, ParameterIDs::lfoShape, shapeSelector);
        syncDivAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, ParameterIDs::lfoSyncDiv, syncDivSelector);
        syncAttach    = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::lfoSync, syncButton);
        keySyncAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::lfoKeySync, keySyncButton);

        rateKnob->attach (apvts, ParameterIDs::lfoRate);
        toOscFreq->attach (apvts, ParameterIDs::lfoToOscFreq);
        toOscShape->attach (apvts, ParameterIDs::lfoToOscShape);
        toFilter->attach (apvts, ParameterIDs::lfoToFilter);
        toAmp->attach (apvts, ParameterIDs::lfoToAmp);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Brushed metal background
        AnalogSynthLookAndFeel::fillBrushedMetal (g, bounds, SynthColours::faceplateDark);

        // Silkscreen title with section color
        auto titleArea = bounds.reduced (10, 4).removeFromTop (16);
        g.setFont (juce::Font (11.0f).boldened());
        g.setColour (juce::Colours::black.withAlpha (0.3f));
        g.drawText ("LFO", titleArea.translated (0, -0.5f), juce::Justification::centredLeft, false);
        g.setColour (SynthColours::sectionLfo);
        g.drawText ("LFO", titleArea, juce::Justification::centredLeft, false);

        // Horizontal rule
        g.setColour (SynthColours::silkscreen.withAlpha (0.15f));
        g.fillRect (bounds.getX() + 8.0f, titleArea.getBottom() + 1.0f, bounds.getWidth() - 16.0f, 1.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (8);
        bounds.removeFromTop (20);

        // Top row: shape selector, sync controls
        auto topRow = bounds.removeFromTop (26);
        shapeSelector.setBounds (topRow.removeFromLeft (120).reduced (2, 0));
        topRow.removeFromLeft (8);
        syncButton.setBounds (topRow.removeFromLeft (60).reduced (2, 0));
        syncDivSelector.setBounds (topRow.removeFromLeft (80).reduced (2, 0));
        topRow.removeFromLeft (8);
        keySyncButton.setBounds (topRow.removeFromLeft (80).reduced (2, 0));

        bounds.removeFromTop (6);

        // Knob row
        auto knobs = getKnobs();
        int knobW = bounds.getWidth() / static_cast<int> (knobs.size());
        for (int i = 0; i < static_cast<int> (knobs.size()); ++i)
            knobs[static_cast<size_t> (i)]->setBounds (bounds.getX() + i * knobW, bounds.getY(),
                                                        knobW, bounds.getHeight());
    }

private:
    std::vector<RotaryKnob*> getKnobs()
    {
        return { rateKnob.get(), toOscFreq.get(), toOscShape.get(),
                 toFilter.get(), toAmp.get() };
    }

    juce::ComboBox shapeSelector, syncDivSelector;
    juce::TextButton syncButton, keySyncButton;
    std::unique_ptr<RotaryKnob> rateKnob, toOscFreq, toOscShape, toFilter, toAmp;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttach, syncDivAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttach, keySyncAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LFOPanel)
};
