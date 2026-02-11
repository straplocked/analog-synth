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

class ADSREditor : public juce::Component,
                   private juce::Timer
{
public:
    ADSREditor (const juce::String& titleText,
                juce::Colour colour = SynthColours::accent)
        : title (titleText), accentColour (colour)
    {
        startTimerHz (30);
    }

    ~ADSREditor() override { stopTimer(); }

    void attach (juce::AudioProcessorValueTreeState& apvts,
                 const juce::String& attackID,
                 const juce::String& decayID,
                 const juce::String& sustainID,
                 const juce::String& releaseID)
    {
        attackParam  = apvts.getRawParameterValue (attackID);
        decayParam   = apvts.getRawParameterValue (decayID);
        sustainParam = apvts.getRawParameterValue (sustainID);
        releaseParam = apvts.getRawParameterValue (releaseID);

        attackAttach  = std::make_unique<Attach> (apvts, attackID,  attackSlider);
        decayAttach   = std::make_unique<Attach> (apvts, decayID,   decaySlider);
        sustainAttach = std::make_unique<Attach> (apvts, sustainID, sustainSlider);
        releaseAttach = std::make_unique<Attach> (apvts, releaseID, releaseSlider);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Title drawn above the LCD bezel in silkscreen style
        auto titleArea = bounds.removeFromTop (18.0f);
        g.setFont (juce::Font (11.0f).boldened());
        g.setColour (juce::Colours::black.withAlpha (0.3f));
        g.drawText (title, titleArea.translated (0, -0.5f).reduced (8, 0), juce::Justification::centredLeft, false);
        g.setColour (accentColour);
        g.drawText (title, titleArea.reduced (8, 0), juce::Justification::centredLeft, false);

        // LCD display with beveled bezel
        AnalogSynthLookAndFeel::drawLCDBackground (g, bounds, 2.0f);

        if (attackParam == nullptr) return;

        auto area = bounds.reduced (8, 6);
        area.removeFromBottom (14);

        float attack  = attackParam->load();
        float decay   = decayParam->load();
        float sustain = sustainParam->load();
        float release = releaseParam->load();

        // Normalize times: use log scale for visual representation
        float maxTime = 10.0f;
        auto timeToX = [&] (float t) {
            return std::log (1.0f + t * 10.0f) / std::log (1.0f + maxTime * 10.0f);
        };

        float totalNorm = timeToX (attack) + timeToX (decay) + 0.15f + timeToX (release);

        float aW = (timeToX (attack)  / totalNorm) * area.getWidth();
        float dW = (timeToX (decay)   / totalNorm) * area.getWidth();
        float sW = (0.15f             / totalNorm) * area.getWidth();
        float rW = (timeToX (release) / totalNorm) * area.getWidth();

        float x0 = area.getX();
        float top = area.getY();
        float bot = area.getBottom();
        float sustainY = top + (1.0f - sustain) * (bot - top);

        // Build envelope path with exponential curves
        juce::Path envPath;
        envPath.startNewSubPath (x0, bot);

        // Attack: exponential curve up
        int segments = 20;
        for (int i = 1; i <= segments; ++i)
        {
            float t = static_cast<float> (i) / static_cast<float> (segments);
            float expT = 1.0f - std::exp (-4.0f * t);
            expT /= (1.0f - std::exp (-4.0f));
            envPath.lineTo (x0 + t * aW, bot - expT * (bot - top));
        }

        // Decay: exponential curve down to sustain
        float decayStartX = x0 + aW;
        for (int i = 1; i <= segments; ++i)
        {
            float t = static_cast<float> (i) / static_cast<float> (segments);
            float expT = std::exp (-4.0f * t);
            float y = top + (1.0f - expT) * (sustainY - top);
            envPath.lineTo (decayStartX + t * dW, y);
        }

        // Sustain hold
        float sustainStartX = x0 + aW + dW;
        envPath.lineTo (sustainStartX + sW, sustainY);

        // Release: exponential curve down to zero
        float releaseStartX = sustainStartX + sW;
        for (int i = 1; i <= segments; ++i)
        {
            float t = static_cast<float> (i) / static_cast<float> (segments);
            float expT = std::exp (-4.0f * t);
            float y = sustainY + (1.0f - expT) * (bot - sustainY);
            envPath.lineTo (releaseStartX + t * rW, y);
        }

        // Fill under curve (phosphor glow)
        juce::Path fillPath (envPath);
        fillPath.lineTo (area.getRight(), bot);
        fillPath.lineTo (x0, bot);
        fillPath.closeSubPath();

        g.setColour (SynthColours::lcdWaveform.withAlpha (0.06f));
        g.fillPath (fillPath);

        // Glow pass
        g.setColour (SynthColours::lcdWaveform.withAlpha (0.15f));
        g.strokePath (envPath, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));

        // Draw curve
        g.setColour (SynthColours::lcdWaveform);
        g.strokePath (envPath, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));

        // Draw nodes (bright green with glow)
        float nodeRadius = 3.5f;
        auto drawNode = [&] (float nx, float ny) {
            g.setColour (SynthColours::lcdWaveform.withAlpha (0.2f));
            g.fillEllipse (nx - nodeRadius - 2.0f, ny - nodeRadius - 2.0f,
                          (nodeRadius + 2.0f) * 2.0f, (nodeRadius + 2.0f) * 2.0f);
            g.setColour (SynthColours::lcdWaveform);
            g.fillEllipse (nx - nodeRadius, ny - nodeRadius, nodeRadius * 2, nodeRadius * 2);
            g.setColour (SynthColours::lcdBackground);
            g.fillEllipse (nx - nodeRadius + 1.5f, ny - nodeRadius + 1.5f,
                          nodeRadius * 2 - 3.0f, nodeRadius * 2 - 3.0f);
        };

        drawNode (x0, bot);
        drawNode (x0 + aW, top);
        drawNode (x0 + aW + dW, sustainY);
        drawNode (sustainStartX + sW, sustainY);
        drawNode (releaseStartX + rW, bot);

        // Stage labels in LCD text color
        g.setColour (SynthColours::lcdText.withAlpha (0.6f));
        g.setFont (juce::Font (9.0f));

        auto labelY = area.getBottom() + 2;
        g.drawText ("A", juce::Rectangle<float> (x0, labelY, aW, 12),
                    juce::Justification::centred);
        g.drawText ("D", juce::Rectangle<float> (x0 + aW, labelY, dW, 12),
                    juce::Justification::centred);
        g.drawText ("S", juce::Rectangle<float> (sustainStartX, labelY, sW, 12),
                    juce::Justification::centred);
        g.drawText ("R", juce::Rectangle<float> (releaseStartX, labelY, rW, 12),
                    juce::Justification::centred);
    }

    void resized() override {}

private:
    using Attach = juce::AudioProcessorValueTreeState::SliderAttachment;

    void timerCallback() override { repaint(); }

    juce::String title;
    juce::Colour accentColour;

    std::atomic<float>* attackParam  = nullptr;
    std::atomic<float>* decayParam   = nullptr;
    std::atomic<float>* sustainParam = nullptr;
    std::atomic<float>* releaseParam = nullptr;

    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;
    std::unique_ptr<Attach> attackAttach, decayAttach, sustainAttach, releaseAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ADSREditor)
};
