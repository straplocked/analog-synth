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

class MixerPanel : public juce::Component
{
public:
    MixerPanel()
    {
        osc1Knob  = std::make_unique<RotaryKnob> ("OSC 1", "");
        osc2Knob  = std::make_unique<RotaryKnob> ("OSC 2", "");
        osc3Knob  = std::make_unique<RotaryKnob> ("OSC 3", "");
        subKnob   = std::make_unique<RotaryKnob> ("SUB", "");
        noiseKnob = std::make_unique<RotaryKnob> ("NOISE", "");

        for (auto* k : getKnobs())
            addAndMakeVisible (*k);

        // Sub waveform selector
        subWaveformBox.addItemList ({ "Sine", "Triangle", "Square", "Pulse" }, 1);
        subWaveformBox.setColour (juce::ComboBox::backgroundColourId, SynthColours::panelBg);
        subWaveformBox.setColour (juce::ComboBox::textColourId, SynthColours::silkscreen);
        addAndMakeVisible (subWaveformBox);

        // Sub octave selector
        subOctaveBox.addItemList ({ "-1 Oct", "-2 Oct" }, 1);
        subOctaveBox.setColour (juce::ComboBox::backgroundColourId, SynthColours::panelBg);
        subOctaveBox.setColour (juce::ComboBox::textColourId, SynthColours::silkscreen);
        addAndMakeVisible (subOctaveBox);
    }

    void setMidiLearnManager (MidiLearnManager* m)
    {
        for (auto* k : getKnobs())
            k->setMidiLearnManager (m);
    }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        osc1Knob->attach (apvts, ParameterIDs::osc1Level);
        osc2Knob->attach (apvts, ParameterIDs::osc2Level);
        osc3Knob->attach (apvts, ParameterIDs::osc3Level);
        subKnob->attach (apvts, ParameterIDs::subLevel);
        noiseKnob->attach (apvts, ParameterIDs::noiseLevel);

        subWaveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            apvts, ParameterIDs::subWaveform, subWaveformBox);
        subOctaveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            apvts, ParameterIDs::subOctave, subOctaveBox);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        AnalogSynthLookAndFeel::drawRecessedPanel (g, bounds);

        // Silkscreen title
        auto titleArea = bounds.reduced (10, 4).removeFromTop (16);
        AnalogSynthLookAndFeel::drawSectionTitle (g, "MIXER", titleArea, SynthColours::silkscreen);

        // Horizontal rule
        g.setColour (SynthColours::silkscreen.withAlpha (0.15f));
        g.fillRect (bounds.getX() + 8.0f, titleArea.getBottom() + 1.0f, bounds.getWidth() - 16.0f, 1.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (8);
        bounds.removeFromTop (20);

        // Bottom row: sub waveform and octave selectors
        auto bottomRow = bounds.removeFromBottom (22);
        int halfW = bottomRow.getWidth() / 2;
        subWaveformBox.setBounds (bottomRow.removeFromLeft (halfW).reduced (2, 1));
        subOctaveBox.setBounds (bottomRow.reduced (2, 1));

        bounds.removeFromBottom (2); // spacing

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
        return { osc1Knob.get(), osc2Knob.get(), osc3Knob.get(), subKnob.get(), noiseKnob.get() };
    }

    std::unique_ptr<RotaryKnob> osc1Knob, osc2Knob, osc3Knob, subKnob, noiseKnob;

    juce::ComboBox subWaveformBox;
    juce::ComboBox subOctaveBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> subWaveformAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> subOctaveAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerPanel)
};
