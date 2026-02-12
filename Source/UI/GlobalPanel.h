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

class GlobalPanel : public juce::Component
{
public:
    GlobalPanel()
    {
        // Unison / Drift
        slopKnob   = std::make_unique<RotaryKnob> ("SLOP", "");
        detuneKnob = std::make_unique<RotaryKnob> ("DETUNE", "ct");
        glideKnob  = std::make_unique<RotaryKnob> ("GLIDE", "s");
        tuneKnob   = std::make_unique<RotaryKnob> ("TUNE", "ct");
        pbRangeKnob = std::make_unique<RotaryKnob> ("PB RANGE", "st");

        for (auto* k : getKnobs())
            addAndMakeVisible (*k);

        unisonButton.setButtonText ("UNISON");
        unisonButton.setClickingTogglesState (true);
        unisonButton.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        unisonButton.setColour (juce::TextButton::buttonOnColourId, SynthColours::accent.withAlpha (0.3f));
        addAndMakeVisible (unisonButton);

        glideButton.setButtonText ("GLIDE");
        glideButton.setClickingTogglesState (true);
        glideButton.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        glideButton.setColour (juce::TextButton::buttonOnColourId, SynthColours::accent.withAlpha (0.3f));
        addAndMakeVisible (glideButton);

        // Arpeggiator section
        arpButton.setButtonText ("ARP");
        arpButton.setClickingTogglesState (true);
        arpButton.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        arpButton.setColour (juce::TextButton::buttonOnColourId, SynthColours::sectionLfo.withAlpha (0.3f));
        addAndMakeVisible (arpButton);

        arpLatchButton.setButtonText ("LATCH");
        arpLatchButton.setClickingTogglesState (true);
        arpLatchButton.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        arpLatchButton.setColour (juce::TextButton::buttonOnColourId, SynthColours::sectionLfo.withAlpha (0.3f));
        addAndMakeVisible (arpLatchButton);

        arpModeSelector.addItemList ({"Up", "Down", "Up/Down", "Random", "As Played"}, 1);
        arpModeSelector.setSelectedId (1, juce::dontSendNotification);
        addAndMakeVisible (arpModeSelector);

        arpOctavesKnob = std::make_unique<RotaryKnob> ("OCTAVES", "");
        arpGateKnob    = std::make_unique<RotaryKnob> ("GATE", "%");
        arpBpmKnob     = std::make_unique<RotaryKnob> ("BPM", "");
        arpOctavesKnob->setAccentColour (SynthColours::sectionLfo);
        arpGateKnob->setAccentColour (SynthColours::sectionLfo);
        arpBpmKnob->setAccentColour (SynthColours::sectionLfo);
        addAndMakeVisible (*arpOctavesKnob);
        addAndMakeVisible (*arpGateKnob);
        addAndMakeVisible (*arpBpmKnob);

        arpDivSelector.addItemList ({"1/32", "1/16T", "1/16", "1/8T", "1/8", "1/4T", "1/4", "1/2", "1 Bar"}, 1);
        arpDivSelector.setSelectedId (5, juce::dontSendNotification);
        addAndMakeVisible (arpDivSelector);
    }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        slopKnob->attach (apvts, ParameterIDs::slop);
        detuneKnob->attach (apvts, ParameterIDs::unisonDetune);
        glideKnob->attach (apvts, ParameterIDs::glideTime);
        tuneKnob->attach (apvts, ParameterIDs::masterTune);
        pbRangeKnob->attach (apvts, ParameterIDs::pitchBendRange);

        unisonAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::unison, unisonButton);
        glideAttach  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::glide, glideButton);

        arpAttach      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::arpEnabled, arpButton);
        arpLatchAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::arpLatch, arpLatchButton);
        arpModeAttach  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, ParameterIDs::arpMode, arpModeSelector);
        arpDivAttach   = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, ParameterIDs::arpSyncDiv, arpDivSelector);
        arpOctavesKnob->attach (apvts, ParameterIDs::arpOctaves);
        arpGateKnob->attach (apvts, ParameterIDs::arpGate);
        arpBpmKnob->attach (apvts, ParameterIDs::arpStandaloneBpm);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Brushed metal background
        AnalogSynthLookAndFeel::fillBrushedMetal (g, bounds, SynthColours::faceplateDark);

        // Engraved vertical divider between voice and arp sections
        float halfX = bounds.getWidth() * 0.5f;
        g.setColour (SynthColours::faceplateEdge.darker (0.1f));
        g.fillRect (halfX, bounds.getY() + 4.0f, 1.0f, bounds.getHeight() - 8.0f);
        g.setColour (SynthColours::faceplateMid.brighter (0.08f));
        g.fillRect (halfX + 1.0f, bounds.getY() + 4.0f, 1.0f, bounds.getHeight() - 8.0f);

        // Silkscreen titles in top sub-row
        g.setFont (juce::Font (9.0f).boldened());
        g.setColour (SynthColours::silkscreen);
        g.drawText ("VOICE", bounds.reduced (10, 4).removeFromTop (14), juce::Justification::centredLeft, false);

        auto rightTitle = bounds.reduced (10, 4).removeFromTop (14);
        rightTitle.removeFromLeft (bounds.getWidth() * 0.5f + 8.0f);
        g.setColour (SynthColours::sectionLfo);
        g.drawText ("ARPEGGIATOR", rightTitle, juce::Justification::centredLeft, false);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (6, 2);

        int halfW = bounds.getWidth() / 2;

        // ---- Left: Voice controls ----
        auto voiceArea = bounds.removeFromLeft (halfW).reduced (2, 0);

        // Top sub-row: title + toggle buttons (22px)
        auto vTopRow = voiceArea.removeFromTop (22);
        vTopRow.removeFromLeft (42); // space for "VOICE" title
        unisonButton.setBounds (vTopRow.removeFromLeft (62).reduced (1, 0));
        vTopRow.removeFromLeft (2);
        glideButton.setBounds (vTopRow.removeFromLeft (52).reduced (1, 0));

        // Bottom sub-row: knobs (remaining ~46px)
        voiceArea.removeFromTop (2);
        auto knobs = getKnobs();
        int knobW = voiceArea.getWidth() / static_cast<int> (knobs.size());
        for (int i = 0; i < static_cast<int> (knobs.size()); ++i)
            knobs[static_cast<size_t> (i)]->setBounds (voiceArea.getX() + i * knobW, voiceArea.getY(),
                                                        knobW, voiceArea.getHeight());

        // ---- Right: Arpeggiator ----
        auto arpArea = bounds.reduced (6, 0);

        // Top sub-row: title + toggle buttons + combos (22px)
        auto aTopRow = arpArea.removeFromTop (22);
        aTopRow.removeFromLeft (80); // space for "ARPEGGIATOR" title
        arpButton.setBounds (aTopRow.removeFromLeft (42).reduced (1, 0));
        aTopRow.removeFromLeft (2);
        arpModeSelector.setBounds (aTopRow.removeFromLeft (90).reduced (1, 0));
        aTopRow.removeFromLeft (2);
        arpDivSelector.setBounds (aTopRow.removeFromLeft (70).reduced (1, 0));
        aTopRow.removeFromLeft (2);
        arpLatchButton.setBounds (aTopRow.removeFromLeft (52).reduced (1, 0));

        // Bottom sub-row: knobs (remaining ~46px)
        arpArea.removeFromTop (2);
        int arpKnobW = arpArea.getWidth() / 3;
        arpOctavesKnob->setBounds (arpArea.getX(), arpArea.getY(), arpKnobW, arpArea.getHeight());
        arpGateKnob->setBounds (arpArea.getX() + arpKnobW, arpArea.getY(), arpKnobW, arpArea.getHeight());
        arpBpmKnob->setBounds (arpArea.getX() + arpKnobW * 2, arpArea.getY(), arpKnobW, arpArea.getHeight());
    }

private:
    std::vector<RotaryKnob*> getKnobs()
    {
        return { slopKnob.get(), detuneKnob.get(), glideKnob.get(),
                 tuneKnob.get(), pbRangeKnob.get() };
    }

    std::unique_ptr<RotaryKnob> slopKnob, detuneKnob, glideKnob, tuneKnob, pbRangeKnob;
    juce::TextButton unisonButton, glideButton;

    juce::TextButton arpButton, arpLatchButton;
    juce::ComboBox arpModeSelector, arpDivSelector;
    std::unique_ptr<RotaryKnob> arpOctavesKnob, arpGateKnob, arpBpmKnob;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
        unisonAttach, glideAttach, arpAttach, arpLatchAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        arpModeAttach, arpDivAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GlobalPanel)
};
