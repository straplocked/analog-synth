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

class ModMatrixPanel : public juce::Component
{
public:
    ModMatrixPanel()
    {
        // ---- Macro knobs ----
        macro1 = std::make_unique<RotaryKnob> ("MACRO 1", "");
        macro2 = std::make_unique<RotaryKnob> ("MACRO 2", "");
        macro3 = std::make_unique<RotaryKnob> ("MACRO 3", "");
        macro4 = std::make_unique<RotaryKnob> ("MACRO 4", "");

        for (auto* k : getMacroKnobs())
        {
            k->setAccentColour (SynthColours::accent);
            addAndMakeVisible (*k);
        }

        // ---- Mod slots (16 rows of source/dest/amount) ----
        juce::StringArray sourceNames { "None", "Filter Env", "Amp Env", "Env 3", "Env 4",
                                        "LFO 1", "LFO 2", "LFO 3",
                                        "Velocity", "Note Num", "Mod Wheel",
                                        "Aftertouch", "Pitch Bend", "Random",
                                        "Macro 1", "Macro 2", "Macro 3", "Macro 4" };

        juce::StringArray destNames { "None", "Osc1 Freq", "Osc2 Freq", "Osc3 Freq",
                                      "Osc1 PW", "Osc2 PW", "Osc3 PW",
                                      "Osc1 Level", "Osc2 Level", "Osc3 Level",
                                      "Sub Level", "Noise Level",
                                      "LPF Cutoff", "LPF Res", "HPF Cutoff",
                                      "Filt Drive", "Filt Env Amt", "Amp Level",
                                      "LFO1 Rate", "LFO2 Rate", "LFO3 Rate",
                                      "Dist Drive", "Dist Mix",
                                      "Chorus Rate", "Chorus Depth", "Chorus Mix",
                                      "Delay Time", "Delay FB", "Delay Mix",
                                      "Reverb Size", "Reverb Mix" };

        for (int i = 0; i < 16; ++i)
        {
            sourceCombos[i] = std::make_unique<juce::ComboBox>();
            sourceCombos[i]->addItemList (sourceNames, 1);
            sourceCombos[i]->setSelectedId (1, juce::dontSendNotification);
            slotContent.addAndMakeVisible (*sourceCombos[i]);

            destCombos[i] = std::make_unique<juce::ComboBox>();
            destCombos[i]->addItemList (destNames, 1);
            destCombos[i]->setSelectedId (1, juce::dontSendNotification);
            slotContent.addAndMakeVisible (*destCombos[i]);

            amountSliders[i] = std::make_unique<juce::Slider> (juce::Slider::LinearHorizontal,
                                                                 juce::Slider::NoTextBox);
            amountSliders[i]->setRange (-1.0, 1.0, 0.01);
            amountSliders[i]->setValue (0.0);
            amountSliders[i]->setColour (juce::Slider::thumbColourId, SynthColours::accent);
            amountSliders[i]->setColour (juce::Slider::trackColourId, SynthColours::panelBg.brighter (0.15f));
            slotContent.addAndMakeVisible (*amountSliders[i]);

            slotLabels[i] = std::make_unique<juce::Label>();
            slotLabels[i]->setText (juce::String (i + 1), juce::dontSendNotification);
            slotLabels[i]->setFont (juce::Font (10.0f));
            slotLabels[i]->setColour (juce::Label::textColourId, SynthColours::silkscreen.withAlpha (0.6f));
            slotLabels[i]->setJustificationType (juce::Justification::centred);
            slotContent.addAndMakeVisible (*slotLabels[i]);
        }

        viewport.setViewedComponent (&slotContent, false);
        viewport.setScrollBarsShown (true, false);
        addAndMakeVisible (viewport);

        // ---- Osc2 FM legacy section ----
        fmLabel.setText ("FM (Osc2)", juce::dontSendNotification);
        fmLabel.setFont (juce::Font (9.0f));
        fmLabel.setColour (juce::Label::textColourId, SynthColours::sectionOsc);
        fmLabel.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (fmLabel);

        osc2ToFreq  = std::make_unique<RotaryKnob> ("FREQ", "");
        osc2ToShape = std::make_unique<RotaryKnob> ("SHAPE", "");
        osc2ToPW    = std::make_unique<RotaryKnob> ("PW", "");
        osc2ToLPF   = std::make_unique<RotaryKnob> ("LPF", "");
        osc2ToHPF   = std::make_unique<RotaryKnob> ("HPF", "");

        for (auto* k : getFMKnobs())
        {
            k->setAccentColour (SynthColours::sectionOsc);
            addAndMakeVisible (*k);
        }
    }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        macro1->attach (apvts, ParameterIDs::macro1);
        macro2->attach (apvts, ParameterIDs::macro2);
        macro3->attach (apvts, ParameterIDs::macro3);
        macro4->attach (apvts, ParameterIDs::macro4);

        for (int i = 0; i < 16; ++i)
        {
            sourceAttach[i] = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
                apvts, ParameterIDs::modSlotSourceIDs[i], *sourceCombos[i]);
            destAttach[i] = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
                apvts, ParameterIDs::modSlotDestIDs[i], *destCombos[i]);
            amountAttach[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                apvts, ParameterIDs::modSlotAmountIDs[i], *amountSliders[i]);
        }

        osc2ToFreq->attach (apvts, ParameterIDs::polyModOsc2ToOsc1Freq);
        osc2ToShape->attach (apvts, ParameterIDs::polyModOsc2ToOsc1Shape);
        osc2ToPW->attach (apvts, ParameterIDs::polyModOsc2ToOsc1PW);
        osc2ToLPF->attach (apvts, ParameterIDs::polyModOsc2ToLPF);
        osc2ToHPF->attach (apvts, ParameterIDs::polyModOsc2ToHPF);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        AnalogSynthLookAndFeel::fillBrushedMetal (g, bounds, SynthColours::faceplateDark);

        auto titleArea = bounds.reduced (10, 4).removeFromTop (16);
        AnalogSynthLookAndFeel::drawSectionTitle (g, "MOD MATRIX", titleArea, SynthColours::accent);

        g.setColour (SynthColours::silkscreen.withAlpha (0.15f));
        g.fillRect (bounds.getX() + 8.0f, titleArea.getBottom() + 1.0f, bounds.getWidth() - 16.0f, 1.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (6);
        bounds.removeFromTop (20);

        // ---- Macro knobs row (top) ----
        auto macroRow = bounds.removeFromTop (60);
        auto macros = getMacroKnobs();
        int macroW = macroRow.getWidth() / 4;
        for (int i = 0; i < 4; ++i)
            macros[static_cast<size_t> (i)]->setBounds (macroRow.getX() + i * macroW, macroRow.getY(),
                                                         macroW, macroRow.getHeight());

        // ---- FM knobs row (bottom) ----
        auto fmRow = bounds.removeFromBottom (60);
        fmLabel.setBounds (fmRow.removeFromLeft (50));
        auto fmKnobs = getFMKnobs();
        int fmW = fmRow.getWidth() / 5;
        for (int i = 0; i < 5; ++i)
            fmKnobs[static_cast<size_t> (i)]->setBounds (fmRow.getX() + i * fmW, fmRow.getY(),
                                                           fmW, fmRow.getHeight());

        // ---- Mod slot list (middle, scrollable) ----
        bounds.removeFromTop (4);
        bounds.removeFromBottom (4);

        constexpr int kRowHeight = 24;
        constexpr int kNumLabel = 18;
        int srcW = (bounds.getWidth() - kNumLabel) / 3;
        int dstW = srcW;
        int amtW = bounds.getWidth() - kNumLabel - srcW - dstW;

        slotContent.setBounds (0, 0, bounds.getWidth(), kRowHeight * 16);

        for (int i = 0; i < 16; ++i)
        {
            int y = i * kRowHeight;
            slotLabels[i]->setBounds (0, y, kNumLabel, kRowHeight);
            sourceCombos[i]->setBounds (kNumLabel, y + 2, srcW - 2, kRowHeight - 4);
            destCombos[i]->setBounds (kNumLabel + srcW, y + 2, dstW - 2, kRowHeight - 4);
            amountSliders[i]->setBounds (kNumLabel + srcW + dstW, y + 2, amtW, kRowHeight - 4);
        }

        viewport.setBounds (bounds);
    }

private:
    std::vector<RotaryKnob*> getMacroKnobs()
    {
        return { macro1.get(), macro2.get(), macro3.get(), macro4.get() };
    }

    std::vector<RotaryKnob*> getFMKnobs()
    {
        return { osc2ToFreq.get(), osc2ToShape.get(), osc2ToPW.get(),
                 osc2ToLPF.get(), osc2ToHPF.get() };
    }

    // Macros
    std::unique_ptr<RotaryKnob> macro1, macro2, macro3, macro4;

    // Mod slot UI (16 slots)
    std::unique_ptr<juce::ComboBox> sourceCombos[16];
    std::unique_ptr<juce::ComboBox> destCombos[16];
    std::unique_ptr<juce::Slider> amountSliders[16];
    std::unique_ptr<juce::Label> slotLabels[16];
    juce::Component slotContent;
    juce::Viewport viewport;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sourceAttach[16];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> destAttach[16];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttach[16];

    // Osc2 FM legacy knobs
    std::unique_ptr<RotaryKnob> osc2ToFreq, osc2ToShape, osc2ToPW, osc2ToLPF, osc2ToHPF;
    juce::Label fmLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModMatrixPanel)
};
