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
        g.setFont (juce::Font (11.0f).boldened());
        g.setColour (juce::Colours::black.withAlpha (0.3f));
        g.drawText ("FILTER", titleArea.translated (0, -0.5f), juce::Justification::centredLeft, false);
        g.setColour (SynthColours::sectionFilter);
        g.drawText ("FILTER", titleArea, juce::Justification::centredLeft, false);

        // Horizontal rule
        g.setColour (SynthColours::silkscreen.withAlpha (0.15f));
        g.fillRect (bounds.getX() + 8.0f, titleArea.getBottom() + 1.0f, bounds.getWidth() - 16.0f, 1.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (6);
        bounds.removeFromTop (20);

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

    std::unique_ptr<RotaryKnob> cutoffKnob, resonanceKnob, envAmountKnob,
                                 keyFollowKnob, driveKnob, hpfKnob;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterPanel)
};
