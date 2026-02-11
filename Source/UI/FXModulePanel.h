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
#include "../FX/FXOrderUtils.h"
#include <functional>

// A single eurorack-style FX module panel.
// Each module draws its own faceplate with screws, title, jack decorations,
// and contains its own knobs, buttons, and APVTS attachments.
class FXModulePanel : public juce::Component,
                      public juce::DragAndDropTarget
{
public:
    FXModulePanel (int effectIndex, const juce::String& title)
        : fxIndex (effectIndex), moduleName (title)
    {
        enableButton.setClickingTogglesState (true);
        enableButton.setButtonText ("ON");
        enableButton.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        enableButton.setColour (juce::TextButton::buttonOnColourId, SynthColours::sectionFx.withAlpha (0.3f));
        addAndMakeVisible (enableButton);
    }

    ~FXModulePanel() override = default;

    int getEffectIndex() const { return fxIndex; }

    // Callback fired when a module is dropped onto this one.
    // Args: sourceFxIndex, targetFxIndex, insertBefore
    std::function<void (int, int, bool)> onModuleDropped;

    // --- Drag and Drop ---
    bool isInterestedInDragSource (const SourceDetails& details) override
    {
        return details.description.toString().startsWith ("fx_module_");
    }

    void itemDragEnter (const SourceDetails&) override
    {
        dragHovering = true;
        repaint();
    }

    void itemDragExit (const SourceDetails&) override
    {
        dragHovering = false;
        repaint();
    }

    void itemDropped (const SourceDetails& details) override
    {
        auto desc = details.description.toString();
        if (desc.startsWith ("fx_module_") && onModuleDropped)
        {
            int sourceFxIndex = desc.substring (10).getIntValue();
            onModuleDropped (sourceFxIndex, fxIndex, dragIndicatorLeft);
        }

        dragHovering = false;
        repaint();
    }

    void itemDragMove (const SourceDetails& details) override
    {
        // Determine if we should show indicator on left or right
        bool wasLeft = dragIndicatorLeft;
        dragIndicatorLeft = details.localPosition.x < getWidth() / 2;
        if (wasLeft != dragIndicatorLeft)
            repaint();
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.y < dragHandleHeight)
            dragger.startDraggingComponent (this, e);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (e.y < dragHandleHeight || isDragActive)
        {
            isDragActive = true;
            if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor (this))
            {
                auto snapshot = createComponentSnapshot (getLocalBounds());
                snapshot.multiplyAllAlphas (0.6f);
                container->startDragging ("fx_module_" + juce::String (fxIndex), this,
                                          juce::ScaledImage (snapshot), true);
                isDragActive = false;
            }
        }
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        isDragActive = false;
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Module faceplate - slightly different tint per module
        auto baseColour = SynthColours::faceplateDark.brighter (fxIndex * 0.008f);
        AnalogSynthLookAndFeel::fillBrushedMetal (g, bounds, baseColour);

        // Top and bottom rail strips (3px)
        g.setColour (SynthColours::faceplateEdge.darker (0.2f));
        g.fillRect (bounds.getX(), bounds.getY(), bounds.getWidth(), 3.0f);
        g.fillRect (bounds.getX(), bounds.getBottom() - 3.0f, bounds.getWidth(), 3.0f);

        // Corner screws (2 top, 2 bottom)
        float screwInset = 8.0f;
        float screwR = 3.5f;
        AnalogSynthLookAndFeel::drawScrewHead (g, bounds.getX() + screwInset, bounds.getY() + screwInset, screwR,
                                                static_cast<float> (fxIndex) * 0.7f);
        AnalogSynthLookAndFeel::drawScrewHead (g, bounds.getRight() - screwInset, bounds.getY() + screwInset, screwR,
                                                static_cast<float> (fxIndex) * 0.7f + 1.2f);
        AnalogSynthLookAndFeel::drawScrewHead (g, bounds.getX() + screwInset, bounds.getBottom() - screwInset, screwR,
                                                static_cast<float> (fxIndex) * 0.7f + 0.5f);
        AnalogSynthLookAndFeel::drawScrewHead (g, bounds.getRight() - screwInset, bounds.getBottom() - screwInset, screwR,
                                                static_cast<float> (fxIndex) * 0.7f + 2.0f);

        // Drag handle grip (3 horizontal lines) at very top
        float gripY = 5.0f;
        float gripW = 16.0f;
        float gripX = bounds.getCentreX() - gripW * 0.5f;
        g.setColour (SynthColours::silkscreenDim.withAlpha (0.5f));
        for (int line = 0; line < 3; ++line)
        {
            float ly = gripY + static_cast<float> (line) * 3.0f;
            g.fillRect (gripX, ly, gripW, 1.0f);
        }

        // Module title (silk-screened) — centered, below grip, above jacks
        auto titleArea = bounds.reduced (4.0f, 0.0f).withTop (16.0f).withHeight (14.0f);
        g.setFont (juce::Font (12.0f).boldened());

        // Subtle glow
        g.setColour (SynthColours::sectionFx.withAlpha (0.06f));
        g.drawText (moduleName, titleArea.expanded (2, 1), juce::Justification::centred, false);

        // Dark inset
        g.setColour (juce::Colours::black.withAlpha (0.3f));
        g.drawText (moduleName, titleArea.translated (0, -0.5f), juce::Justification::centred, false);

        // Main title
        g.setColour (SynthColours::sectionFx);
        g.drawText (moduleName, titleArea, juce::Justification::centred, false);

        // Enable LED (right of title)
        float ledX = bounds.getRight() - 16.0f;
        float ledY = titleArea.getCentreY();
        AnalogSynthLookAndFeel::drawLED (g, ledX, ledY, 3.0f, enableButton.getToggleState(), SynthColours::sectionFx);

        // Decorative jack sockets (IN/OUT) below title
        float jackY = titleArea.getBottom() + 2.0f;
        float jackR = 4.0f;
        drawJackSocket (g, bounds.getX() + 16.0f, jackY + jackR, jackR, true);
        drawJackSocket (g, bounds.getRight() - 16.0f, jackY + jackR, jackR, false);

        // Silkscreen labels for jacks
        g.setFont (juce::Font (7.0f));
        g.setColour (SynthColours::silkscreenDim.withAlpha (0.4f));
        g.drawText ("IN", static_cast<int> (bounds.getX() + 6.0f), static_cast<int> (jackY + jackR * 2 + 1),
                    20, 8, juce::Justification::centred);
        g.drawText ("OUT", static_cast<int> (bounds.getRight() - 26.0f), static_cast<int> (jackY + jackR * 2 + 1),
                    20, 8, juce::Justification::centred);

        // Recessed knob area
        auto knobArea = bounds.reduced (6.0f, 0.0f)
                            .withTop (jackY + jackR * 2 + 10.0f)
                            .withBottom (bounds.getBottom() - 12.0f);
        AnalogSynthLookAndFeel::drawRecessedPanel (g, knobArea, 3.0f, 1.5f);

        // Drag-and-drop hover indicator
        if (dragHovering)
        {
            float indicatorX = dragIndicatorLeft ? bounds.getX() : bounds.getRight() - 2.0f;
            g.setColour (SynthColours::sectionFx.withAlpha (0.8f));
            g.fillRect (indicatorX, bounds.getY() + 4.0f, 2.0f, bounds.getHeight() - 8.0f);

            // Glow
            g.setColour (SynthColours::sectionFx.withAlpha (0.2f));
            g.fillRect (indicatorX - 2.0f, bounds.getY() + 4.0f, 6.0f, bounds.getHeight() - 8.0f);
        }
    }

    bool isDragIndicatorOnLeft() const { return dragIndicatorLeft; }
    bool isDragHovering() const { return dragHovering; }

protected:
    juce::TextButton enableButton;
    int fxIndex;
    juce::String moduleName;

    static constexpr int dragHandleHeight = 16;
    static constexpr int railHeight = 3;
    static constexpr int headerHeight = 52; // grip + title + jacks + labels

    // Returns the bounds area for placing knobs (below header/jacks, above bottom rail)
    juce::Rectangle<int> getKnobArea() const
    {
        auto bounds = getLocalBounds().reduced (8, 0);
        bounds.removeFromTop (headerHeight);
        bounds.removeFromBottom (14);  // bottom rail + margin
        return bounds;
    }

private:
    bool dragHovering = false;
    bool dragIndicatorLeft = true;
    bool isDragActive = false;
    juce::ComponentDragger dragger;

    static void drawJackSocket (juce::Graphics& g, float cx, float cy, float radius, bool /*isInput*/)
    {
        // Outer ring
        g.setColour (SynthColours::screwBody.darker (0.2f));
        g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

        // Inner recess
        g.setColour (SynthColours::faceplateEdge);
        g.fillEllipse (cx - radius * 0.65f, cy - radius * 0.65f, radius * 1.3f, radius * 1.3f);

        // Center hole
        g.setColour (juce::Colour (0xff111111));
        g.fillEllipse (cx - radius * 0.3f, cy - radius * 0.3f, radius * 0.6f, radius * 0.6f);

        // Highlight on ring
        g.setColour (juce::Colour (0x15ffffff));
        g.fillEllipse (cx - radius * 0.8f, cy - radius * 0.9f, radius * 0.8f, radius * 0.5f);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FXModulePanel)
};

// ============================================================
// Concrete module panels for each effect type
// ============================================================

class DistortionModulePanel : public FXModulePanel
{
public:
    DistortionModulePanel() : FXModulePanel (0, "FURY")
    {
        distType.addItemList ({ "Tanh", "Tube", "Hard Clip", "Fuzz", "Bitcrush", "Tape", "Wavefolder" }, 1);
        distType.setSelectedId (1, juce::dontSendNotification);
        addAndMakeVisible (distType);

        drive = std::make_unique<RotaryKnob> ("DRIVE", "");
        tone  = std::make_unique<RotaryKnob> ("TONE", "");
        mixKnob = std::make_unique<RotaryKnob> ("MIX", "%");

        for (auto* k : { drive.get(), tone.get(), mixKnob.get() })
        {
            k->setAccentColour (SynthColours::sectionFx);
            addAndMakeVisible (*k);
        }
    }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        enableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::distEnabled, enableButton);
        typeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, ParameterIDs::distType, distType);
        drive->attach (apvts, ParameterIDs::distDrive);
        tone->attach (apvts, ParameterIDs::distTone);
        mixKnob->attach (apvts, ParameterIDs::distMix);
    }

    void resized() override
    {
        auto area = getKnobArea();
        auto typeRow = area.removeFromTop (22);
        distType.setBounds (typeRow.reduced (4, 0));

        enableButton.setBounds (juce::Rectangle<int> (4, 15, 30, 16));

        area.removeFromTop (4);
        int knobW = area.getWidth() / 3;
        drive->setBounds (area.removeFromLeft (knobW));
        tone->setBounds (area.removeFromLeft (knobW));
        mixKnob->setBounds (area);
    }

private:
    juce::ComboBox distType;
    std::unique_ptr<RotaryKnob> drive, tone, mixKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DistortionModulePanel)
};

class CompressorModulePanel : public FXModulePanel
{
public:
    CompressorModulePanel() : FXModulePanel (1, "CRUSH")
    {
        threshold = std::make_unique<RotaryKnob> ("THRESH", "dB");
        ratio     = std::make_unique<RotaryKnob> ("RATIO", "");
        mixKnob   = std::make_unique<RotaryKnob> ("MIX", "%");
        attack    = std::make_unique<RotaryKnob> ("ATK", "ms");
        release   = std::make_unique<RotaryKnob> ("REL", "ms");
        makeup    = std::make_unique<RotaryKnob> ("GAIN", "dB");

        for (auto* k : { threshold.get(), ratio.get(), mixKnob.get(), attack.get(), release.get(), makeup.get() })
        {
            k->setAccentColour (SynthColours::sectionFx);
            addAndMakeVisible (*k);
        }
    }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        enableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::compEnabled, enableButton);
        threshold->attach (apvts, ParameterIDs::compThreshold);
        ratio->attach (apvts, ParameterIDs::compRatio);
        attack->attach (apvts, ParameterIDs::compAttack);
        release->attach (apvts, ParameterIDs::compRelease);
        makeup->attach (apvts, ParameterIDs::compMakeup);
        mixKnob->attach (apvts, ParameterIDs::compMix);
    }

    void resized() override
    {
        auto area = getKnobArea();
        enableButton.setBounds (juce::Rectangle<int> (4, 15, 30, 16));

        // Two rows of 3 knobs
        int rowH = area.getHeight() / 2;
        auto row1 = area.removeFromTop (rowH);
        auto row2 = area;

        int knobW = row1.getWidth() / 3;
        threshold->setBounds (row1.removeFromLeft (knobW));
        ratio->setBounds (row1.removeFromLeft (knobW));
        mixKnob->setBounds (row1);

        attack->setBounds (row2.removeFromLeft (knobW));
        release->setBounds (row2.removeFromLeft (knobW));
        makeup->setBounds (row2);
    }

private:
    std::unique_ptr<RotaryKnob> threshold, ratio, mixKnob, attack, release, makeup;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorModulePanel)
};

class EQModulePanel : public FXModulePanel
{
public:
    EQModulePanel() : FXModulePanel (2, "SCULPT")
    {
        lowFreq  = std::make_unique<RotaryKnob> ("LO Hz", "Hz");
        lowGain  = std::make_unique<RotaryKnob> ("LO", "dB");
        midFreq  = std::make_unique<RotaryKnob> ("MID Hz", "Hz");
        midGain  = std::make_unique<RotaryKnob> ("MID", "dB");
        midQ     = std::make_unique<RotaryKnob> ("Q", "");
        highFreq = std::make_unique<RotaryKnob> ("HI Hz", "Hz");
        highGain = std::make_unique<RotaryKnob> ("HI", "dB");

        for (auto* k : { lowFreq.get(), lowGain.get(), midFreq.get(), midGain.get(),
                          midQ.get(), highFreq.get(), highGain.get() })
        {
            k->setAccentColour (SynthColours::sectionFx);
            addAndMakeVisible (*k);
        }
    }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        enableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::eqEnabled, enableButton);
        lowFreq->attach (apvts, ParameterIDs::eqLowFreq);
        lowGain->attach (apvts, ParameterIDs::eqLowGain);
        midFreq->attach (apvts, ParameterIDs::eqMidFreq);
        midGain->attach (apvts, ParameterIDs::eqMidGain);
        midQ->attach (apvts, ParameterIDs::eqMidQ);
        highFreq->attach (apvts, ParameterIDs::eqHighFreq);
        highGain->attach (apvts, ParameterIDs::eqHighGain);
    }

    void resized() override
    {
        auto area = getKnobArea();
        enableButton.setBounds (juce::Rectangle<int> (4, 15, 30, 16));

        // Row 1: LO freq, LO gain, MID freq (top half)
        // Row 2: MID gain, Q, HI freq, HI gain (bottom half)
        int rowH = area.getHeight() / 2;
        auto row1 = area.removeFromTop (rowH);
        auto row2 = area;

        int w3 = row1.getWidth() / 3;
        lowFreq->setBounds (row1.removeFromLeft (w3));
        lowGain->setBounds (row1.removeFromLeft (w3));
        midFreq->setBounds (row1);

        int w4 = row2.getWidth() / 4;
        midGain->setBounds (row2.removeFromLeft (w4));
        midQ->setBounds (row2.removeFromLeft (w4));
        highFreq->setBounds (row2.removeFromLeft (w4));
        highGain->setBounds (row2);
    }

private:
    std::unique_ptr<RotaryKnob> lowFreq, lowGain, midFreq, midGain, midQ, highFreq, highGain;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQModulePanel)
};

class ChorusModulePanel : public FXModulePanel
{
public:
    ChorusModulePanel() : FXModulePanel (3, "DRIFT")
    {
        rate    = std::make_unique<RotaryKnob> ("RATE", "Hz");
        depth   = std::make_unique<RotaryKnob> ("DEPTH", "");
        mixKnob = std::make_unique<RotaryKnob> ("MIX", "%");

        flangerBtn.setButtonText ("FLG");
        flangerBtn.setClickingTogglesState (true);
        flangerBtn.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        flangerBtn.setColour (juce::TextButton::buttonOnColourId, SynthColours::sectionFx.withAlpha (0.3f));

        for (auto* k : { rate.get(), depth.get(), mixKnob.get() })
        {
            k->setAccentColour (SynthColours::sectionFx);
            addAndMakeVisible (*k);
        }
        addAndMakeVisible (flangerBtn);
    }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        enableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::chorusEnabled, enableButton);
        flangerAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::chorusFlanger, flangerBtn);
        rate->attach (apvts, ParameterIDs::chorusRate);
        depth->attach (apvts, ParameterIDs::chorusDepth);
        mixKnob->attach (apvts, ParameterIDs::chorusMix);
    }

    void resized() override
    {
        auto area = getKnobArea();

        enableButton.setBounds (juce::Rectangle<int> (4, 15, 30, 16));
        flangerBtn.setBounds (juce::Rectangle<int> (getWidth() - 38, 35, 34, 14));

        int knobW = area.getWidth() / 3;
        rate->setBounds (area.removeFromLeft (knobW));
        depth->setBounds (area.removeFromLeft (knobW));
        mixKnob->setBounds (area);
    }

private:
    std::unique_ptr<RotaryKnob> rate, depth, mixKnob;
    juce::TextButton flangerBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttach, flangerAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChorusModulePanel)
};

class DelayModulePanel : public FXModulePanel
{
public:
    DelayModulePanel() : FXModulePanel (4, "VOID")
    {
        time     = std::make_unique<RotaryKnob> ("TIME", "ms");
        feedback = std::make_unique<RotaryKnob> ("FDBK", "");
        mixKnob  = std::make_unique<RotaryKnob> ("MIX", "%");
        widthKnob = std::make_unique<RotaryKnob> ("WIDTH", "");

        auto setupBtn = [&] (juce::TextButton& btn, const juce::String& text)
        {
            btn.setButtonText (text);
            btn.setClickingTogglesState (true);
            btn.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
            btn.setColour (juce::TextButton::buttonOnColourId, SynthColours::sectionFx.withAlpha (0.3f));
            addAndMakeVisible (btn);
        };

        setupBtn (pingPongBtn, "P-P");
        setupBtn (bbdBtn, "BBD");
        setupBtn (syncBtn, "SYNC");

        syncDivBox.addItemList ({ "1/32", "1/16T", "1/16", "1/8T", "1/8", "1/4T", "1/4", "1/2", "1 Bar" }, 1);
        syncDivBox.setSelectedId (7, juce::dontSendNotification);
        addAndMakeVisible (syncDivBox);

        for (auto* k : { time.get(), feedback.get(), mixKnob.get(), widthKnob.get() })
        {
            k->setAccentColour (SynthColours::sectionFx);
            addAndMakeVisible (*k);
        }
    }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        enableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::delayEnabled, enableButton);
        ppAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::delayPingPong, pingPongBtn);
        bbdAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::delayBBD, bbdBtn);
        syncAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::delaySync, syncBtn);
        syncDivAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, ParameterIDs::delaySyncDiv, syncDivBox);
        time->attach (apvts, ParameterIDs::delayTime);
        feedback->attach (apvts, ParameterIDs::delayFeedback);
        mixKnob->attach (apvts, ParameterIDs::delayMix);
        widthKnob->attach (apvts, ParameterIDs::delayWidth);

        apvtsRef = &apvts;
    }

    void resized() override
    {
        auto area = getKnobArea();

        enableButton.setBounds (juce::Rectangle<int> (4, 15, 30, 16));

        // Buttons row below header
        auto btnRow = area.removeFromTop (20);
        int btnW = btnRow.getWidth() / 4;
        syncBtn.setBounds (btnRow.removeFromLeft (btnW).reduced (1, 0));
        pingPongBtn.setBounds (btnRow.removeFromLeft (btnW).reduced (1, 0));
        bbdBtn.setBounds (btnRow.removeFromLeft (btnW).reduced (1, 0));

        // Sync div combobox
        auto divRow = area.removeFromTop (22);
        syncDivBox.setBounds (divRow.reduced (4, 0));

        area.removeFromTop (2);

        // Row 1: TIME, FDBK, MIX
        int rowH = area.getHeight() / 2;
        auto row1 = area.removeFromTop (rowH);
        int knobW = row1.getWidth() / 3;
        time->setBounds (row1.removeFromLeft (knobW));
        feedback->setBounds (row1.removeFromLeft (knobW));
        mixKnob->setBounds (row1);

        // Row 2: WIDTH (centered)
        widthKnob->setBounds (area.withSizeKeepingCentre (area.getWidth() / 3, area.getHeight()));
    }

    // Called from parent to grey out TIME knob when sync is active
    void updateSyncState()
    {
        if (apvtsRef != nullptr)
        {
            bool syncOn = apvtsRef->getRawParameterValue (ParameterIDs::delaySync)->load() > 0.5f;
            time->setEnabled (! syncOn);
            time->setAlpha (syncOn ? 0.3f : 1.0f);
        }
    }

private:
    std::unique_ptr<RotaryKnob> time, feedback, mixKnob, widthKnob;
    juce::TextButton pingPongBtn, bbdBtn, syncBtn;
    juce::ComboBox syncDivBox;
    juce::AudioProcessorValueTreeState* apvtsRef = nullptr;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttach, ppAttach, bbdAttach, syncAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> syncDivAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayModulePanel)
};

class ReverbModulePanel : public FXModulePanel
{
public:
    ReverbModulePanel() : FXModulePanel (5, "ABYSS")
    {
        reverbTypeBox.addItemList ({ "Room", "Hall", "Plate", "Spring" }, 1);
        reverbTypeBox.setSelectedId (1, juce::dontSendNotification);
        addAndMakeVisible (reverbTypeBox);

        sizeKnob    = std::make_unique<RotaryKnob> ("SIZE", "");
        dampingKnob = std::make_unique<RotaryKnob> ("DAMP", "");
        mixKnob     = std::make_unique<RotaryKnob> ("MIX", "%");

        for (auto* k : { sizeKnob.get(), dampingKnob.get(), mixKnob.get() })
        {
            k->setAccentColour (SynthColours::sectionFx);
            addAndMakeVisible (*k);
        }
    }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        enableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::reverbEnabled, enableButton);
        typeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, ParameterIDs::reverbType, reverbTypeBox);
        sizeKnob->attach (apvts, ParameterIDs::reverbSize);
        dampingKnob->attach (apvts, ParameterIDs::reverbDamping);
        mixKnob->attach (apvts, ParameterIDs::reverbMix);
    }

    void resized() override
    {
        auto area = getKnobArea();
        enableButton.setBounds (juce::Rectangle<int> (4, 15, 30, 16));

        auto typeRow = area.removeFromTop (22);
        reverbTypeBox.setBounds (typeRow.reduced (4, 0));

        area.removeFromTop (4);
        int knobW = area.getWidth() / 3;
        sizeKnob->setBounds (area.removeFromLeft (knobW));
        dampingKnob->setBounds (area.removeFromLeft (knobW));
        mixKnob->setBounds (area);
    }

private:
    juce::ComboBox reverbTypeBox;
    std::unique_ptr<RotaryKnob> sizeKnob, dampingKnob, mixKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbModulePanel)
};

class PhaserModulePanel : public FXModulePanel
{
public:
    PhaserModulePanel() : FXModulePanel (6, "PHASE")
    {
        stagesBox.addItemList ({ "4", "6", "8" }, 1);
        stagesBox.setSelectedId (2, juce::dontSendNotification);
        addAndMakeVisible (stagesBox);

        rate     = std::make_unique<RotaryKnob> ("RATE", "Hz");
        depth    = std::make_unique<RotaryKnob> ("DEPTH", "");
        feedback = std::make_unique<RotaryKnob> ("FDBK", "");
        mixKnob  = std::make_unique<RotaryKnob> ("MIX", "%");

        for (auto* k : { rate.get(), depth.get(), feedback.get(), mixKnob.get() })
        {
            k->setAccentColour (SynthColours::sectionFx);
            addAndMakeVisible (*k);
        }
    }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        enableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::phaserEnabled, enableButton);
        stagesAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, ParameterIDs::phaserStages, stagesBox);
        rate->attach (apvts, ParameterIDs::phaserRate);
        depth->attach (apvts, ParameterIDs::phaserDepth);
        feedback->attach (apvts, ParameterIDs::phaserFeedback);
        mixKnob->attach (apvts, ParameterIDs::phaserMix);
    }

    void resized() override
    {
        auto area = getKnobArea();
        enableButton.setBounds (juce::Rectangle<int> (4, 15, 30, 16));

        auto stagesRow = area.removeFromTop (22);
        stagesBox.setBounds (stagesRow.reduced (4, 0));
        area.removeFromTop (2);

        int knobW = area.getWidth() / 2;
        int rowH = area.getHeight() / 2;
        auto row1 = area.removeFromTop (rowH);
        rate->setBounds (row1.removeFromLeft (knobW));
        depth->setBounds (row1);
        auto row2 = area;
        feedback->setBounds (row2.removeFromLeft (knobW));
        mixKnob->setBounds (row2);
    }

private:
    juce::ComboBox stagesBox;
    std::unique_ptr<RotaryKnob> rate, depth, feedback, mixKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> stagesAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhaserModulePanel)
};

class FlangerModulePanel : public FXModulePanel
{
public:
    FlangerModulePanel() : FXModulePanel (7, "FLANGE")
    {
        rate     = std::make_unique<RotaryKnob> ("RATE", "Hz");
        depth    = std::make_unique<RotaryKnob> ("DEPTH", "");
        feedback = std::make_unique<RotaryKnob> ("FDBK", "");
        mixKnob  = std::make_unique<RotaryKnob> ("MIX", "%");

        for (auto* k : { rate.get(), depth.get(), feedback.get(), mixKnob.get() })
        {
            k->setAccentColour (SynthColours::sectionFx);
            addAndMakeVisible (*k);
        }
    }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        enableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::flangerEnabled, enableButton);
        rate->attach (apvts, ParameterIDs::flangerRate);
        depth->attach (apvts, ParameterIDs::flangerDepth);
        feedback->attach (apvts, ParameterIDs::flangerFeedback);
        mixKnob->attach (apvts, ParameterIDs::flangerMix);
    }

    void resized() override
    {
        auto area = getKnobArea();
        enableButton.setBounds (juce::Rectangle<int> (4, 15, 30, 16));

        int knobW = area.getWidth() / 2;
        int rowH = area.getHeight() / 2;
        auto row1 = area.removeFromTop (rowH);
        rate->setBounds (row1.removeFromLeft (knobW));
        depth->setBounds (row1);
        auto row2 = area;
        feedback->setBounds (row2.removeFromLeft (knobW));
        mixKnob->setBounds (row2);
    }

private:
    std::unique_ptr<RotaryKnob> rate, depth, feedback, mixKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FlangerModulePanel)
};

class HyperModulePanel : public FXModulePanel
{
public:
    HyperModulePanel() : FXModulePanel (8, "HYPER")
    {
        voicesBox.addItemList ({ "2", "4", "8" }, 1);
        voicesBox.setSelectedId (1, juce::dontSendNotification);
        addAndMakeVisible (voicesBox);

        detune  = std::make_unique<RotaryKnob> ("DETUNE", "ct");
        spread  = std::make_unique<RotaryKnob> ("SPREAD", "");
        mixKnob = std::make_unique<RotaryKnob> ("MIX", "%");

        for (auto* k : { detune.get(), spread.get(), mixKnob.get() })
        {
            k->setAccentColour (SynthColours::sectionFx);
            addAndMakeVisible (*k);
        }
    }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        enableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::hyperEnabled, enableButton);
        voicesAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, ParameterIDs::hyperVoices, voicesBox);
        detune->attach (apvts, ParameterIDs::hyperDetune);
        spread->attach (apvts, ParameterIDs::hyperSpread);
        mixKnob->attach (apvts, ParameterIDs::hyperMix);
    }

    void resized() override
    {
        auto area = getKnobArea();
        enableButton.setBounds (juce::Rectangle<int> (4, 15, 30, 16));

        auto voicesRow = area.removeFromTop (22);
        voicesBox.setBounds (voicesRow.reduced (4, 0));
        area.removeFromTop (4);

        int knobW = area.getWidth() / 3;
        detune->setBounds (area.removeFromLeft (knobW));
        spread->setBounds (area.removeFromLeft (knobW));
        mixKnob->setBounds (area);
    }

private:
    juce::ComboBox voicesBox;
    std::unique_ptr<RotaryKnob> detune, spread, mixKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> voicesAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HyperModulePanel)
};
