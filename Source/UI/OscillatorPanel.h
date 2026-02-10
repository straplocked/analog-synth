#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "AnalogSynthLookAndFeel.h"
#include "RotaryKnob.h"

class OscillatorPanel : public juce::Component
{
public:
    OscillatorPanel (const juce::String& titleText, juce::Colour colour, bool enableSync = false)
        : title (titleText), sectionColour (colour), showSync (enableSync)
    {
        waveformSelector.addItemList ({"Saw", "Pulse", "Triangle", "Square"}, 1);
        waveformSelector.setSelectedId (1, juce::dontSendNotification);
        addAndMakeVisible (waveformSelector);

        octaveKnob   = std::make_unique<RotaryKnob> ("OCTAVE", "");
        semitoneKnob = std::make_unique<RotaryKnob> ("SEMI", "st");
        fineKnob     = std::make_unique<RotaryKnob> ("FINE", "ct");
        pwKnob       = std::make_unique<RotaryKnob> ("PW", "%");
        levelKnob    = std::make_unique<RotaryKnob> ("LEVEL", "");

        for (auto* k : getKnobs())
        {
            k->setAccentColour (sectionColour);
            addAndMakeVisible (*k);
        }

        if (showSync)
        {
            syncButton.setButtonText ("SYNC");
            syncButton.setClickingTogglesState (true);
            syncButton.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
            syncButton.setColour (juce::TextButton::buttonOnColourId, sectionColour.withAlpha (0.3f));
            addAndMakeVisible (syncButton);
        }
    }

    void attach (juce::AudioProcessorValueTreeState& apvts,
                 const juce::String& waveformID,
                 const juce::String& octaveID,
                 const juce::String& semitoneID,
                 const juce::String& fineID,
                 const juce::String& pwID,
                 const juce::String& levelID,
                 const juce::String& syncID = "")
    {
        waveformAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            apvts, waveformID, waveformSelector);
        octaveKnob->attach (apvts, octaveID);
        semitoneKnob->attach (apvts, semitoneID);
        fineKnob->attach (apvts, fineID);
        pwKnob->attach (apvts, pwID);
        levelKnob->attach (apvts, levelID);

        if (showSync && syncID.isNotEmpty())
            syncAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
                apvts, syncID, syncButton);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        AnalogSynthLookAndFeel::drawRecessedPanel (g, bounds);

        // Silkscreen title with section color tint
        auto titleArea = bounds.reduced (10, 4).removeFromTop (16);
        AnalogSynthLookAndFeel::drawSectionTitle (g, title, titleArea, sectionColour);

        // Horizontal rule below title
        g.setColour (SynthColours::silkscreen.withAlpha (0.15f));
        g.fillRect (bounds.getX() + 8.0f, titleArea.getBottom() + 1.0f, bounds.getWidth() - 16.0f, 1.0f);

        // Corner screws
        AnalogSynthLookAndFeel::drawScrewHead (g, bounds.getX() + 8.0f, bounds.getY() + 8.0f, 2.5f);
        AnalogSynthLookAndFeel::drawScrewHead (g, bounds.getRight() - 8.0f, bounds.getY() + 8.0f, 2.5f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (8);
        bounds.removeFromTop (20);

        auto comboRow = bounds.removeFromTop (24);
        comboRow.removeFromLeft (2);
        if (showSync)
        {
            syncButton.setBounds (comboRow.removeFromRight (50).reduced (2, 2));
            comboRow.removeFromRight (4);
        }
        waveformSelector.setBounds (comboRow.reduced (2, 0));

        bounds.removeFromTop (4);
        auto knobH = bounds.getHeight();
        int knobW = bounds.getWidth() / 5;

        auto knobs = getKnobs();
        for (int i = 0; i < static_cast<int> (knobs.size()); ++i)
        {
            knobs[static_cast<size_t> (i)]->setBounds (bounds.getX() + i * knobW, bounds.getY(),
                                                        knobW, knobH);
        }
    }

private:
    std::vector<RotaryKnob*> getKnobs()
    {
        return { octaveKnob.get(), semitoneKnob.get(), fineKnob.get(),
                 pwKnob.get(), levelKnob.get() };
    }

    juce::String title;
    juce::Colour sectionColour;
    bool showSync;

    juce::ComboBox waveformSelector;
    std::unique_ptr<RotaryKnob> octaveKnob, semitoneKnob, fineKnob, pwKnob, levelKnob;
    juce::TextButton syncButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveformAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscillatorPanel)
};
