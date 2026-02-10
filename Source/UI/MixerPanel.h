#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "AnalogSynthLookAndFeel.h"
#include "RotaryKnob.h"
#include "../ParameterIDs.h"

class MixerPanel : public juce::Component
{
public:
    MixerPanel()
    {
        osc1Knob  = std::make_unique<RotaryKnob> ("OSC 1", "");
        osc2Knob  = std::make_unique<RotaryKnob> ("OSC 2", "");
        subKnob   = std::make_unique<RotaryKnob> ("SUB", "");
        noiseKnob = std::make_unique<RotaryKnob> ("NOISE", "");

        for (auto* k : getKnobs())
            addAndMakeVisible (*k);
    }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        osc1Knob->attach (apvts, ParameterIDs::osc1Level);
        osc2Knob->attach (apvts, ParameterIDs::osc2Level);
        subKnob->attach (apvts, ParameterIDs::subLevel);
        noiseKnob->attach (apvts, ParameterIDs::noiseLevel);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        AnalogSynthLookAndFeel::drawRecessedPanel (g, bounds);

        // Silkscreen title
        auto titleArea = bounds.reduced (10, 4).removeFromTop (16);
        AnalogSynthLookAndFeel::drawSilkscreenLabel (g, "MIXER", titleArea, juce::Justification::centredLeft, 11.0f);

        // Horizontal rule
        g.setColour (SynthColours::silkscreen.withAlpha (0.15f));
        g.fillRect (bounds.getX() + 8.0f, titleArea.getBottom() + 1.0f, bounds.getWidth() - 16.0f, 1.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (6);
        bounds.removeFromTop (20);

        auto knobs = getKnobs();
        int knobW = bounds.getWidth() / 4;

        for (int i = 0; i < 4; ++i)
            knobs[static_cast<size_t> (i)]->setBounds (bounds.getX() + i * knobW, bounds.getY(),
                                                        knobW, bounds.getHeight());
    }

private:
    std::vector<RotaryKnob*> getKnobs()
    {
        return { osc1Knob.get(), osc2Knob.get(), subKnob.get(), noiseKnob.get() };
    }

    std::unique_ptr<RotaryKnob> osc1Knob, osc2Knob, subKnob, noiseKnob;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerPanel)
};
