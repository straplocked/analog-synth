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

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "AnalogSynthLookAndFeel.h"
#include <atomic>
#include <array>

class WaveformDisplay : public juce::Component,
                        private juce::Timer
{
public:
    static constexpr int kBufferSize = 2048;
    static constexpr int kDisplaySamples = 256;

    WaveformDisplay()
    {
        writeIndex.store (0);
        std::fill (ringBuffer.begin(), ringBuffer.end(), 0.0f);
        startTimerHz (30);
    }

    ~WaveformDisplay() override { stopTimer(); }

    // Call from audio thread
    void pushSamples (const float* data, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            int idx = writeIndex.load (std::memory_order_relaxed);
            ringBuffer[static_cast<size_t> (idx)] = data[i];
            writeIndex.store ((idx + 1) % kBufferSize, std::memory_order_release);
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // LCD display with beveled bezel
        AnalogSynthLookAndFeel::drawLCDBackground (g, bounds, 3.0f);

        auto area = bounds.reduced (11, 11);  // inside bezel + margin

        // Read snapshot from ring buffer
        std::array<float, kDisplaySamples> snapshot {};
        int currentWrite = writeIndex.load (std::memory_order_acquire);

        // Find a zero-crossing trigger point for stable display
        int startIdx = (currentWrite - kDisplaySamples * 2 + kBufferSize) % kBufferSize;
        int triggerOffset = 0;

        for (int i = 0; i < kDisplaySamples; ++i)
        {
            int idx0 = (startIdx + i) % kBufferSize;
            int idx1 = (startIdx + i + 1) % kBufferSize;
            if (ringBuffer[static_cast<size_t> (idx0)] <= 0.0f &&
                ringBuffer[static_cast<size_t> (idx1)] > 0.0f)
            {
                triggerOffset = i;
                break;
            }
        }

        int readStart = (startIdx + triggerOffset) % kBufferSize;
        for (int i = 0; i < kDisplaySamples; ++i)
            snapshot[static_cast<size_t> (i)] = ringBuffer[static_cast<size_t> ((readStart + i) % kBufferSize)];

        // Centre line
        g.setColour (SynthColours::lcdGrid.brighter (0.1f));
        float centreY = area.getCentreY();
        g.drawHorizontalLine (static_cast<int> (centreY), area.getX(), area.getRight());

        // Waveform path
        juce::Path wavePath;
        float halfHeight = area.getHeight() * 0.45f;

        for (int i = 0; i < kDisplaySamples; ++i)
        {
            float xPos = area.getX() + (static_cast<float> (i) / static_cast<float> (kDisplaySamples - 1)) * area.getWidth();
            float sample = juce::jlimit (-1.0f, 1.0f, snapshot[static_cast<size_t> (i)]);
            float yPos = centreY - sample * halfHeight;

            if (i == 0)
                wavePath.startNewSubPath (xPos, yPos);
            else
                wavePath.lineTo (xPos, yPos);
        }

        // Phosphor green glow: wider dim stroke first
        g.setColour (SynthColours::lcdWaveform.withAlpha (0.12f));
        g.strokePath (wavePath, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

        g.setColour (SynthColours::lcdWaveform.withAlpha (0.35f));
        g.strokePath (wavePath, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

        // Main waveform stroke
        g.setColour (SynthColours::lcdWaveform);
        g.strokePath (wavePath, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
    }

    void resized() override {}

private:
    void timerCallback() override { repaint(); }

    std::array<float, kBufferSize> ringBuffer;
    std::atomic<int> writeIndex { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformDisplay)
};
