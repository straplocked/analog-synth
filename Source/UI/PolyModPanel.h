/*
 * AnalogSynth - Virtual analog synthesizer plugin
 * Copyright (C) 2025 AnalogSynth contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "AnalogSynthLookAndFeel.h"
#include "RotaryKnob.h"
#include "../ParameterIDs.h"

class PolyModPanel : public juce::Component
{
public:
    PolyModPanel()
    {
        // Filter Env sources
        fenvToFreq  = std::make_unique<RotaryKnob> ("FREQ", "");
        fenvToShape = std::make_unique<RotaryKnob> ("SHAPE", "");
        fenvToPW    = std::make_unique<RotaryKnob> ("PW", "");
        fenvToLPF   = std::make_unique<RotaryKnob> ("LPF", "");
        fenvToHPF   = std::make_unique<RotaryKnob> ("HPF", "");

        // Osc2 sources
        osc2ToFreq  = std::make_unique<RotaryKnob> ("FREQ", "");
        osc2ToShape = std::make_unique<RotaryKnob> ("SHAPE", "");
        osc2ToPW    = std::make_unique<RotaryKnob> ("PW", "");
        osc2ToLPF   = std::make_unique<RotaryKnob> ("LPF", "");
        osc2ToHPF   = std::make_unique<RotaryKnob> ("HPF", "");

        auto setColAndAdd = [this] (RotaryKnob& knob) {
            knob.setAccentColour (SynthColours::accent);
            addAndMakeVisible (knob);
        };

        for (auto* k : getFenvKnobs()) setColAndAdd (*k);
        for (auto* k : getOsc2Knobs()) setColAndAdd (*k);

        fenvLabel.setText ("FILT ENV", juce::dontSendNotification);
        fenvLabel.setFont (juce::Font (9.0f));
        fenvLabel.setColour (juce::Label::textColourId, SynthColours::sectionEnv);
        fenvLabel.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (fenvLabel);

        osc2Label.setText ("OSC 2", juce::dontSendNotification);
        osc2Label.setFont (juce::Font (9.0f));
        osc2Label.setColour (juce::Label::textColourId, SynthColours::sectionOsc);
        osc2Label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (osc2Label);
    }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        fenvToFreq->attach (apvts, ParameterIDs::polyModFenvToOsc1Freq);
        fenvToShape->attach (apvts, ParameterIDs::polyModFenvToOsc1Shape);
        fenvToPW->attach (apvts, ParameterIDs::polyModFenvToOsc1PW);
        fenvToLPF->attach (apvts, ParameterIDs::polyModFenvToLPF);
        fenvToHPF->attach (apvts, ParameterIDs::polyModFenvToHPF);

        osc2ToFreq->attach (apvts, ParameterIDs::polyModOsc2ToOsc1Freq);
        osc2ToShape->attach (apvts, ParameterIDs::polyModOsc2ToOsc1Shape);
        osc2ToPW->attach (apvts, ParameterIDs::polyModOsc2ToOsc1PW);
        osc2ToLPF->attach (apvts, ParameterIDs::polyModOsc2ToLPF);
        osc2ToHPF->attach (apvts, ParameterIDs::polyModOsc2ToHPF);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        AnalogSynthLookAndFeel::drawRecessedPanel (g, bounds);

        // Silkscreen title
        auto titleArea = bounds.reduced (10, 4).removeFromTop (16);
        AnalogSynthLookAndFeel::drawSectionTitle (g, "POLY MOD", titleArea, SynthColours::accent);

        // Horizontal rule
        g.setColour (SynthColours::silkscreen.withAlpha (0.15f));
        g.fillRect (bounds.getX() + 8.0f, titleArea.getBottom() + 1.0f, bounds.getWidth() - 16.0f, 1.0f);

        // Horizontal divider between FILT ENV and OSC 2 rows
        float divY = bounds.getY() + 20.0f + (bounds.getHeight() - 20.0f) * 0.5f;
        g.setColour (SynthColours::faceplateEdge.darker (0.1f));
        g.fillRect (bounds.getX() + 6.0f, divY, bounds.getWidth() - 12.0f, 1.0f);
        g.setColour (SynthColours::recessHighlight);
        g.fillRect (bounds.getX() + 6.0f, divY + 1.0f, bounds.getWidth() - 12.0f, 1.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (8);
        bounds.removeFromTop (20);

        int labelW = 50;
        int knobW = (bounds.getWidth() - labelW) / 5;
        int rowH = bounds.getHeight() / 2;

        // Row 1: Filter Env
        auto row1 = bounds.removeFromTop (rowH);
        fenvLabel.setBounds (row1.removeFromLeft (labelW));
        auto fenvKnobs = getFenvKnobs();
        for (int i = 0; i < 5; ++i)
            fenvKnobs[static_cast<size_t> (i)]->setBounds (row1.getX() + i * knobW, row1.getY(),
                                                            knobW, rowH);

        // Row 2: Osc2
        auto row2 = bounds;
        osc2Label.setBounds (row2.removeFromLeft (labelW));
        auto osc2Knobs = getOsc2Knobs();
        for (int i = 0; i < 5; ++i)
            osc2Knobs[static_cast<size_t> (i)]->setBounds (row2.getX() + i * knobW, row2.getY(),
                                                            knobW, rowH);
    }

private:
    std::vector<RotaryKnob*> getFenvKnobs()
    {
        return { fenvToFreq.get(), fenvToShape.get(), fenvToPW.get(),
                 fenvToLPF.get(), fenvToHPF.get() };
    }

    std::vector<RotaryKnob*> getOsc2Knobs()
    {
        return { osc2ToFreq.get(), osc2ToShape.get(), osc2ToPW.get(),
                 osc2ToLPF.get(), osc2ToHPF.get() };
    }

    std::unique_ptr<RotaryKnob> fenvToFreq, fenvToShape, fenvToPW, fenvToLPF, fenvToHPF;
    std::unique_ptr<RotaryKnob> osc2ToFreq, osc2ToShape, osc2ToPW, osc2ToLPF, osc2ToHPF;
    juce::Label fenvLabel, osc2Label;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PolyModPanel)
};
