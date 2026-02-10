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

        // Engraved vertical divider
        float halfX = bounds.getWidth() * 0.5f;
        g.setColour (SynthColours::faceplateEdge.darker (0.1f));
        g.fillRect (halfX, bounds.getY() + 20.0f, 1.0f, bounds.getHeight() - 24.0f);
        g.setColour (SynthColours::faceplateMid.brighter (0.08f));
        g.fillRect (halfX + 1.0f, bounds.getY() + 20.0f, 1.0f, bounds.getHeight() - 24.0f);

        // Silkscreen titles
        auto titleArea = bounds.reduced (10, 4).removeFromTop (16);
        AnalogSynthLookAndFeel::drawSectionTitle (g, "VOICE", titleArea, SynthColours::silkscreen);

        auto rightTitle = bounds.reduced (10, 4).removeFromTop (16);
        rightTitle.removeFromLeft (bounds.getWidth() * 0.5f + 8.0f);
        AnalogSynthLookAndFeel::drawSectionTitle (g, "ARPEGGIATOR", rightTitle, SynthColours::sectionLfo);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (8);
        bounds.removeFromTop (20);

        int halfW = bounds.getWidth() / 2;

        // Left: Voice controls
        auto voiceArea = bounds.removeFromLeft (halfW);
        auto btnRow = voiceArea.removeFromTop (26);
        unisonButton.setBounds (btnRow.removeFromLeft (80).reduced (2, 0));
        btnRow.removeFromLeft (8);
        glideButton.setBounds (btnRow.removeFromLeft (80).reduced (2, 0));

        voiceArea.removeFromTop (4);
        auto knobs = getKnobs();
        int knobW = voiceArea.getWidth() / static_cast<int> (knobs.size());
        for (int i = 0; i < static_cast<int> (knobs.size()); ++i)
            knobs[static_cast<size_t> (i)]->setBounds (voiceArea.getX() + i * knobW, voiceArea.getY(),
                                                        knobW, voiceArea.getHeight());

        // Right: Arpeggiator
        auto arpArea = bounds.reduced (8, 0);
        auto arpBtnRow = arpArea.removeFromTop (26);
        arpButton.setBounds (arpBtnRow.removeFromLeft (60).reduced (2, 0));
        arpBtnRow.removeFromLeft (4);
        arpModeSelector.setBounds (arpBtnRow.removeFromLeft (110).reduced (2, 0));
        arpBtnRow.removeFromLeft (4);
        arpDivSelector.setBounds (arpBtnRow.removeFromLeft (80).reduced (2, 0));
        arpBtnRow.removeFromLeft (4);
        arpLatchButton.setBounds (arpBtnRow.removeFromLeft (70).reduced (2, 0));

        arpArea.removeFromTop (4);
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
