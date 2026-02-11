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
#include <juce_dsp/juce_dsp.h>
#include "AnalogSynthLookAndFeel.h"
#include <atomic>
#include <array>

class WaveformDisplay : public juce::Component,
                        private juce::Timer
{
public:
    static constexpr int kBufferSize = 2048;
    static constexpr int kDisplaySamples = 256;
    static constexpr int kFFTOrder = 11;   // 2048-point FFT
    static constexpr int kFFTSize = 1 << kFFTOrder;

    WaveformDisplay()
        : fft (kFFTOrder)
    {
        writeIndex.store (0);
        std::fill (ringBuffer.begin(), ringBuffer.end(), 0.0f);
        std::fill (fftSmoothed.begin(), fftSmoothed.end(), -100.0f);

        // Precompute Hann window
        for (int i = 0; i < kFFTSize; ++i)
            hannWindow[static_cast<size_t> (i)] = 0.5f * (1.0f - std::cos (2.0f * juce::MathConstants<float>::pi * static_cast<float> (i) / static_cast<float> (kFFTSize)));

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

    void mouseDown (const juce::MouseEvent&) override
    {
        showSpectrum = ! showSpectrum;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // LCD display with beveled bezel
        AnalogSynthLookAndFeel::drawLCDBackground (g, bounds, 3.0f);

        auto area = bounds.reduced (11, 11);  // inside bezel + margin

        if (showSpectrum)
            paintSpectrum (g, area);
        else
            paintWaveform (g, area);

        // Mode label
        g.setColour (SynthColours::lcdGrid.brighter (0.3f));
        g.setFont (juce::Font (9.0f));
        g.drawText (showSpectrum ? "SPECTRUM" : "SCOPE",
                    area.removeFromTop (10).removeFromRight (50),
                    juce::Justification::centredRight, false);
    }

    void resized() override {}

private:
    void timerCallback() override { repaint(); }

    void paintWaveform (juce::Graphics& g, juce::Rectangle<float> area)
    {
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

    void paintSpectrum (juce::Graphics& g, juce::Rectangle<float> area)
    {
        // Copy ring buffer into FFT input with Hann window
        std::array<float, kFFTSize * 2> fftData {};
        int currentWrite = writeIndex.load (std::memory_order_acquire);
        int readStart = (currentWrite - kFFTSize + kBufferSize) % kBufferSize;

        for (int i = 0; i < kFFTSize; ++i)
            fftData[static_cast<size_t> (i)] = ringBuffer[static_cast<size_t> ((readStart + i) % kBufferSize)] * hannWindow[static_cast<size_t> (i)];

        // Perform FFT
        fft.performFrequencyOnlyForwardTransform (fftData.data());

        // Convert to dB and smooth
        constexpr float minDb = -90.0f;
        constexpr float maxDb = 0.0f;
        constexpr float smoothing = 0.7f;

        int numBins = kFFTSize / 2;
        for (int i = 1; i < numBins; ++i)
        {
            float magnitude = fftData[static_cast<size_t> (i)];
            float db = (magnitude > 1e-10f)
                ? 20.0f * std::log10 (magnitude)
                : minDb;
            db = juce::jlimit (minDb, maxDb, db);

            // Exponential smoothing for visual stability
            fftSmoothed[static_cast<size_t> (i)] = fftSmoothed[static_cast<size_t> (i)] * smoothing + db * (1.0f - smoothing);
        }

        // Draw frequency grid lines (100Hz, 1kHz, 10kHz)
        g.setColour (SynthColours::lcdGrid.brighter (0.1f));
        float logMin = std::log10 (20.0f);
        float logMax = std::log10 (20000.0f);

        float freqs[] = { 100.0f, 1000.0f, 10000.0f };
        for (float freq : freqs)
        {
            float normX = (std::log10 (freq) - logMin) / (logMax - logMin);
            float xPos = area.getX() + normX * area.getWidth();
            g.drawVerticalLine (static_cast<int> (xPos), area.getY(), area.getBottom());
        }

        // Draw -30dB and -60dB lines
        for (float dbLine : { -30.0f, -60.0f })
        {
            float normY = 1.0f - (dbLine - minDb) / (maxDb - minDb);
            float yPos = area.getY() + normY * area.getHeight();
            g.drawHorizontalLine (static_cast<int> (yPos), area.getX(), area.getRight());
        }

        // Build spectrum path (log frequency axis)
        juce::Path specPath;
        bool started = false;

        // Assume 44100 sample rate for display (approximate is fine)
        float sampleRate = 44100.0f;

        for (int i = 1; i < numBins; ++i)
        {
            float freq = static_cast<float> (i) * sampleRate / static_cast<float> (kFFTSize);
            if (freq < 20.0f || freq > 20000.0f)
                continue;

            float normX = (std::log10 (freq) - logMin) / (logMax - logMin);
            float normY = 1.0f - (fftSmoothed[static_cast<size_t> (i)] - minDb) / (maxDb - minDb);

            float xPos = area.getX() + normX * area.getWidth();
            float yPos = area.getY() + normY * area.getHeight();
            yPos = juce::jlimit (area.getY(), area.getBottom(), yPos);

            if (! started)
            {
                specPath.startNewSubPath (xPos, yPos);
                started = true;
            }
            else
            {
                specPath.lineTo (xPos, yPos);
            }
        }

        // Phosphor green glow
        g.setColour (SynthColours::lcdWaveform.withAlpha (0.12f));
        g.strokePath (specPath, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

        g.setColour (SynthColours::lcdWaveform.withAlpha (0.35f));
        g.strokePath (specPath, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

        // Main spectrum stroke
        g.setColour (SynthColours::lcdWaveform);
        g.strokePath (specPath, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
    }

    juce::dsp::FFT fft;
    bool showSpectrum = false;

    std::array<float, kBufferSize> ringBuffer;
    std::atomic<int> writeIndex { 0 };

    std::array<float, kFFTSize> hannWindow {};
    std::array<float, kFFTSize / 2> fftSmoothed {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformDisplay)
};
