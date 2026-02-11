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
#include "../PresetManager.h"

class HeaderPanel : public juce::Component,
                    private juce::Timer,
                    public PresetManager::Listener
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
                presetMgr->loadPreviousPreset();
        };

        nextButton.onClick = [this] {
            if (presetMgr != nullptr)
                presetMgr->loadNextPreset();
        };

        saveButton.setButtonText ("SAVE");
        saveButton.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        addAndMakeVisible (saveButton);

        menuButton.setButtonText ("...");
        menuButton.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        addAndMakeVisible (menuButton);

        saveButton.onClick = [this] { handleSave(); };
        menuButton.onClick = [this] { showMenuPopup(); };

        masterKnob = std::make_unique<RotaryKnob> ("MASTER", "dB");
        addAndMakeVisible (*masterKnob);

        voicesLabel.setText ("6 VOICE POLY", juce::dontSendNotification);
        voicesLabel.setFont (juce::Font (10.0f));
        voicesLabel.setColour (juce::Label::textColourId, SynthColours::textSecondary);
        voicesLabel.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (voicesLabel);
    }

    ~HeaderPanel() override
    {
        stopTimer();
        if (presetMgr != nullptr)
            presetMgr->removeListener (this);
    }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        masterKnob->attach (apvts, ParameterIDs::masterVol);
    }

    void setPresetManager (PresetManager* mgr)
    {
        if (presetMgr != nullptr)
            presetMgr->removeListener (this);

        presetMgr = mgr;

        if (presetMgr != nullptr)
            presetMgr->addListener (this);

        updatePresetLabel();
    }

    void presetChanged() override
    {
        updatePresetLabel();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        AnalogSynthLookAndFeel::fillBrushedMetal (g, bounds, SynthColours::faceplateMid.darker (0.05f));

        g.setColour (SynthColours::faceplateEdge.darker (0.2f));
        g.fillRect (0.0f, bounds.getBottom() - 2.0f, bounds.getWidth(), 1.0f);
        g.setColour (SynthColours::faceplateMid.brighter (0.08f));
        g.fillRect (0.0f, bounds.getBottom() - 1.0f, bounds.getWidth(), 1.0f);

        // LCD-style preset display area background
        AnalogSynthLookAndFeel::drawLCDBackground (g, lcdBounds, 2.0f);
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

        bounds.removeFromRight (10);

        auto menuBtnArea = bounds.removeFromRight (32);
        menuButton.setBounds (menuBtnArea.reduced (2));

        auto saveBtnArea = bounds.removeFromRight (48);
        saveButton.setBounds (saveBtnArea.reduced (2));

        bounds.removeFromRight (6);

        auto presetArea = bounds.withSizeKeepingCentre (260, bounds.getHeight());
        auto prevArea = presetArea.removeFromLeft (30);
        auto nextArea = presetArea.removeFromRight (30);
        prevButton.setBounds (prevArea.reduced (2));
        nextButton.setBounds (nextArea.reduced (2));
        presetLabel.setBounds (presetArea);

        // Store LCD bounds for paint
        lcdBounds = presetArea.toFloat();
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        auto labelBounds = presetLabel.getBounds();
        if (labelBounds.contains (e.getPosition()))
            showPresetBrowserPopup();
    }

private:
    void timerCallback() override {}

    void updatePresetLabel()
    {
        if (presetMgr == nullptr) return;

        juce::String text;
        if (presetMgr->getCurrentPresetIndex() >= 0)
            text = presetMgr->getCurrentPresetName();
        else
            text = "INIT";

        if (presetMgr->isDirty())
            text += " *";

        presetLabel.setText (text, juce::dontSendNotification);
    }

    // --- Preset browser popup (click on LCD area) ---

    void showPresetBrowserPopup()
    {
        if (presetMgr == nullptr) return;

        juce::PopupMenu menu;

        constexpr int kInitID = 90000;
        constexpr int kPresetOffset = 1000;

        menu.addItem (kInitID, "Init Preset");
        menu.addSeparator();

        // Factory section
        menu.addSectionHeader ("FACTORY");
        for (int i = 0; i < presetMgr->getNumPresets(); ++i)
        {
            if (presetMgr->isFactoryPreset (i))
                menu.addItem (i + kPresetOffset, presetMgr->getPresetName (i),
                              true, i == presetMgr->getCurrentPresetIndex());
        }

        menu.addSeparator();

        // User section
        menu.addSectionHeader ("USER");
        bool hasUser = false;
        for (int i = 0; i < presetMgr->getNumPresets(); ++i)
        {
            if (! presetMgr->isFactoryPreset (i))
            {
                menu.addItem (i + kPresetOffset, presetMgr->getPresetName (i),
                              true, i == presetMgr->getCurrentPresetIndex());
                hasUser = true;
            }
        }

        if (! hasUser)
            menu.addItem (-1, "(No user presets)", false);

        menu.showMenuAsync (juce::PopupMenu::Options()
                                .withTargetComponent (presetLabel)
                                .withMinimumWidth (220),
                            [this] (int result)
                            {
                                if (result == kInitID)
                                    presetMgr->initPreset();
                                else if (result >= kPresetOffset)
                                    presetMgr->loadPreset (result - kPresetOffset);
                            });
    }

    // --- Save button ---

    void handleSave()
    {
        if (presetMgr == nullptr) return;

        if (presetMgr->getCurrentPresetIndex() >= 0
            && ! presetMgr->isCurrentPresetFactory())
        {
            // Overwrite current user preset
            presetMgr->saveUserPreset (presetMgr->getCurrentPresetName());
        }
        else
        {
            showSaveAsDialog();
        }
    }

    // --- Menu button (...) ---

    void showMenuPopup()
    {
        if (presetMgr == nullptr) return;

        bool canEditCurrent = presetMgr->getCurrentPresetIndex() >= 0
                              && ! presetMgr->isCurrentPresetFactory();

        juce::PopupMenu menu;
        menu.addItem (1, "Save As...");
        menu.addItem (2, "Rename...", canEditCurrent);
        menu.addItem (3, "Delete", canEditCurrent);
        menu.addSeparator();
        menu.addItem (4, "Init");

        menu.showMenuAsync (juce::PopupMenu::Options()
                                .withTargetComponent (menuButton),
                            [this] (int result)
                            {
                                if (result == 1) showSaveAsDialog();
                                else if (result == 2) showRenameDialog();
                                else if (result == 3) showDeleteConfirmation();
                                else if (result == 4) presetMgr->initPreset();
                            });
    }

    // --- Dialogs ---

    void showSaveAsDialog()
    {
        if (presetMgr == nullptr) return;

        auto* aw = new juce::AlertWindow ("Save Preset",
                                           "Enter a name for your preset:",
                                           juce::MessageBoxIconType::NoIcon);
        aw->addTextEditor ("name", "", "Preset Name:");
        aw->addButton ("Save", 1);
        aw->addButton ("Cancel", 0);

        if (auto* te = aw->getTextEditor ("name"))
        {
            te->setWantsKeyboardFocus (true);
            te->setEscapeAndReturnKeysConsumed (true);
        }

        aw->enterModalState (true, juce::ModalCallbackFunction::create (
            [this, aw] (int result)
            {
                if (result == 1 && presetMgr != nullptr)
                {
                    auto name = aw->getTextEditorContents ("name").trim();
                    if (name.isNotEmpty())
                        presetMgr->saveUserPreset (name);
                }
                delete aw;
            }), false);

        juce::Timer::callAfterDelay (100, [aw]()
        {
            if (auto* te = aw->getTextEditor ("name"))
                te->grabKeyboardFocus();
        });
    }

    void showRenameDialog()
    {
        if (presetMgr == nullptr || presetMgr->getCurrentPresetIndex() < 0) return;

        auto currentName = presetMgr->getCurrentPresetName();

        auto* aw = new juce::AlertWindow ("Rename Preset",
                                           "Enter a new name:",
                                           juce::MessageBoxIconType::NoIcon);
        aw->addTextEditor ("name", currentName, "Preset Name:");
        aw->addButton ("Rename", 1);
        aw->addButton ("Cancel", 0);

        if (auto* te = aw->getTextEditor ("name"))
        {
            te->setWantsKeyboardFocus (true);
            te->setEscapeAndReturnKeysConsumed (true);
        }

        aw->enterModalState (true, juce::ModalCallbackFunction::create (
            [this, aw] (int result)
            {
                if (result == 1 && presetMgr != nullptr)
                {
                    auto name = aw->getTextEditorContents ("name").trim();
                    if (name.isNotEmpty())
                        presetMgr->renameCurrentPreset (name);
                }
                delete aw;
            }), false);

        juce::Timer::callAfterDelay (100, [aw]()
        {
            if (auto* te = aw->getTextEditor ("name"))
                te->grabKeyboardFocus();
        });
    }

    void showDeleteConfirmation()
    {
        if (presetMgr == nullptr || presetMgr->getCurrentPresetIndex() < 0) return;

        auto name = presetMgr->getCurrentPresetName();

        auto* aw = new juce::AlertWindow ("Delete Preset",
                                           "Delete \"" + name + "\"?",
                                           juce::MessageBoxIconType::WarningIcon);
        aw->addButton ("Delete", 1);
        aw->addButton ("Cancel", 0);

        aw->enterModalState (true, juce::ModalCallbackFunction::create (
            [this, aw] (int result)
            {
                if (result == 1 && presetMgr != nullptr)
                    presetMgr->deleteCurrentPreset();
                delete aw;
            }), false);
    }

    juce::Label titleLabel, subtitleLabel, presetLabel, voicesLabel;
    juce::TextButton prevButton, nextButton, saveButton, menuButton;
    std::unique_ptr<RotaryKnob> masterKnob;
    PresetManager* presetMgr = nullptr;
    juce::Rectangle<float> lcdBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderPanel)
};
