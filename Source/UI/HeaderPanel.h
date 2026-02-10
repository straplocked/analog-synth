#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "AnalogSynthLookAndFeel.h"
#include "RotaryKnob.h"
#include "../ParameterIDs.h"
#include "../PresetManager.h"

class HeaderPanel : public juce::Component,
                    private juce::Timer
{
public:
    HeaderPanel()
    {
        titleLabel.setText ("ANALOG SYNTH", juce::dontSendNotification);
        titleLabel.setFont (juce::Font (18.0f).boldened());
        titleLabel.setColour (juce::Label::textColourId, SynthColours::silkscreen);
        titleLabel.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (titleLabel);

        subtitleLabel.setText ("Prophet-6 Inspired", juce::dontSendNotification);
        subtitleLabel.setFont (juce::Font (10.0f));
        subtitleLabel.setColour (juce::Label::textColourId, SynthColours::silkscreenDim);
        subtitleLabel.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (subtitleLabel);

        presetLabel.setText ("INIT", juce::dontSendNotification);
        presetLabel.setFont (juce::Font (13.0f));
        presetLabel.setColour (juce::Label::textColourId, SynthColours::lcdText);
        presetLabel.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (presetLabel);

        prevButton.setButtonText ("<");
        nextButton.setButtonText (">");
        prevButton.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        nextButton.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        addAndMakeVisible (prevButton);
        addAndMakeVisible (nextButton);

        prevButton.onClick = [this] {
            if (presetMgr != nullptr)
            {
                presetMgr->loadPreviousPreset();
                updatePresetLabel();
            }
        };

        nextButton.onClick = [this] {
            if (presetMgr != nullptr)
            {
                presetMgr->loadNextPreset();
                updatePresetLabel();
            }
        };

        masterKnob = std::make_unique<RotaryKnob> ("MASTER", "dB");
        addAndMakeVisible (*masterKnob);

        voicesLabel.setText ("6 VOICE POLY", juce::dontSendNotification);
        voicesLabel.setFont (juce::Font (10.0f));
        voicesLabel.setColour (juce::Label::textColourId, SynthColours::textSecondary);
        voicesLabel.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (voicesLabel);
    }

    ~HeaderPanel() override { stopTimer(); }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        masterKnob->attach (apvts, ParameterIDs::masterVol);
    }

    void setPresetManager (PresetManager* mgr)
    {
        presetMgr = mgr;
        updatePresetLabel();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Brushed metal header strip
        AnalogSynthLookAndFeel::fillBrushedMetal (g, bounds, SynthColours::faceplateMid.darker (0.05f));

        // Bottom groove (1px dark + 1px light)
        g.setColour (SynthColours::faceplateEdge.darker (0.2f));
        g.fillRect (0.0f, bounds.getBottom() - 2.0f, bounds.getWidth(), 1.0f);
        g.setColour (SynthColours::faceplateMid.brighter (0.08f));
        g.fillRect (0.0f, bounds.getBottom() - 1.0f, bounds.getWidth(), 1.0f);

        // LCD-style preset display area background
        auto presetArea = bounds.withSizeKeepingCentre (200.0f, 24.0f);
        presetArea.setY (bounds.getCentreY() - 12.0f);
        AnalogSynthLookAndFeel::drawLCDBackground (g, presetArea, 2.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (10, 4);

        auto left = bounds.removeFromLeft (160);
        titleLabel.setBounds (left.removeFromTop (22));
        subtitleLabel.setBounds (left);

        auto right = bounds.removeFromRight (80);
        voicesLabel.setBounds (right);

        auto masterArea = bounds.removeFromRight (60);
        masterKnob->setBounds (masterArea);

        bounds.removeFromRight (20);
        auto presetArea = bounds.withSizeKeepingCentre (200, bounds.getHeight());
        auto prevArea = presetArea.removeFromLeft (30);
        auto nextArea = presetArea.removeFromRight (30);
        prevButton.setBounds (prevArea.reduced (2));
        nextButton.setBounds (nextArea.reduced (2));
        presetLabel.setBounds (presetArea);
    }

private:
    void timerCallback() override {}

    void updatePresetLabel()
    {
        if (presetMgr != nullptr && presetMgr->getCurrentPresetIndex() >= 0)
            presetLabel.setText (presetMgr->getCurrentPresetName(), juce::dontSendNotification);
        else
            presetLabel.setText ("INIT", juce::dontSendNotification);
    }

    juce::Label titleLabel, subtitleLabel, presetLabel, voicesLabel;
    juce::TextButton prevButton, nextButton;
    std::unique_ptr<RotaryKnob> masterKnob;
    PresetManager* presetMgr = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderPanel)
};
