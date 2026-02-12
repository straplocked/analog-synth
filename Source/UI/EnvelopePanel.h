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
#include "ADSREditor.h"
#include "../ParameterIDs.h"

class EnvelopePanel : public juce::Component
{
public:
    EnvelopePanel()
        : filterEnvEditor ("FILTER ENV", SynthColours::sectionEnv),
          ampEnvEditor ("AMP ENV", SynthColours::accent),
          env3Editor ("ENV 3", SynthColours::sectionEnv),
          env4Editor ("ENV 4", SynthColours::accent)
    {
        // Pair selector buttons
        filterAmpButton.setButtonText ("Filter + Amp");
        filterAmpButton.setRadioGroupId (9002);
        filterAmpButton.setClickingTogglesState (true);
        filterAmpButton.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        filterAmpButton.setColour (juce::TextButton::buttonOnColourId, SynthColours::sectionEnv.withAlpha (0.3f));
        filterAmpButton.setColour (juce::TextButton::textColourOnId, SynthColours::silkscreen);
        filterAmpButton.setToggleState (true, juce::dontSendNotification);
        filterAmpButton.onClick = [this] { selectPair (0); };
        addAndMakeVisible (filterAmpButton);

        env34Button.setButtonText ("Env 3 + Env 4");
        env34Button.setRadioGroupId (9002);
        env34Button.setClickingTogglesState (true);
        env34Button.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        env34Button.setColour (juce::TextButton::buttonOnColourId, SynthColours::sectionEnv.withAlpha (0.3f));
        env34Button.setColour (juce::TextButton::textColourOnId, SynthColours::silkscreen);
        env34Button.onClick = [this] { selectPair (1); };
        addAndMakeVisible (env34Button);

        // Filter + Amp pair (default visible)
        addAndMakeVisible (filterEnvEditor);
        addAndMakeVisible (ampEnvEditor);

        fAttack   = std::make_unique<RotaryKnob> ("A", "s");
        fHold     = std::make_unique<RotaryKnob> ("H", "s");
        fDecay    = std::make_unique<RotaryKnob> ("D", "s");
        fSustain  = std::make_unique<RotaryKnob> ("S", "");
        fRelease  = std::make_unique<RotaryKnob> ("R", "s");
        fVelocity = std::make_unique<RotaryKnob> ("VEL", "");

        for (auto* k : getFilterKnobs())
        {
            k->setAccentColour (SynthColours::sectionEnv);
            addAndMakeVisible (*k);
        }

        aAttack   = std::make_unique<RotaryKnob> ("A", "s");
        aHold     = std::make_unique<RotaryKnob> ("H", "s");
        aDecay    = std::make_unique<RotaryKnob> ("D", "s");
        aSustain  = std::make_unique<RotaryKnob> ("S", "");
        aRelease  = std::make_unique<RotaryKnob> ("R", "s");
        aVelocity = std::make_unique<RotaryKnob> ("VEL", "");

        for (auto* k : getAmpKnobs())
        {
            k->setAccentColour (SynthColours::accent);
            addAndMakeVisible (*k);
        }

        // Env3 + Env4 pair (initially hidden)
        addChildComponent (env3Editor);
        addChildComponent (env4Editor);

        e3Attack   = std::make_unique<RotaryKnob> ("A", "s");
        e3Hold     = std::make_unique<RotaryKnob> ("H", "s");
        e3Decay    = std::make_unique<RotaryKnob> ("D", "s");
        e3Sustain  = std::make_unique<RotaryKnob> ("S", "");
        e3Release  = std::make_unique<RotaryKnob> ("R", "s");
        e3Velocity = std::make_unique<RotaryKnob> ("VEL", "");

        for (auto* k : getEnv3Knobs())
        {
            k->setAccentColour (SynthColours::sectionEnv);
            addChildComponent (*k);
        }

        e4Attack   = std::make_unique<RotaryKnob> ("A", "s");
        e4Hold     = std::make_unique<RotaryKnob> ("H", "s");
        e4Decay    = std::make_unique<RotaryKnob> ("D", "s");
        e4Sustain  = std::make_unique<RotaryKnob> ("S", "");
        e4Release  = std::make_unique<RotaryKnob> ("R", "s");
        e4Velocity = std::make_unique<RotaryKnob> ("VEL", "");

        for (auto* k : getEnv4Knobs())
        {
            k->setAccentColour (SynthColours::accent);
            addChildComponent (*k);
        }

        // Loop controls for Env3
        env3LoopButton.setButtonText ("LOOP");
        env3LoopButton.setClickingTogglesState (true);
        env3LoopButton.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        env3LoopButton.setColour (juce::TextButton::buttonOnColourId, SynthColours::sectionEnv.withAlpha (0.3f));
        addChildComponent (env3LoopButton);

        env3LoopStartSelector.addItemList ({"Attack", "Hold", "Decay"}, 1);
        env3LoopStartSelector.setSelectedId (1, juce::dontSendNotification);
        addChildComponent (env3LoopStartSelector);

        env3LoopEndSelector.addItemList ({"After Decay", "At Sustain"}, 1);
        env3LoopEndSelector.setSelectedId (1, juce::dontSendNotification);
        addChildComponent (env3LoopEndSelector);

        // Loop controls for Env4
        env4LoopButton.setButtonText ("LOOP");
        env4LoopButton.setClickingTogglesState (true);
        env4LoopButton.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        env4LoopButton.setColour (juce::TextButton::buttonOnColourId, SynthColours::accent.withAlpha (0.3f));
        addChildComponent (env4LoopButton);

        env4LoopStartSelector.addItemList ({"Attack", "Hold", "Decay"}, 1);
        env4LoopStartSelector.setSelectedId (1, juce::dontSendNotification);
        addChildComponent (env4LoopStartSelector);

        env4LoopEndSelector.addItemList ({"After Decay", "At Sustain"}, 1);
        env4LoopEndSelector.setSelectedId (1, juce::dontSendNotification);
        addChildComponent (env4LoopEndSelector);

        // Mod matrix hint for Env3/4
        modMatrixHint.setText ("Route via Mod Matrix", juce::dontSendNotification);
        modMatrixHint.setFont (juce::Font (10.0f));
        modMatrixHint.setColour (juce::Label::textColourId, SynthColours::silkscreen.withAlpha (0.5f));
        modMatrixHint.setJustificationType (juce::Justification::centred);
        addChildComponent (modMatrixHint);
    }

    void setEnvelopeLevels (std::atomic<float>* filter, std::atomic<float>* amp,
                            std::atomic<float>* e3, std::atomic<float>* e4)
    {
        filterEnvEditor.setEnvLevel (filter);
        ampEnvEditor.setEnvLevel (amp);
        env3Editor.setEnvLevel (e3);
        env4Editor.setEnvLevel (e4);
    }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        // Filter + Amp envelopes (always attached)
        filterEnvEditor.attach (apvts,
                                ParameterIDs::filterEnvAttack,
                                ParameterIDs::filterEnvDecay,
                                ParameterIDs::filterEnvSustain,
                                ParameterIDs::filterEnvRelease);

        ampEnvEditor.attach (apvts,
                             ParameterIDs::ampEnvAttack,
                             ParameterIDs::ampEnvDecay,
                             ParameterIDs::ampEnvSustain,
                             ParameterIDs::ampEnvRelease);

        fAttack->attach (apvts, ParameterIDs::filterEnvAttack);
        fHold->attach (apvts, ParameterIDs::filterEnvHold);
        fDecay->attach (apvts, ParameterIDs::filterEnvDecay);
        fSustain->attach (apvts, ParameterIDs::filterEnvSustain);
        fRelease->attach (apvts, ParameterIDs::filterEnvRelease);
        fVelocity->attach (apvts, ParameterIDs::filterEnvVelocity);

        aAttack->attach (apvts, ParameterIDs::ampEnvAttack);
        aHold->attach (apvts, ParameterIDs::ampEnvHold);
        aDecay->attach (apvts, ParameterIDs::ampEnvDecay);
        aSustain->attach (apvts, ParameterIDs::ampEnvSustain);
        aRelease->attach (apvts, ParameterIDs::ampEnvRelease);
        aVelocity->attach (apvts, ParameterIDs::ampEnvVelocity);

        // Env3 + Env4 (always attached)
        env3Editor.attach (apvts,
                           ParameterIDs::env3Attack,
                           ParameterIDs::env3Decay,
                           ParameterIDs::env3Sustain,
                           ParameterIDs::env3Release);

        env4Editor.attach (apvts,
                           ParameterIDs::env4Attack,
                           ParameterIDs::env4Decay,
                           ParameterIDs::env4Sustain,
                           ParameterIDs::env4Release);

        e3Attack->attach (apvts, ParameterIDs::env3Attack);
        e3Hold->attach (apvts, ParameterIDs::env3Hold);
        e3Decay->attach (apvts, ParameterIDs::env3Decay);
        e3Sustain->attach (apvts, ParameterIDs::env3Sustain);
        e3Release->attach (apvts, ParameterIDs::env3Release);
        e3Velocity->attach (apvts, ParameterIDs::env3Velocity);

        e4Attack->attach (apvts, ParameterIDs::env4Attack);
        e4Hold->attach (apvts, ParameterIDs::env4Hold);
        e4Decay->attach (apvts, ParameterIDs::env4Decay);
        e4Sustain->attach (apvts, ParameterIDs::env4Sustain);
        e4Release->attach (apvts, ParameterIDs::env4Release);
        e4Velocity->attach (apvts, ParameterIDs::env4Velocity);

        // Loop controls
        env3LoopAttach      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::env3LoopEnabled, env3LoopButton);
        env3LoopStartAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, ParameterIDs::env3LoopStart, env3LoopStartSelector);
        env3LoopEndAttach   = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, ParameterIDs::env3LoopEnd, env3LoopEndSelector);
        env4LoopAttach      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::env4LoopEnabled, env4LoopButton);
        env4LoopStartAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, ParameterIDs::env4LoopStart, env4LoopStartSelector);
        env4LoopEndAttach   = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, ParameterIDs::env4LoopEnd, env4LoopEndSelector);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        AnalogSynthLookAndFeel::fillBrushedMetal (g, bounds, SynthColours::faceplateDark);

        // Vertical divider between left and right envelope halves
        float halfX = bounds.getWidth() * 0.5f;
        g.setColour (SynthColours::faceplateEdge.darker (0.1f));
        g.fillRect (halfX, bounds.getY() + 28.0f, 1.0f, bounds.getHeight() - 32.0f);
        g.setColour (SynthColours::faceplateMid.brighter (0.08f));
        g.fillRect (halfX + 1.0f, bounds.getY() + 28.0f, 1.0f, bounds.getHeight() - 32.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // Pair selector row at top
        auto selectorRow = bounds.removeFromTop (26).reduced (6, 2);
        filterAmpButton.setBounds (selectorRow.removeFromLeft (100));
        selectorRow.removeFromLeft (4);
        env34Button.setBounds (selectorRow.removeFromLeft (100));

        int halfW = bounds.getWidth() / 2;
        auto leftHalf  = bounds.removeFromLeft (halfW).reduced (6);
        auto rightHalf = bounds.reduced (6);

        if (selectedPair == 0)
        {
            filterEnvEditor.setBounds (leftHalf.removeFromTop (leftHalf.getHeight() * 2 / 3));
            layoutKnobRow (leftHalf, getFilterKnobs());

            ampEnvEditor.setBounds (rightHalf.removeFromTop (rightHalf.getHeight() * 2 / 3));
            layoutKnobRow (rightHalf, getAmpKnobs());
        }
        else
        {
            // Env3 (left half): editor + knobs + loop controls
            auto e3LoopRow = leftHalf.removeFromBottom (24);
            env3LoopButton.setBounds (e3LoopRow.removeFromLeft (50).reduced (2, 0));
            env3LoopStartSelector.setBounds (e3LoopRow.removeFromLeft (80).reduced (2, 0));
            env3LoopEndSelector.setBounds (e3LoopRow.removeFromLeft (90).reduced (2, 0));

            env3Editor.setBounds (leftHalf.removeFromTop (leftHalf.getHeight() * 2 / 3));
            layoutKnobRow (leftHalf, getEnv3Knobs());

            // Env4 (right half): editor + knobs + loop controls + hint
            auto hintRow = rightHalf.removeFromBottom (20);
            modMatrixHint.setBounds (hintRow);

            auto e4LoopRow = rightHalf.removeFromBottom (24);
            env4LoopButton.setBounds (e4LoopRow.removeFromLeft (50).reduced (2, 0));
            env4LoopStartSelector.setBounds (e4LoopRow.removeFromLeft (80).reduced (2, 0));
            env4LoopEndSelector.setBounds (e4LoopRow.removeFromLeft (90).reduced (2, 0));

            env4Editor.setBounds (rightHalf.removeFromTop (rightHalf.getHeight() * 2 / 3));
            layoutKnobRow (rightHalf, getEnv4Knobs());
        }
    }

private:
    void selectPair (int pair)
    {
        selectedPair = pair;
        bool showFA = (pair == 0);

        filterEnvEditor.setVisible (showFA);
        ampEnvEditor.setVisible (showFA);
        for (auto* k : getFilterKnobs()) k->setVisible (showFA);
        for (auto* k : getAmpKnobs()) k->setVisible (showFA);

        env3Editor.setVisible (! showFA);
        env4Editor.setVisible (! showFA);
        for (auto* k : getEnv3Knobs()) k->setVisible (! showFA);
        for (auto* k : getEnv4Knobs()) k->setVisible (! showFA);
        modMatrixHint.setVisible (! showFA);

        env3LoopButton.setVisible (! showFA);
        env3LoopStartSelector.setVisible (! showFA);
        env3LoopEndSelector.setVisible (! showFA);
        env4LoopButton.setVisible (! showFA);
        env4LoopStartSelector.setVisible (! showFA);
        env4LoopEndSelector.setVisible (! showFA);

        resized();
        repaint();
    }

    void layoutKnobRow (juce::Rectangle<int> area, std::vector<RotaryKnob*> knobs)
    {
        int knobW = area.getWidth() / static_cast<int> (knobs.size());
        for (int i = 0; i < static_cast<int> (knobs.size()); ++i)
            knobs[static_cast<size_t> (i)]->setBounds (area.getX() + i * knobW, area.getY(),
                                                        knobW, area.getHeight());
    }

    std::vector<RotaryKnob*> getFilterKnobs()
    {
        return { fAttack.get(), fHold.get(), fDecay.get(), fSustain.get(), fRelease.get(), fVelocity.get() };
    }

    std::vector<RotaryKnob*> getAmpKnobs()
    {
        return { aAttack.get(), aHold.get(), aDecay.get(), aSustain.get(), aRelease.get(), aVelocity.get() };
    }

    std::vector<RotaryKnob*> getEnv3Knobs()
    {
        return { e3Attack.get(), e3Hold.get(), e3Decay.get(), e3Sustain.get(), e3Release.get(), e3Velocity.get() };
    }

    std::vector<RotaryKnob*> getEnv4Knobs()
    {
        return { e4Attack.get(), e4Hold.get(), e4Decay.get(), e4Sustain.get(), e4Release.get(), e4Velocity.get() };
    }

    int selectedPair = 0;

    // Pair selector
    juce::TextButton filterAmpButton, env34Button;

    // Filter + Amp pair
    ADSREditor filterEnvEditor, ampEnvEditor;
    std::unique_ptr<RotaryKnob> fAttack, fHold, fDecay, fSustain, fRelease, fVelocity;
    std::unique_ptr<RotaryKnob> aAttack, aHold, aDecay, aSustain, aRelease, aVelocity;

    // Env3 + Env4 pair
    ADSREditor env3Editor, env4Editor;
    std::unique_ptr<RotaryKnob> e3Attack, e3Hold, e3Decay, e3Sustain, e3Release, e3Velocity;
    std::unique_ptr<RotaryKnob> e4Attack, e4Hold, e4Decay, e4Sustain, e4Release, e4Velocity;

    // Hint label
    juce::Label modMatrixHint;

    // Loop controls (Env3/Env4)
    juce::TextButton env3LoopButton, env4LoopButton;
    juce::ComboBox env3LoopStartSelector, env3LoopEndSelector;
    juce::ComboBox env4LoopStartSelector, env4LoopEndSelector;

    // Loop attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> env3LoopAttach, env4LoopAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> env3LoopStartAttach, env3LoopEndAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> env4LoopStartAttach, env4LoopEndAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvelopePanel)
};
