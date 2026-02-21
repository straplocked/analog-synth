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
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "AnalogSynthLookAndFeel.h"
#include "FilterResponseComputer.h"
#include "../ParameterIDs.h"
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
    static constexpr int kOverlayPoints = 200;

    enum class DisplayMode { Scope, Spectrum, SpectrumFilter, SpectrumEQ, SpectrumAll };

    struct FilterOverlayParams
    {
        int   lpfType = 0, lpfSlope = 1;
        float lpfCutoff = 8000.0f, lpfResonance = 0.0f;
        float hpfCutoff = 20.0f;
        bool  eqEnabled = false;
        std::array<float, 6> eqLowCoeffs  {{ 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f }};
        std::array<float, 6> eqMidCoeffs  {{ 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f }};
        std::array<float, 6> eqHighCoeffs {{ 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f }};
    };

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

    void setSampleRate (float sr) { currentSampleRate.store (sr, std::memory_order_relaxed); }

    void connectToAPVTS (juce::AudioProcessorValueTreeState* a) { apvtsPtr = a; }

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

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
        {
            juce::PopupMenu menu;
            menu.addItem (1, "Scope",            true, displayMode == DisplayMode::Scope);
            menu.addItem (2, "Spectrum",         true, displayMode == DisplayMode::Spectrum);
            menu.addItem (3, "Spectrum + Filter", true, displayMode == DisplayMode::SpectrumFilter);
            menu.addItem (4, "Spectrum + EQ",    true, displayMode == DisplayMode::SpectrumEQ);
            menu.addItem (5, "Spectrum + All",   true, displayMode == DisplayMode::SpectrumAll);

            menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                [this] (int result)
                {
                    switch (result)
                    {
                        case 1: displayMode = DisplayMode::Scope; break;
                        case 2: displayMode = DisplayMode::Spectrum; break;
                        case 3: displayMode = DisplayMode::SpectrumFilter; break;
                        case 4: displayMode = DisplayMode::SpectrumEQ; break;
                        case 5: displayMode = DisplayMode::SpectrumAll; break;
                        default: break;
                    }
                    repaint();
                });
            return;
        }

        // Left-click: toggle between Scope and last spectrum mode
        if (displayMode == DisplayMode::Scope)
            displayMode = lastSpectrumMode;
        else
        {
            lastSpectrumMode = displayMode;
            displayMode = DisplayMode::Scope;
        }
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // LCD display with beveled bezel
        AnalogSynthLookAndFeel::drawLCDBackground (g, bounds, 3.0f);

        auto area = bounds.reduced (11, 11);  // inside bezel + margin

        bool isSpectrum = displayMode != DisplayMode::Scope;

        if (isSpectrum)
        {
            paintSpectrum (g, area);

            bool showFilter = displayMode == DisplayMode::SpectrumFilter || displayMode == DisplayMode::SpectrumAll;
            bool showEQ     = displayMode == DisplayMode::SpectrumEQ     || displayMode == DisplayMode::SpectrumAll;

            if (showFilter)
                paintFilterOverlay (g, area);
            if (showEQ)
                paintEQOverlay (g, area);

            paintAxisLabels (g, area);
        }
        else
        {
            paintWaveform (g, area);
        }

        // Mode label
        g.setColour (SynthColours::lcdGrid.brighter (0.3f));
        g.setFont (juce::Font (9.0f));
        juce::String modeStr;
        switch (displayMode)
        {
            case DisplayMode::Scope:          modeStr = "SCOPE"; break;
            case DisplayMode::Spectrum:       modeStr = "SPECTRUM"; break;
            case DisplayMode::SpectrumFilter: modeStr = "SPECTRUM+F"; break;
            case DisplayMode::SpectrumEQ:     modeStr = "SPECTRUM+EQ"; break;
            case DisplayMode::SpectrumAll:    modeStr = "SPECTRUM+"; break;
        }
        g.drawText (modeStr,
                    bounds.reduced (11, 11).removeFromTop (10).removeFromRight (70),
                    juce::Justification::centredRight, false);
    }

    void resized() override {}

private:
    void timerCallback() override
    {
        if (apvtsPtr != nullptr && displayMode != DisplayMode::Scope)
        {
            auto& apvts = *apvtsPtr;
            float sr = currentSampleRate.load (std::memory_order_relaxed);

            // Snapshot filter parameters
            overlayParams.lpfType      = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::lpfType)->load());
            overlayParams.lpfSlope     = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::lpfSlope)->load());
            overlayParams.lpfCutoff    = apvts.getRawParameterValue (ParameterIDs::lpfCutoff)->load();
            overlayParams.lpfResonance = apvts.getRawParameterValue (ParameterIDs::lpfResonance)->load();
            overlayParams.hpfCutoff    = apvts.getRawParameterValue (ParameterIDs::hpfCutoff)->load();

            // EQ parameters
            overlayParams.eqEnabled = apvts.getRawParameterValue (ParameterIDs::eqEnabled)->load() > 0.5f;

            if (overlayParams.eqEnabled)
            {
                float safeNyquist = sr * 0.49f;
                float lf = std::min (apvts.getRawParameterValue (ParameterIDs::eqLowFreq)->load(), safeNyquist);
                float lg = apvts.getRawParameterValue (ParameterIDs::eqLowGain)->load();
                float mf = std::min (apvts.getRawParameterValue (ParameterIDs::eqMidFreq)->load(), safeNyquist);
                float mg = apvts.getRawParameterValue (ParameterIDs::eqMidGain)->load();
                float mq = apvts.getRawParameterValue (ParameterIDs::eqMidQ)->load();
                float hf = std::min (apvts.getRawParameterValue (ParameterIDs::eqHighFreq)->load(), safeNyquist);
                float hg = apvts.getRawParameterValue (ParameterIDs::eqHighGain)->load();

                auto lowC = juce::dsp::IIR::Coefficients<float>::makeLowShelf (
                    static_cast<double> (sr), lf, 0.707f, juce::Decibels::decibelsToGain (lg));
                auto midC = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
                    static_cast<double> (sr), mf, mq, juce::Decibels::decibelsToGain (mg));
                auto highC = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
                    static_cast<double> (sr), hf, 0.707f, juce::Decibels::decibelsToGain (hg));

                // Extract JUCE IIR coefficients into our 6-element layout.
                // JUCE stores 5 floats: { b0, b1, b2, a1, a2 } (already normalized by a0).
                // Our biquadMagnitudeDB expects: { b0, b1, b2, a0(=1), a1, a2 }
                auto extractCoeffs = [] (const juce::dsp::IIR::Coefficients<float>::Ptr& src, std::array<float, 6>& dst)
                {
                    auto* raw = src->getRawCoefficients();
                    dst[0] = raw[0];  // b0
                    dst[1] = raw[1];  // b1
                    dst[2] = raw[2];  // b2
                    dst[3] = 1.0f;    // a0 (always 1, not stored by JUCE)
                    dst[4] = raw[3];  // a1
                    dst[5] = raw[4];  // a2
                };
                extractCoeffs (lowC, overlayParams.eqLowCoeffs);
                extractCoeffs (midC, overlayParams.eqMidCoeffs);
                extractCoeffs (highC, overlayParams.eqHighCoeffs);
            }
        }

        repaint();
    }

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
        constexpr float smoothing = 0.7f;

        int numBins = kFFTSize / 2;
        for (int i = 1; i < numBins; ++i)
        {
            float magnitude = fftData[static_cast<size_t> (i)];
            float db = (magnitude > 1e-10f)
                ? 20.0f * std::log10 (magnitude)
                : kMinDb;
            db = juce::jlimit (kMinDb, kMaxDb, db);

            // Exponential smoothing for visual stability
            fftSmoothed[static_cast<size_t> (i)] = fftSmoothed[static_cast<size_t> (i)] * smoothing + db * (1.0f - smoothing);
        }

        // Draw frequency grid lines (100Hz, 1kHz, 10kHz)
        g.setColour (SynthColours::lcdGrid.brighter (0.1f));

        float freqs[] = { 100.0f, 1000.0f, 10000.0f };
        for (float freq : freqs)
        {
            float normX = freqToNormX (freq);
            float xPos = area.getX() + normX * area.getWidth();
            g.drawVerticalLine (static_cast<int> (xPos), area.getY(), area.getBottom());
        }

        // Draw -30dB and -60dB lines
        for (float dbLine : { -30.0f, -60.0f })
        {
            float normY = dbToNormY (dbLine);
            float yPos = area.getY() + normY * area.getHeight();
            g.drawHorizontalLine (static_cast<int> (yPos), area.getX(), area.getRight());
        }

        // Build spectrum path (log frequency axis)
        juce::Path specPath;
        bool started = false;

        float sampleRate = currentSampleRate.load (std::memory_order_relaxed);

        for (int i = 1; i < numBins; ++i)
        {
            float freq = static_cast<float> (i) * sampleRate / static_cast<float> (kFFTSize);
            if (freq < 20.0f || freq > 20000.0f)
                continue;

            float normX = freqToNormX (freq);
            float normY = dbToNormY (fftSmoothed[static_cast<size_t> (i)]);

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

    void paintFilterOverlay (juce::Graphics& g, juce::Rectangle<float> area)
    {
        float sr = currentSampleRate.load (std::memory_order_relaxed);
        bool twoPole = overlayParams.lpfSlope == 0;
        auto filterColour = SynthColours::sectionFilter;

        // LPF response curve
        {
            juce::Path lpfPath;
            bool started = false;

            for (int i = 0; i < kOverlayPoints; ++i)
            {
                float normX = static_cast<float> (i) / static_cast<float> (kOverlayPoints - 1);
                float freq = normXToFreq (normX);

                float db = FilterResponseComputer::ladderMagnitudeDB (
                    overlayParams.lpfCutoff, overlayParams.lpfResonance, sr, twoPole, freq);

                float ny = dbToNormY (db);
                float xPos = area.getX() + normX * area.getWidth();
                float yPos = juce::jlimit (area.getY(), area.getBottom(), area.getY() + ny * area.getHeight());

                if (! started)
                {
                    lpfPath.startNewSubPath (xPos, yPos);
                    started = true;
                }
                else
                {
                    lpfPath.lineTo (xPos, yPos);
                }
            }

            // 3-layer glow matching phosphor style
            g.setColour (filterColour.withAlpha (0.12f));
            g.strokePath (lpfPath, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));
            g.setColour (filterColour.withAlpha (0.35f));
            g.strokePath (lpfPath, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));
            g.setColour (filterColour);
            g.strokePath (lpfPath, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));
        }

        // HPF response curve (only when cutoff is above minimum threshold)
        if (overlayParams.hpfCutoff > 25.0f)
        {
            auto hpfColour = filterColour.brighter (0.3f);
            juce::Path hpfPath;
            bool started = false;

            for (int i = 0; i < kOverlayPoints; ++i)
            {
                float normX = static_cast<float> (i) / static_cast<float> (kOverlayPoints - 1);
                float freq = normXToFreq (normX);

                float db = FilterResponseComputer::hpfMagnitudeDB (overlayParams.hpfCutoff, sr, freq);

                float ny = dbToNormY (db);
                float xPos = area.getX() + normX * area.getWidth();
                float yPos = juce::jlimit (area.getY(), area.getBottom(), area.getY() + ny * area.getHeight());

                if (! started)
                {
                    hpfPath.startNewSubPath (xPos, yPos);
                    started = true;
                }
                else
                {
                    hpfPath.lineTo (xPos, yPos);
                }
            }

            g.setColour (hpfColour.withAlpha (0.12f));
            g.strokePath (hpfPath, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));
            g.setColour (hpfColour.withAlpha (0.35f));
            g.strokePath (hpfPath, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));
            g.setColour (hpfColour);
            g.strokePath (hpfPath, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));
        }

        // Dashed vertical cutoff marker at LPF cutoff frequency
        {
            float cutoffNormX = freqToNormX (overlayParams.lpfCutoff);
            float cutoffX = area.getX() + cutoffNormX * area.getWidth();

            if (cutoffX >= area.getX() && cutoffX <= area.getRight())
            {
                g.setColour (filterColour.withAlpha (0.5f));
                float dashLengths[] = { 4.0f, 3.0f };
                juce::Path sourceLine;
                sourceLine.startNewSubPath (cutoffX, area.getY());
                sourceLine.lineTo (cutoffX, area.getBottom());
                juce::Path dashedLine;
                juce::PathStrokeType stroke (1.0f);
                stroke.createDashedStroke (dashedLine, sourceLine, dashLengths, 2);
                g.strokePath (dashedLine, stroke);

                // Cutoff frequency label
                g.setColour (filterColour.withAlpha (0.8f));
                g.setFont (juce::Font (9.0f).boldened());
                g.drawText (formatFreq (overlayParams.lpfCutoff),
                            juce::Rectangle<float> (cutoffX - 20.0f, area.getBottom() - 14.0f, 40.0f, 12.0f),
                            juce::Justification::centred, false);
            }
        }
    }

    void paintEQOverlay (juce::Graphics& g, juce::Rectangle<float> area)
    {
        if (! overlayParams.eqEnabled) return;

        float sr = currentSampleRate.load (std::memory_order_relaxed);
        auto eqColour = SynthColours::accent;

        juce::Path eqPath;
        bool started = false;

        for (int i = 0; i < kOverlayPoints; ++i)
        {
            float normX = static_cast<float> (i) / static_cast<float> (kOverlayPoints - 1);
            float freq = normXToFreq (normX);

            float db = FilterResponseComputer::eqMagnitudeDB (
                overlayParams.eqLowCoeffs.data(), overlayParams.eqMidCoeffs.data(),
                overlayParams.eqHighCoeffs.data(), freq, sr);

            // EQ overlay: draw relative to 0 dB center line
            // Map dB range: -24..+24 dB into the display area
            constexpr float eqRange = 24.0f;
            float normY = 0.5f - (db / (2.0f * eqRange));
            normY = juce::jlimit (0.0f, 1.0f, normY);

            float xPos = area.getX() + normX * area.getWidth();
            float yPos = area.getY() + normY * area.getHeight();

            if (! started)
            {
                eqPath.startNewSubPath (xPos, yPos);
                started = true;
            }
            else
            {
                eqPath.lineTo (xPos, yPos);
            }
        }

        // 3-layer glow
        g.setColour (eqColour.withAlpha (0.12f));
        g.strokePath (eqPath, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
        g.setColour (eqColour.withAlpha (0.35f));
        g.strokePath (eqPath, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
        g.setColour (eqColour);
        g.strokePath (eqPath, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));

        // Draw 0 dB reference line for EQ
        g.setColour (eqColour.withAlpha (0.15f));
        float zeroY = area.getCentreY();
        g.drawHorizontalLine (static_cast<int> (zeroY), area.getX(), area.getRight());
    }

    void paintAxisLabels (juce::Graphics& g, juce::Rectangle<float> area)
    {
        auto labelColour = SynthColours::lcdGrid.brighter (0.5f);
        g.setColour (labelColour);
        g.setFont (juce::Font (8.0f));

        // Frequency labels at bottom
        struct FreqLabel { float freq; const char* text; };
        FreqLabel freqLabels[] = {
            { 50.0f, "50" }, { 100.0f, "100" }, { 500.0f, "500" },
            { 1000.0f, "1k" }, { 5000.0f, "5k" }, { 10000.0f, "10k" }, { 20000.0f, "20k" }
        };

        for (auto& fl : freqLabels)
        {
            float normX = freqToNormX (fl.freq);
            float xPos = area.getX() + normX * area.getWidth();
            g.drawText (fl.text,
                        juce::Rectangle<float> (xPos - 12.0f, area.getBottom() - 10.0f, 24.0f, 10.0f),
                        juce::Justification::centred, false);
        }

        // dB labels at left
        struct DbLabel { float db; const char* text; };
        DbLabel dbLabels[] = { { -30.0f, "-30dB" }, { -60.0f, "-60dB" } };

        for (auto& dl : dbLabels)
        {
            float normY = dbToNormY (dl.db);
            float yPos = area.getY() + normY * area.getHeight();
            g.drawText (dl.text,
                        juce::Rectangle<float> (area.getX(), yPos - 5.0f, 30.0f, 10.0f),
                        juce::Justification::centredLeft, false);
        }
    }

    // --- Coordinate helpers ---

    static float freqToNormX (float freq)
    {
        constexpr float logMin = 1.30103f;   // log10(20)
        constexpr float logMax = 4.30103f;   // log10(20000)
        return (std::log10 (std::max (freq, 20.0f)) - logMin) / (logMax - logMin);
    }

    static float normXToFreq (float normX)
    {
        constexpr float logMin = 1.30103f;
        constexpr float logMax = 4.30103f;
        return std::pow (10.0f, logMin + normX * (logMax - logMin));
    }

    static float dbToNormY (float db)
    {
        return 1.0f - (db - kMinDb) / (kMaxDb - kMinDb);
    }

    static juce::String formatFreq (float freq)
    {
        if (freq >= 1000.0f)
            return juce::String (freq / 1000.0f, 1) + "k";
        return juce::String (static_cast<int> (freq));
    }

    // --- Constants ---
    static constexpr float kMinDb = -90.0f;
    static constexpr float kMaxDb = 0.0f;

    // --- Members ---
    std::atomic<float> currentSampleRate { 44100.0f };

    juce::dsp::FFT fft;
    DisplayMode displayMode = DisplayMode::Scope;
    DisplayMode lastSpectrumMode = DisplayMode::Spectrum;

    juce::AudioProcessorValueTreeState* apvtsPtr = nullptr;
    FilterOverlayParams overlayParams;

    std::array<float, kBufferSize> ringBuffer;
    std::atomic<int> writeIndex { 0 };

    std::array<float, kFFTSize> hannWindow {};
    std::array<float, kFFTSize / 2> fftSmoothed {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformDisplay)
};
