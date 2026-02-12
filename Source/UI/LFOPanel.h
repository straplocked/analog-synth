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

class LFOPanel : public juce::Component
{
public:
    LFOPanel()
    {
        juce::StringArray shapeNames { "Triangle", "Saw", "Square", "S&H", "Sine", "Custom", "Step Seq" };
        juce::StringArray syncDivNames { "1/32", "1/16T", "1/16", "1/8T", "1/8", "1/4T", "1/4", "1/2", "1 Bar", "2 Bars", "4 Bars" };

        for (int i = 0; i < 3; ++i)
        {
            auto& col = columns[i];

            col.headerLabel.setText ("LFO " + juce::String (i + 1), juce::dontSendNotification);
            col.headerLabel.setFont (juce::Font (10.0f).boldened());
            col.headerLabel.setColour (juce::Label::textColourId, SynthColours::sectionLfo);
            col.headerLabel.setJustificationType (juce::Justification::centred);
            addAndMakeVisible (col.headerLabel);

            col.shapeSelector.addItemList (shapeNames, 1);
            col.shapeSelector.setSelectedId (1, juce::dontSendNotification);
            addAndMakeVisible (col.shapeSelector);

            col.rateKnob = std::make_unique<RotaryKnob> ("RATE", "Hz");
            col.rateKnob->setAccentColour (SynthColours::sectionLfo);
            addAndMakeVisible (*col.rateKnob);

            col.syncButton.setButtonText ("SYNC");
            col.syncButton.setClickingTogglesState (true);
            col.syncButton.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
            col.syncButton.setColour (juce::TextButton::buttonOnColourId, SynthColours::sectionLfo.withAlpha (0.3f));
            addAndMakeVisible (col.syncButton);

            col.syncDivSelector.addItemList (syncDivNames, 1);
            col.syncDivSelector.setSelectedId (7, juce::dontSendNotification);
            addAndMakeVisible (col.syncDivSelector);

            col.keySyncButton.setButtonText ("KEY SYNC");
            col.keySyncButton.setClickingTogglesState (true);
            col.keySyncButton.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
            col.keySyncButton.setColour (juce::TextButton::buttonOnColourId, SynthColours::sectionLfo.withAlpha (0.3f));
            addAndMakeVisible (col.keySyncButton);
        }
    }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        // LFO 1
        columns[0].shapeAttach   = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, ParameterIDs::lfoShape, columns[0].shapeSelector);
        columns[0].rateKnob->attach (apvts, ParameterIDs::lfoRate);
        columns[0].syncAttach    = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::lfoSync, columns[0].syncButton);
        columns[0].syncDivAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, ParameterIDs::lfoSyncDiv, columns[0].syncDivSelector);
        columns[0].keySyncAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::lfoKeySync, columns[0].keySyncButton);

        // LFO 2
        columns[1].shapeAttach   = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, ParameterIDs::lfo2Shape, columns[1].shapeSelector);
        columns[1].rateKnob->attach (apvts, ParameterIDs::lfo2Rate);
        columns[1].syncAttach    = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::lfo2Sync, columns[1].syncButton);
        columns[1].syncDivAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, ParameterIDs::lfo2SyncDiv, columns[1].syncDivSelector);
        columns[1].keySyncAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::lfo2KeySync, columns[1].keySyncButton);

        // LFO 3
        columns[2].shapeAttach   = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, ParameterIDs::lfo3Shape, columns[2].shapeSelector);
        columns[2].rateKnob->attach (apvts, ParameterIDs::lfo3Rate);
        columns[2].syncAttach    = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::lfo3Sync, columns[2].syncButton);
        columns[2].syncDivAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, ParameterIDs::lfo3SyncDiv, columns[2].syncDivSelector);
        columns[2].keySyncAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::lfo3KeySync, columns[2].keySyncButton);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        AnalogSynthLookAndFeel::fillBrushedMetal (g, bounds, SynthColours::faceplateDark);

        auto titleArea = bounds.reduced (10, 4).removeFromTop (16);
        AnalogSynthLookAndFeel::drawSectionTitle (g, "LFO", titleArea, SynthColours::sectionLfo);

        g.setColour (SynthColours::silkscreen.withAlpha (0.15f));
        g.fillRect (bounds.getX() + 8.0f, titleArea.getBottom() + 1.0f, bounds.getWidth() - 16.0f, 1.0f);

        // Vertical dividers between columns
        float colW = (bounds.getWidth() - 16.0f) / 3.0f;
        for (int i = 1; i < 3; ++i)
        {
            float divX = 8.0f + colW * static_cast<float> (i);
            g.setColour (SynthColours::faceplateEdge.darker (0.1f));
            g.fillRect (divX, bounds.getY() + 24.0f, 1.0f, bounds.getHeight() - 28.0f);
            g.setColour (SynthColours::faceplateMid.brighter (0.08f));
            g.fillRect (divX + 1.0f, bounds.getY() + 24.0f, 1.0f, bounds.getHeight() - 28.0f);
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (8);
        bounds.removeFromTop (20);

        int colW = bounds.getWidth() / 3;

        for (int i = 0; i < 3; ++i)
        {
            auto& col = columns[i];
            auto colBounds = juce::Rectangle<int> (bounds.getX() + i * colW, bounds.getY(),
                                                    colW, bounds.getHeight()).reduced (4, 0);

            // Header label (16px)
            col.headerLabel.setBounds (colBounds.removeFromTop (16));
            colBounds.removeFromTop (2);

            // Shape combo (22px)
            col.shapeSelector.setBounds (colBounds.removeFromTop (22).reduced (2, 0));
            colBounds.removeFromTop (2);

            // Rate knob (takes remaining middle space)
            auto bottomControls = 24 + 4 + 22; // sync row + gap + keysync
            auto rateArea = colBounds.withTrimmedBottom (bottomControls);
            col.rateKnob->setBounds (rateArea);

            // Bottom controls
            auto bottomArea = colBounds.withTop (colBounds.getBottom() - bottomControls);

            // Sync row (24px): SYNC button + Div combo
            auto syncRow = bottomArea.removeFromTop (24);
            int syncBtnW = syncRow.getWidth() * 35 / 100;
            col.syncButton.setBounds (syncRow.removeFromLeft (syncBtnW).reduced (1, 0));
            col.syncDivSelector.setBounds (syncRow.reduced (1, 0));

            bottomArea.removeFromTop (4);

            // KeySync button (22px)
            col.keySyncButton.setBounds (bottomArea.removeFromTop (22).reduced (2, 0));
        }
    }

private:
    struct LFOColumn
    {
        juce::Label headerLabel;
        juce::ComboBox shapeSelector;
        std::unique_ptr<RotaryKnob> rateKnob;
        juce::TextButton syncButton;
        juce::ComboBox syncDivSelector;
        juce::TextButton keySyncButton;

        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttach, syncDivAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttach, keySyncAttach;
    };

    LFOColumn columns[3];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LFOPanel)
};
