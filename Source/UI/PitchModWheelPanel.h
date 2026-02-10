#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "AnalogSynthLookAndFeel.h"

class SpringBackSlider : public juce::Slider
{
public:
    SpringBackSlider() : juce::Slider (juce::Slider::LinearVertical, juce::Slider::NoTextBox) {}

    void mouseUp (const juce::MouseEvent& e) override
    {
        juce::Slider::mouseUp (e);
        setValue (0.0, juce::sendNotificationSync);
    }
};

class PitchModWheelPanel : public juce::Component
{
public:
    PitchModWheelPanel()
    {
        pitchWheel.setRange (-1.0, 1.0, 0.0);
        pitchWheel.setValue (0.0);
        pitchWheel.setDoubleClickReturnValue (true, 0.0);
        pitchWheel.setColour (juce::Slider::thumbColourId, SynthColours::accent);
        pitchWheel.setColour (juce::Slider::trackColourId, SynthColours::knobTrack);
        pitchWheel.onValueChange = [this] {
            if (onPitchBendChanged)
                onPitchBendChanged (static_cast<float> (pitchWheel.getValue()));
        };
        addAndMakeVisible (pitchWheel);

        modWheel.setSliderStyle (juce::Slider::LinearVertical);
        modWheel.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        modWheel.setRange (0.0, 1.0, 0.0);
        modWheel.setValue (0.0);
        modWheel.setDoubleClickReturnValue (true, 0.0);
        modWheel.setColour (juce::Slider::thumbColourId, SynthColours::accent);
        modWheel.setColour (juce::Slider::trackColourId, SynthColours::knobTrack);
        modWheel.onValueChange = [this] {
            if (onModWheelChanged)
                onModWheelChanged (static_cast<float> (modWheel.getValue()));
        };
        addAndMakeVisible (modWheel);

        pitchLabel.setText ("PITCH", juce::dontSendNotification);
        pitchLabel.setFont (juce::Font (9.0f).boldened());
        pitchLabel.setColour (juce::Label::textColourId, SynthColours::silkscreenDim);
        pitchLabel.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (pitchLabel);

        modLabel.setText ("MOD", juce::dontSendNotification);
        modLabel.setFont (juce::Font (9.0f).boldened());
        modLabel.setColour (juce::Label::textColourId, SynthColours::silkscreenDim);
        modLabel.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (modLabel);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Darker metal (in shadow at bottom of synth)
        AnalogSynthLookAndFeel::fillBrushedMetal (g, bounds, SynthColours::faceplateDark.darker (0.1f));

        // Recessed wheel channels
        float sliderW = bounds.getWidth() / 2.0f;
        float channelW = sliderW - 12.0f;
        float channelH = bounds.getHeight() - 20.0f;

        // Pitch wheel channel
        auto pitchChannel = juce::Rectangle<float> (4.0f + (sliderW - channelW) * 0.5f,
                                                      14.0f, channelW, channelH);
        AnalogSynthLookAndFeel::drawRecessedPanel (g, pitchChannel, 3.0f, 1.5f);

        // Mod wheel channel
        auto modChannel = juce::Rectangle<float> (sliderW + (sliderW - channelW) * 0.5f,
                                                    14.0f, channelW, channelH);
        AnalogSynthLookAndFeel::drawRecessedPanel (g, modChannel, 3.0f, 1.5f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        int labelH = 12;
        auto labels = bounds.removeFromTop (labelH);
        int sliderW = bounds.getWidth() / 2;

        pitchLabel.setBounds (labels.removeFromLeft (sliderW));
        modLabel.setBounds (labels);

        pitchWheel.setBounds (bounds.removeFromLeft (sliderW).reduced (4, 2));
        modWheel.setBounds (bounds.reduced (4, 2));
    }

    std::function<void (float)> onPitchBendChanged;
    std::function<void (float)> onModWheelChanged;

private:
    SpringBackSlider pitchWheel;
    juce::Slider modWheel;
    juce::Label pitchLabel;
    juce::Label modLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchModWheelPanel)
};
