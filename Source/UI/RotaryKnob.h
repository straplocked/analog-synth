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

class RotaryKnob : public juce::Component
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

    ~RotaryKnob() override = default;

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
        slider.setColour (juce::Slider::thumbColourId, colour);
        slider.setColour (juce::Slider::rotarySliderFillColourId, colour);
    }

    void attach (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID)
    {
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            apvts, paramID, slider);
        updateValueText();
    }

    void detach()
    {
        attachment.reset();
    }

    juce::Slider& getSlider() { return slider; }

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

    juce::Slider slider;
    juce::Label label;
    juce::Label valueLabel;
    juce::String suffix;
    bool showValue;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RotaryKnob)
};
