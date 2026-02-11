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

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

class FXBase
{
public:
    virtual ~FXBase() = default;

    virtual void prepare (double sampleRate, int samplesPerBlock) = 0;
    virtual void processBlock (juce::AudioBuffer<float>& buffer) = 0;
    virtual void reset() = 0;
    virtual void updateParameters (juce::AudioProcessorValueTreeState& apvts) = 0;

    virtual juce::String getEffectID() const = 0;
    virtual juce::String getDisplayName() const = 0;

    void setEnabled (bool e) { enabled = e; }
    bool isEnabled() const { return enabled; }

protected:
    bool enabled = false;
};
