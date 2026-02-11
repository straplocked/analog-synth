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
#include "CustomLFOEditor.h"
#include "StepSeqEditor.h"
#include "../ParameterIDs.h"

class LFOPanel : public juce::Component
{
public:
    LFOPanel()
    {
        // LFO selector buttons
        for (int i = 0; i < 3; ++i)
        {
            lfoButtons[i].setButtonText ("LFO " + juce::String (i + 1));
            lfoButtons[i].setRadioGroupId (9001);
            lfoButtons[i].setClickingTogglesState (true);
            lfoButtons[i].setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
            lfoButtons[i].setColour (juce::TextButton::buttonOnColourId, SynthColours::sectionLfo.withAlpha (0.3f));
            lfoButtons[i].setColour (juce::TextButton::textColourOnId, SynthColours::silkscreen);
            lfoButtons[i].onClick = [this, i] { selectLFO (i); };
            addAndMakeVisible (lfoButtons[i]);
        }
        lfoButtons[0].setToggleState (true, juce::dontSendNotification);

        // Shape selector (shared — swapped on LFO switch)
        shapeSelector.addItemList ({"Triangle", "Saw", "Square", "S&H", "Sine", "Custom", "Step Seq"}, 1);
        shapeSelector.setSelectedId (1, juce::dontSendNotification);
        shapeSelector.onChange = [this] { onShapeChanged(); };
        addAndMakeVisible (shapeSelector);

        // Rate knob (shared — swapped on LFO switch)
        rateKnob = std::make_unique<RotaryKnob> ("RATE", "Hz");
        rateKnob->setAccentColour (SynthColours::sectionLfo);
        addAndMakeVisible (*rateKnob);

        // Sync controls (shared — swapped on LFO switch)
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

        // Step smooth toggle (only visible in StepSeq mode)
        stepSmoothButton.setButtonText ("SMOOTH");
        stepSmoothButton.setClickingTogglesState (true);
        stepSmoothButton.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        stepSmoothButton.setColour (juce::TextButton::buttonOnColourId, SynthColours::sectionLfo.withAlpha (0.3f));
        addChildComponent (stepSmoothButton);

        // LFO1 destination knobs (only shown for LFO1)
        toOscFreq  = std::make_unique<RotaryKnob> ("OSC FREQ", "");
        toOscShape = std::make_unique<RotaryKnob> ("OSC SHAPE", "");
        toFilter   = std::make_unique<RotaryKnob> ("FILTER", "");
        toAmp      = std::make_unique<RotaryKnob> ("AMP", "");

        for (auto* k : getLFO1DestKnobs())
        {
            k->setAccentColour (SynthColours::sectionLfo);
            addAndMakeVisible (*k);
        }

        // Hint label for LFO2/3 (route through mod matrix)
        modMatrixHint.setText ("Route via Mod Matrix", juce::dontSendNotification);
        modMatrixHint.setFont (juce::Font (11.0f));
        modMatrixHint.setColour (juce::Label::textColourId, SynthColours::silkscreen.withAlpha (0.5f));
        modMatrixHint.setJustificationType (juce::Justification::centred);
        addChildComponent (modMatrixHint);

        // Custom LFO shape editor
        customEditor.setAccentColour (SynthColours::sectionLfo);
        customEditor.onChange = [this] { writeCustomDataToTree(); };
        addChildComponent (customEditor);

        // Step sequencer editor
        stepSeqEditor.setAccentColour (SynthColours::sectionLfo);
        stepSeqEditor.onChange = [this] { writeStepSeqDataToTree(); };
        addChildComponent (stepSeqEditor);
    }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        apvtsPtr = &apvts;

        // LFO1 destination knobs (always attached — these are LFO1-specific hardwired routing)
        toOscFreq->attach (apvts, ParameterIDs::lfoToOscFreq);
        toOscShape->attach (apvts, ParameterIDs::lfoToOscShape);
        toFilter->attach (apvts, ParameterIDs::lfoToFilter);
        toAmp->attach (apvts, ParameterIDs::lfoToAmp);

        // Attach LFO1 controls initially
        selectLFO (0);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        AnalogSynthLookAndFeel::fillBrushedMetal (g, bounds, SynthColours::faceplateDark);

        auto titleArea = bounds.reduced (10, 4).removeFromTop (16);
        AnalogSynthLookAndFeel::drawSectionTitle (g, "LFO", titleArea, SynthColours::sectionLfo);

        g.setColour (SynthColours::silkscreen.withAlpha (0.15f));
        g.fillRect (bounds.getX() + 8.0f, titleArea.getBottom() + 1.0f, bounds.getWidth() - 16.0f, 1.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (8);
        bounds.removeFromTop (20);

        // LFO selector row
        auto selectorRow = bounds.removeFromTop (24);
        int btnW = 60;
        for (int i = 0; i < 3; ++i)
            lfoButtons[i].setBounds (selectorRow.getX() + i * (btnW + 4), selectorRow.getY(), btnW, 22);

        bounds.removeFromTop (4);

        // Control row: shape selector, sync controls, step smooth
        auto topRow = bounds.removeFromTop (26);
        shapeSelector.setBounds (topRow.removeFromLeft (120).reduced (2, 0));
        topRow.removeFromLeft (8);
        syncButton.setBounds (topRow.removeFromLeft (60).reduced (2, 0));
        syncDivSelector.setBounds (topRow.removeFromLeft (80).reduced (2, 0));
        topRow.removeFromLeft (8);
        keySyncButton.setBounds (topRow.removeFromLeft (80).reduced (2, 0));
        topRow.removeFromLeft (8);
        stepSmoothButton.setBounds (topRow.removeFromLeft (80).reduced (2, 0));

        bounds.removeFromTop (6);

        // Bottom area: depends on shape and selected LFO
        int shapeIdx = shapeSelector.getSelectedId() - 1; // 0-based
        bool isCustom = (shapeIdx == 5);
        bool isStepSeq = (shapeIdx == 6);

        if (isCustom || isStepSeq)
        {
            // Shape editor mode: rate knob on left, editor on right
            auto rateArea = bounds.removeFromLeft (std::min (bounds.getWidth() / 5, 80));
            rateKnob->setBounds (rateArea);

            if (isCustom)
                customEditor.setBounds (bounds.reduced (2));
            else
                stepSeqEditor.setBounds (bounds.reduced (2));
        }
        else if (selectedLFO == 0)
        {
            // LFO1 normal: rate + 4 destination knobs
            auto knobs = getAllLFO1Knobs();
            int knobW = bounds.getWidth() / static_cast<int> (knobs.size());
            for (int i = 0; i < static_cast<int> (knobs.size()); ++i)
                knobs[static_cast<size_t> (i)]->setBounds (bounds.getX() + i * knobW, bounds.getY(),
                                                            knobW, bounds.getHeight());
        }
        else
        {
            // LFO2/3 normal: only rate knob + hint
            rateKnob->setBounds (bounds.removeFromLeft (bounds.getWidth() / 5));
            modMatrixHint.setBounds (bounds);
        }
    }

private:
    void selectLFO (int index)
    {
        selectedLFO = index;

        // Reset attachments
        shapeAttach.reset();
        syncDivAttach.reset();
        syncAttach.reset();
        keySyncAttach.reset();
        stepSmoothAttach.reset();
        rateKnob->detach();

        if (apvtsPtr == nullptr) return;

        // Attach controls to the selected LFO's parameters
        if (index == 0)
        {
            shapeAttach   = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (*apvtsPtr, ParameterIDs::lfoShape, shapeSelector);
            syncDivAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (*apvtsPtr, ParameterIDs::lfoSyncDiv, syncDivSelector);
            syncAttach    = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (*apvtsPtr, ParameterIDs::lfoSync, syncButton);
            keySyncAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (*apvtsPtr, ParameterIDs::lfoKeySync, keySyncButton);
            stepSmoothAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (*apvtsPtr, ParameterIDs::lfo1StepSmooth, stepSmoothButton);
            rateKnob->attach (*apvtsPtr, ParameterIDs::lfoRate);
        }
        else if (index == 1)
        {
            shapeAttach   = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (*apvtsPtr, ParameterIDs::lfo2Shape, shapeSelector);
            syncDivAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (*apvtsPtr, ParameterIDs::lfo2SyncDiv, syncDivSelector);
            syncAttach    = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (*apvtsPtr, ParameterIDs::lfo2Sync, syncButton);
            keySyncAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (*apvtsPtr, ParameterIDs::lfo2KeySync, keySyncButton);
            stepSmoothAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (*apvtsPtr, ParameterIDs::lfo2StepSmooth, stepSmoothButton);
            rateKnob->attach (*apvtsPtr, ParameterIDs::lfo2Rate);
        }
        else
        {
            shapeAttach   = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (*apvtsPtr, ParameterIDs::lfo3Shape, shapeSelector);
            syncDivAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (*apvtsPtr, ParameterIDs::lfo3SyncDiv, syncDivSelector);
            syncAttach    = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (*apvtsPtr, ParameterIDs::lfo3Sync, syncButton);
            keySyncAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (*apvtsPtr, ParameterIDs::lfo3KeySync, keySyncButton);
            stepSmoothAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (*apvtsPtr, ParameterIDs::lfo3StepSmooth, stepSmoothButton);
            rateKnob->attach (*apvtsPtr, ParameterIDs::lfo3Rate);
        }

        // Load editor data from ValueTree for the selected LFO
        loadEditorDataFromTree();

        // Update visibility
        onShapeChanged();
    }

    void onShapeChanged()
    {
        int shapeIdx = shapeSelector.getSelectedId() - 1; // 0-based
        bool isCustom = (shapeIdx == 5);
        bool isStepSeq = (shapeIdx == 6);
        bool isEditor = isCustom || isStepSeq;

        // Show/hide editors
        customEditor.setVisible (isCustom);
        stepSeqEditor.setVisible (isStepSeq);
        stepSmoothButton.setVisible (isStepSeq);

        // Show/hide normal controls
        if (isEditor)
        {
            // In editor mode: hide dest knobs and hint
            for (auto* k : getLFO1DestKnobs())
                k->setVisible (false);
            modMatrixHint.setVisible (false);
        }
        else
        {
            bool showDests = (selectedLFO == 0);
            for (auto* k : getLFO1DestKnobs())
                k->setVisible (showDests);
            modMatrixHint.setVisible (! showDests);
        }

        resized();
        repaint();
    }

    void loadEditorDataFromTree()
    {
        if (apvtsPtr == nullptr) return;

        auto shapeData = apvtsPtr->state.getChildWithName ("LFO_SHAPE_DATA");
        if (! shapeData.isValid()) return;

        juce::String prefix = "LFO" + juce::String (selectedLFO);

        // Load custom shape data
        auto customTree = shapeData.getChildWithName (juce::Identifier (prefix + "_CUSTOM"));
        if (customTree.isValid())
        {
            int numPts = static_cast<int> (customTree.getProperty ("numPoints", 2));
            customEditor.setNumPoints (numPts);
            for (int i = 0; i < numPts; ++i)
            {
                float x = static_cast<float> (customTree.getProperty (juce::Identifier ("x" + juce::String (i)), 0.0f));
                float y = static_cast<float> (customTree.getProperty (juce::Identifier ("y" + juce::String (i)), 0.0f));
                customEditor.setPoint (i, x, y);
            }
        }

        // Load step seq data
        auto stepTree = shapeData.getChildWithName (juce::Identifier (prefix + "_STEPSEQ"));
        if (stepTree.isValid())
        {
            int numSteps = static_cast<int> (stepTree.getProperty ("numSteps", 16));
            stepSeqEditor.setNumSteps (numSteps);
            for (int i = 0; i < numSteps; ++i)
            {
                float val = static_cast<float> (stepTree.getProperty (juce::Identifier ("s" + juce::String (i)), 0.0f));
                stepSeqEditor.setStepValue (i, val);
            }
        }

        customEditor.repaint();
        stepSeqEditor.repaint();
    }

    void writeCustomDataToTree()
    {
        if (apvtsPtr == nullptr) return;

        auto shapeData = apvtsPtr->state.getChildWithName ("LFO_SHAPE_DATA");
        if (! shapeData.isValid()) return;

        juce::String prefix = "LFO" + juce::String (selectedLFO);
        auto customTree = shapeData.getChildWithName (juce::Identifier (prefix + "_CUSTOM"));
        if (! customTree.isValid()) return;

        int numPts = customEditor.getNumPoints();
        customTree.setProperty ("numPoints", numPts, nullptr);
        for (int i = 0; i < numPts; ++i)
        {
            customTree.setProperty (juce::Identifier ("x" + juce::String (i)), customEditor.getPointX (i), nullptr);
            customTree.setProperty (juce::Identifier ("y" + juce::String (i)), customEditor.getPointY (i), nullptr);
        }
    }

    void writeStepSeqDataToTree()
    {
        if (apvtsPtr == nullptr) return;

        auto shapeData = apvtsPtr->state.getChildWithName ("LFO_SHAPE_DATA");
        if (! shapeData.isValid()) return;

        juce::String prefix = "LFO" + juce::String (selectedLFO);
        auto stepTree = shapeData.getChildWithName (juce::Identifier (prefix + "_STEPSEQ"));
        if (! stepTree.isValid()) return;

        int numSteps = stepSeqEditor.getNumSteps();
        stepTree.setProperty ("numSteps", numSteps, nullptr);
        for (int i = 0; i < numSteps; ++i)
            stepTree.setProperty (juce::Identifier ("s" + juce::String (i)), stepSeqEditor.getStepValue (i), nullptr);
    }

    std::vector<RotaryKnob*> getLFO1DestKnobs()
    {
        return { toOscFreq.get(), toOscShape.get(), toFilter.get(), toAmp.get() };
    }

    std::vector<RotaryKnob*> getAllLFO1Knobs()
    {
        return { rateKnob.get(), toOscFreq.get(), toOscShape.get(), toFilter.get(), toAmp.get() };
    }

    juce::AudioProcessorValueTreeState* apvtsPtr = nullptr;
    int selectedLFO = 0;

    juce::TextButton lfoButtons[3];

    juce::ComboBox shapeSelector, syncDivSelector;
    juce::TextButton syncButton, keySyncButton, stepSmoothButton;
    std::unique_ptr<RotaryKnob> rateKnob;

    // LFO1 destination knobs (hardwired routing — only visible for LFO1)
    std::unique_ptr<RotaryKnob> toOscFreq, toOscShape, toFilter, toAmp;

    // Hint label for LFO2/3
    juce::Label modMatrixHint;

    // Shape editors
    CustomLFOEditor customEditor;
    StepSeqEditor stepSeqEditor;

    // Attachments (swapped when switching LFOs)
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttach, syncDivAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttach, keySyncAttach, stepSmoothAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LFOPanel)
};
