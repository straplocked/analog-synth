#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "AnalogSynthLookAndFeel.h"
#include "RotaryKnob.h"
#include "../ParameterIDs.h"

class FilterPanel : public juce::Component
{
public:
    FilterPanel()
    {
        typeSelector.addItemList ({"Ladder", "Diode", "Sequential", "Acid", "MS20"}, 1);
        typeSelector.setSelectedId (1, juce::dontSendNotification);
        addAndMakeVisible (typeSelector);

        slopeSelector.addItemList ({"12dB", "24dB"}, 1);
        slopeSelector.setSelectedId (2, juce::dontSendNotification);
        addAndMakeVisible (slopeSelector);

        cutoffKnob    = std::make_unique<RotaryKnob> ("CUTOFF", "Hz");
        resonanceKnob = std::make_unique<RotaryKnob> ("RES", "");
        envAmountKnob = std::make_unique<RotaryKnob> ("ENV", "");
        keyFollowKnob = std::make_unique<RotaryKnob> ("KEY", "%");
        driveKnob     = std::make_unique<RotaryKnob> ("DRIVE", "");
        hpfKnob       = std::make_unique<RotaryKnob> ("HPF", "Hz");

        for (auto* k : getKnobs())
        {
            k->setAccentColour (SynthColours::sectionFilter);
            addAndMakeVisible (*k);
        }
    }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        typeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            apvts, ParameterIDs::lpfType, typeSelector);
        slopeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            apvts, ParameterIDs::lpfSlope, slopeSelector);

        cutoffKnob->attach (apvts, ParameterIDs::lpfCutoff);
        resonanceKnob->attach (apvts, ParameterIDs::lpfResonance);
        envAmountKnob->attach (apvts, ParameterIDs::lpfEnvAmount);
        keyFollowKnob->attach (apvts, ParameterIDs::lpfKeyFollow);
        driveKnob->attach (apvts, ParameterIDs::lpfDrive);
        hpfKnob->attach (apvts, ParameterIDs::hpfCutoff);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        AnalogSynthLookAndFeel::drawRecessedPanel (g, bounds);

        // Silkscreen title with section color
        auto titleArea = bounds.reduced (10, 4).removeFromTop (16);
        AnalogSynthLookAndFeel::drawSectionTitle (g, "FILTER", titleArea, SynthColours::sectionFilter);

        // Horizontal rule
        g.setColour (SynthColours::silkscreen.withAlpha (0.15f));
        g.fillRect (bounds.getX() + 8.0f, titleArea.getBottom() + 1.0f, bounds.getWidth() - 16.0f, 1.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (8);
        bounds.removeFromTop (20);

        // Combo row: type selector | slope selector
        auto comboRow = bounds.removeFromTop (24);
        comboRow.removeFromLeft (2);
        slopeSelector.setBounds (comboRow.removeFromRight (70).reduced (2, 0));
        comboRow.removeFromRight (4);
        typeSelector.setBounds (comboRow.reduced (2, 0));

        bounds.removeFromTop (4);

        auto knobs = getKnobs();
        int numKnobs = static_cast<int> (knobs.size());
        int knobW = bounds.getWidth() / numKnobs;

        for (int i = 0; i < numKnobs; ++i)
            knobs[static_cast<size_t> (i)]->setBounds (bounds.getX() + i * knobW, bounds.getY(),
                                                        knobW, bounds.getHeight());
    }

private:
    std::vector<RotaryKnob*> getKnobs()
    {
        return { cutoffKnob.get(), resonanceKnob.get(), envAmountKnob.get(),
                 keyFollowKnob.get(), driveKnob.get(), hpfKnob.get() };
    }

    juce::ComboBox typeSelector;
    juce::ComboBox slopeSelector;
    std::unique_ptr<RotaryKnob> cutoffKnob, resonanceKnob, envAmountKnob,
                                 keyFollowKnob, driveKnob, hpfKnob;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttach, slopeAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterPanel)
};
