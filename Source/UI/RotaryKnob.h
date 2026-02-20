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
#include "../Utility/MidiLearnManager.h"

class RotaryKnob : public juce::Component,
                   public MidiLearnManager::Listener
{
public:
    RotaryKnob (const juce::String& labelText,
                const juce::String& unitSuffix = "",
                bool displayValue = true)
        : suffix (unitSuffix), showValue (displayValue)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        slider.setColour (juce::Slider::thumbColourId, SynthColours::accent);
        slider.setColour (juce::Slider::rotarySliderFillColourId, SynthColours::accent);
        slider.setColour (juce::Slider::rotarySliderOutlineColourId, SynthColours::knobTrack);
        addAndMakeVisible (slider);

        label.setText (labelText, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::Font (10.0f).boldened());
        label.setColour (juce::Label::textColourId, SynthColours::silkscreen);
        addAndMakeVisible (label);

        valueLabel.setJustificationType (juce::Justification::centred);
        valueLabel.setFont (juce::Font (9.0f));
        valueLabel.setColour (juce::Label::textColourId, SynthColours::accent.withAlpha (0.85f));
        valueLabel.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (valueLabel);

        slider.onValueChange = [this] { updateValueText(); };
    }

    ~RotaryKnob() override
    {
        if (midiLearnManager != nullptr)
            midiLearnManager->removeListener (this);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        label.setBounds (bounds.removeFromTop (14));

        if (showValue)
        {
            valueLabel.setBounds (bounds.removeFromBottom (14));
            valueLabel.setVisible (true);
        }
        else
        {
            valueLabel.setVisible (false);
        }

        slider.setBounds (bounds);
    }

    void setAccentColour (juce::Colour colour)
    {
        defaultAccentColour = colour;
        slider.setColour (juce::Slider::thumbColourId, colour);
        slider.setColour (juce::Slider::rotarySliderFillColourId, colour);
    }

    void attach (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID)
    {
        parameterID = paramID;
        defaultAccentColour = slider.findColour (juce::Slider::thumbColourId);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            apvts, paramID, slider);
        updateValueText();
    }

    void detach()
    {
        attachment.reset();
    }

    void setMidiLearnManager (MidiLearnManager* m)
    {
        if (midiLearnManager != nullptr)
            midiLearnManager->removeListener (this);

        midiLearnManager = m;

        if (midiLearnManager != nullptr)
        {
            midiLearnManager->addListener (this);
            updateMidiLearnVisual();
        }
    }

    juce::Slider& getSlider() { return slider; }

    // --- MidiLearnManager::Listener ---
    void midiLearnStateChanged() override
    {
        updateMidiLearnVisual();
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu() && midiLearnManager != nullptr && parameterID.isNotEmpty())
        {
            showMidiLearnMenu();
            return;
        }
        Component::mouseDown (e);
    }

private:
    void updateValueText()
    {
        if (! showValue) return;

        auto val = slider.getValue();
        juce::String text;

        if (std::abs (val) >= 1000.0)
            text = juce::String (val / 1000.0, 1) + "k";
        else if (std::abs (val) >= 100.0)
            text = juce::String (static_cast<int> (val));
        else if (std::abs (val) >= 10.0)
            text = juce::String (val, 1);
        else if (std::abs (val) >= 1.0)
            text = juce::String (val, 2);
        else
            text = juce::String (val, 3);

        if (suffix.isNotEmpty())
            text += " " + suffix;

        valueLabel.setText (text, juce::dontSendNotification);
    }

    void updateMidiLearnVisual()
    {
        if (midiLearnManager == nullptr || parameterID.isEmpty())
            return;

        if (midiLearnManager->isLearning() &&
            midiLearnManager->getLearningParamID() == parameterID)
        {
            slider.setColour (juce::Slider::thumbColourId, SynthColours::ledRed);
            slider.setColour (juce::Slider::rotarySliderFillColourId, SynthColours::ledRed);
        }
        else if (midiLearnManager->hasMapping (parameterID))
        {
            slider.setColour (juce::Slider::thumbColourId, SynthColours::ledAmber);
            slider.setColour (juce::Slider::rotarySliderFillColourId, SynthColours::ledAmber);
        }
        else
        {
            slider.setColour (juce::Slider::thumbColourId, defaultAccentColour);
            slider.setColour (juce::Slider::rotarySliderFillColourId, defaultAccentColour);
        }
        repaint();
    }

    void showMidiLearnMenu()
    {
        juce::PopupMenu menu;
        bool hasMapping = midiLearnManager->hasMapping (parameterID);
        int currentCC = midiLearnManager->getCCForParam (parameterID);

        if (hasMapping)
        {
            menu.addItem (1, "MIDI Learn (CC " + juce::String (currentCC) + " assigned)");
            menu.addItem (2, "Remove MIDI CC " + juce::String (currentCC));
        }
        else
        {
            menu.addItem (1, "MIDI Learn");
        }
        menu.addItem (3, "Clear All MIDI Mappings");

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
            [this] (int result)
            {
                if (result == 1)
                    midiLearnManager->startLearning (parameterID);
                else if (result == 2)
                {
                    midiLearnManager->removeMapping (parameterID);
                    midiLearnManager->saveToState();
                }
                else if (result == 3)
                    midiLearnManager->clearAllMappings();
            });
    }

    juce::Slider slider;
    juce::Label label;
    juce::Label valueLabel;
    juce::String suffix;
    bool showValue;

    juce::String parameterID;
    juce::Colour defaultAccentColour { SynthColours::accent };
    MidiLearnManager* midiLearnManager = nullptr;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RotaryKnob)
};
