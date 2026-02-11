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
#include "FXModulePanel.h"
#include "../ParameterIDs.h"
#include "../FX/FXChain.h"

class FXPanel : public juce::Component,
                public juce::DragAndDropContainer,
                public juce::Timer
{
public:
    static constexpr int kNumModules = FXChain::kNumEffects; // 9

    FXPanel()
    {
        modules[0] = std::make_unique<DistortionModulePanel>();
        modules[1] = std::make_unique<CompressorModulePanel>();
        modules[2] = std::make_unique<EQModulePanel>();
        modules[3] = std::make_unique<ChorusModulePanel>();
        modules[4] = std::make_unique<DelayModulePanel>();
        modules[5] = std::make_unique<ReverbModulePanel>();
        modules[6] = std::make_unique<PhaserModulePanel>();
        modules[7] = std::make_unique<FlangerModulePanel>();
        modules[8] = std::make_unique<HyperModulePanel>();

        for (auto& m : modules)
        {
            addAndMakeVisible (*m);
            m->onModuleDropped = [this] (int src, int tgt, bool before) { handleModuleDrop (src, tgt, before); };
        }

        // Default visual order
        for (int i = 0; i < kNumModules; ++i)
            visualOrder[static_cast<std::size_t> (i)] = i;
    }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        apvtsRef = &apvts;

        static_cast<DistortionModulePanel*> (modules[0].get())->attach (apvts);
        static_cast<CompressorModulePanel*> (modules[1].get())->attach (apvts);
        static_cast<EQModulePanel*> (modules[2].get())->attach (apvts);
        static_cast<ChorusModulePanel*> (modules[3].get())->attach (apvts);
        static_cast<DelayModulePanel*> (modules[4].get())->attach (apvts);
        static_cast<ReverbModulePanel*> (modules[5].get())->attach (apvts);
        static_cast<PhaserModulePanel*> (modules[6].get())->attach (apvts);
        static_cast<FlangerModulePanel*> (modules[7].get())->attach (apvts);
        static_cast<HyperModulePanel*> (modules[8].get())->attach (apvts);

        // Read initial order from slot parameters
        readOrderFromParameter();

        startTimerHz (15); // Poll for sync state updates and order changes
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Background
        AnalogSynthLookAndFeel::fillBrushedMetal (g, bounds, SynthColours::faceplateDark.darker (0.1f));

        // Top rail
        g.setColour (SynthColours::faceplateEdge);
        g.fillRect (bounds.getX(), bounds.getY(), bounds.getWidth(), 3.0f);

        // Bottom rail
        g.fillRect (bounds.getX(), bounds.getBottom() - 3.0f, bounds.getWidth(), 3.0f);

        // Rail screw holes (decorative dots)
        float railDotR = 2.0f;
        g.setColour (SynthColours::faceplateEdge.darker (0.3f));
        for (float x = 20.0f; x < bounds.getWidth(); x += 80.0f)
        {
            g.fillEllipse (bounds.getX() + x - railDotR, bounds.getY() + 1.5f - railDotR * 0.5f,
                           railDotR * 2.0f, railDotR);
            g.fillEllipse (bounds.getX() + x - railDotR, bounds.getBottom() - 3.0f + 0.5f,
                           railDotR * 2.0f, railDotR);
        }
    }

    void resized() override
    {
        layoutModules();
    }

    void timerCallback() override
    {
        // Update delay sync state (grey out TIME knob when sync active)
        static_cast<DelayModulePanel*> (modules[4].get())->updateSyncState();

        // Keep visual order in sync with parameter (e.g. after preset load)
        readOrderFromParameter();
    }

private:
    std::array<std::unique_ptr<FXModulePanel>, kNumModules> modules;
    std::array<int, kNumModules> visualOrder;
    juce::AudioProcessorValueTreeState* apvtsRef = nullptr;

    void handleModuleDrop (int draggedFxIndex, int targetFxIndex, bool insertBefore)
    {
        if (draggedFxIndex == targetFxIndex)
            return;

        // Find visual positions
        int draggedVisualPos = -1;
        int dropTargetVisualPos = -1;
        for (int i = 0; i < kNumModules; ++i)
        {
            if (visualOrder[static_cast<std::size_t> (i)] == draggedFxIndex)
                draggedVisualPos = i;
            if (visualOrder[static_cast<std::size_t> (i)] == targetFxIndex)
                dropTargetVisualPos = i;
        }

        if (draggedVisualPos < 0 || dropTargetVisualPos < 0)
            return;

        // Remove the dragged item and build a temp array without it
        std::vector<int> temp;
        temp.reserve (kNumModules);
        for (int i = 0; i < kNumModules; ++i)
        {
            if (visualOrder[static_cast<std::size_t> (i)] != draggedFxIndex)
                temp.push_back (visualOrder[static_cast<std::size_t> (i)]);
        }

        // Determine insert position in the temp array
        int insertPos = 0;
        for (int i = 0; i < static_cast<int> (temp.size()); ++i)
        {
            if (temp[static_cast<std::size_t> (i)] == targetFxIndex)
            {
                insertPos = insertBefore ? i : i + 1;
                break;
            }
        }

        temp.insert (temp.begin() + insertPos, draggedFxIndex);

        for (int i = 0; i < kNumModules; ++i)
            visualOrder[static_cast<std::size_t> (i)] = temp[static_cast<std::size_t> (i)];

        // Write to slot parameters
        if (apvtsRef != nullptr)
        {
            for (int i = 0; i < kNumModules; ++i)
            {
                if (auto* param = apvtsRef->getParameter (ParameterIDs::fxOrderSlotIDs[i]))
                    param->setValueNotifyingHost (param->convertTo0to1 (
                        static_cast<float> (visualOrder[static_cast<std::size_t> (i)])));
            }
        }

        layoutModules();
        repaint();
    }

    void readOrderFromParameter()
    {
        if (apvtsRef == nullptr) return;

        // Read all 9 slot params
        std::array<int, kNumModules> newOrder;
        for (int i = 0; i < kNumModules; ++i)
            newOrder[static_cast<std::size_t> (i)] = static_cast<int> (
                apvtsRef->getRawParameterValue (ParameterIDs::fxOrderSlotIDs[i])->load());

        // Migration: if all slots are at identity defaults and old factoradic != 0
        bool allDefaults = true;
        for (int i = 0; i < kNumModules; ++i)
        {
            if (newOrder[static_cast<std::size_t> (i)] != i)
            {
                allDefaults = false;
                break;
            }
        }
        if (allDefaults)
        {
            int oldOrder = static_cast<int> (apvtsRef->getRawParameterValue (ParameterIDs::fxOrder)->load());
            if (oldOrder != 0)
            {
                auto legacy = FXOrderUtils::permutationFromIndex (oldOrder);
                for (int i = 0; i < FXOrderUtils::kNumEffects; ++i)
                    newOrder[static_cast<std::size_t> (i)] = legacy[static_cast<std::size_t> (i)];
                newOrder[6] = FXChain::kPhaser;
                newOrder[7] = FXChain::kFlanger;
                newOrder[8] = FXChain::kHyper;
            }
        }

        // Validate: each index must appear exactly once
        std::array<bool, kNumModules> seen {};
        bool valid = true;
        for (int i = 0; i < kNumModules; ++i)
        {
            int idx = newOrder[static_cast<std::size_t> (i)];
            if (idx < 0 || idx >= kNumModules || seen[static_cast<std::size_t> (idx)])
            {
                valid = false;
                break;
            }
            seen[static_cast<std::size_t> (idx)] = true;
        }

        if (valid && newOrder != visualOrder)
        {
            visualOrder = newOrder;
            layoutModules();
        }
    }

    void layoutModules()
    {
        auto bounds = getLocalBounds().reduced (0, 4); // inset from rails
        int moduleW = bounds.getWidth() / kNumModules;
        int gap = 2;

        for (int i = 0; i < kNumModules; ++i)
        {
            int fxIdx = visualOrder[static_cast<std::size_t> (i)];
            auto moduleBounds = juce::Rectangle<int> (
                bounds.getX() + i * moduleW + gap / 2,
                bounds.getY(),
                moduleW - gap,
                bounds.getHeight());
            modules[static_cast<std::size_t> (fxIdx)]->setBounds (moduleBounds);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FXPanel)
};
