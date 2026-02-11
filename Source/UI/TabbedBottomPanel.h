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
#include "EnvelopePanel.h"
#include "LFOPanel.h"
#include "ModMatrixPanel.h"
#include "FXPanel.h"
#include "GlobalPanel.h"

class TabbedBottomPanel : public juce::Component
{
public:
    TabbedBottomPanel()
        : tabbedComponent (juce::TabbedButtonBar::TabsAtTop)
    {
        tabbedComponent.setTabBarDepth (30);

        envPanel       = std::make_unique<EnvelopePanel>();
        lfoPanel       = std::make_unique<LFOPanel>();
        modMatrixPanel = std::make_unique<ModMatrixPanel>();
        fxPanel        = std::make_unique<FXPanel>();
        globalPanel    = std::make_unique<GlobalPanel>();

        tabbedComponent.addTab ("ENV",        SynthColours::panelBg, envPanel.get(),       false);
        tabbedComponent.addTab ("LFO",        SynthColours::panelBg, lfoPanel.get(),       false);
        tabbedComponent.addTab ("MOD MATRIX", SynthColours::panelBg, modMatrixPanel.get(), false);
        tabbedComponent.addTab ("FX",         SynthColours::panelBg, fxPanel.get(),        false);
        tabbedComponent.addTab ("GLOBAL",     SynthColours::panelBg, globalPanel.get(),    false);

        addAndMakeVisible (tabbedComponent);
    }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        envPanel->attach (apvts);
        lfoPanel->attach (apvts);
        modMatrixPanel->attach (apvts);
        fxPanel->attach (apvts);
        globalPanel->attach (apvts);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Groove line at top edge (1px dark + 1px light)
        g.setColour (SynthColours::faceplateEdge.darker (0.2f));
        g.fillRect (bounds.getX(), bounds.getY(), bounds.getWidth(), 1.0f);
        g.setColour (SynthColours::faceplateMid.brighter (0.08f));
        g.fillRect (bounds.getX(), bounds.getY() + 1.0f, bounds.getWidth(), 1.0f);
    }

    void resized() override
    {
        tabbedComponent.setBounds (getLocalBounds());
    }

private:
    juce::TabbedComponent tabbedComponent;

    std::unique_ptr<EnvelopePanel> envPanel;
    std::unique_ptr<LFOPanel> lfoPanel;
    std::unique_ptr<ModMatrixPanel> modMatrixPanel;
    std::unique_ptr<FXPanel> fxPanel;
    std::unique_ptr<GlobalPanel> globalPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TabbedBottomPanel)
};
