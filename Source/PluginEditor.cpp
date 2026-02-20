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

#include "PluginEditor.h"

AnalogSynthEditor::AnalogSynthEditor (AnalogSynthProcessor& p)
    : AudioProcessorEditor (p)
{
    setLookAndFeel (&lookAndFeel);
    setSize (1400, 800);

    mainPanel.attach (p.apvts);
    mainPanel.setPresetManager (&p.presetManager);
    mainPanel.setMidiLearnManager (&p.midiLearnManager);
    mainPanel.setKeyboardState (&p.keyboardState);
    mainPanel.setEnvelopeLevels (p.getEnvLevelFilter(), p.getEnvLevelAmp(),
                                 p.getEnvLevelEnv3(), p.getEnvLevelEnv4());
    addAndMakeVisible (mainPanel);
}

AnalogSynthEditor::~AnalogSynthEditor()
{
    setLookAndFeel (nullptr);
}

void AnalogSynthEditor::paint (juce::Graphics& g)
{
    g.fillAll (SynthColours::background);
}

void AnalogSynthEditor::resized()
{
    mainPanel.setBounds (getLocalBounds());
}
