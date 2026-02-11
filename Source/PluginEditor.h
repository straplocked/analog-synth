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
#include "PluginProcessor.h"
#include "UI/AnalogSynthLookAndFeel.h"
#include "UI/MainPanel.h"

class AnalogSynthEditor : public juce::AudioProcessorEditor
{
public:
    explicit AnalogSynthEditor (AnalogSynthProcessor&);
    ~AnalogSynthEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    AnalogSynthProcessor& getProcessor() { return *static_cast<AnalogSynthProcessor*> (getAudioProcessor()); }

    WaveformDisplay& getWaveformDisplay() { return mainPanel.getWaveformDisplay(); }

    float getPitchBend() const { return mainPanel.getPitchBend(); }
    float getModWheel() const  { return mainPanel.getModWheel(); }

private:
    AnalogSynthLookAndFeel lookAndFeel;
    MainPanel mainPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalogSynthEditor)
};
