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
#include "AnalogSynthLookAndFeel.h"
#include "../DSP/LFO.h"

class StepSeqEditor : public juce::Component
{
public:
    StepSeqEditor()
    {
        for (int i = 0; i < LFO::kMaxStepSeqSteps; ++i)
            stepValues[i] = 0.0f;
    }

    void setAccentColour (juce::Colour c) { accent = c; repaint(); }

    void setNumSteps (int n)
    {
        numSteps = std::clamp (n, 1, LFO::kMaxStepSeqSteps);
        repaint();
    }

    int getNumSteps() const { return numSteps; }

    void setStepValue (int index, float value)
    {
        if (index >= 0 && index < LFO::kMaxStepSeqSteps)
        {
            stepValues[index] = std::clamp (value, -1.0f, 1.0f);
            repaint();
        }
    }

    float getStepValue (int index) const
    {
        if (index >= 0 && index < LFO::kMaxStepSeqSteps)
            return stepValues[index];
        return 0.0f;
    }

    std::function<void()> onChange;

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (1.0f);

        // Background
        g.setColour (SynthColours::panelBg.darker (0.3f));
        g.fillRoundedRectangle (bounds, 3.0f);

        // Grid
        float stepW = bounds.getWidth() / static_cast<float> (numSteps);
        float midY = bounds.getCentreY();

        // Center line (zero)
        g.setColour (SynthColours::silkscreen.withAlpha (0.2f));
        g.drawHorizontalLine (static_cast<int> (midY), bounds.getX(), bounds.getRight());

        // Draw step bars
        for (int i = 0; i < numSteps; ++i)
        {
            float x = bounds.getX() + static_cast<float> (i) * stepW;
            float val = stepValues[i];

            // Bar rect: from center line to value
            float barTop, barH;
            if (val >= 0.0f)
            {
                barH = val * (midY - bounds.getY());
                barTop = midY - barH;
            }
            else
            {
                barTop = midY;
                barH = -val * (bounds.getBottom() - midY);
            }

            auto barRect = juce::Rectangle<float> (x + 1.0f, barTop, stepW - 2.0f, barH);

            // Fill
            g.setColour (accent.withAlpha (0.6f));
            g.fillRect (barRect);

            // Border
            g.setColour (accent.withAlpha (0.9f));
            g.drawRect (barRect, 1.0f);

            // Step separator
            g.setColour (SynthColours::silkscreen.withAlpha (0.1f));
            g.drawVerticalLine (static_cast<int> (x), bounds.getY(), bounds.getBottom());
        }

        // Border
        g.setColour (SynthColours::silkscreen.withAlpha (0.3f));
        g.drawRoundedRectangle (bounds, 3.0f, 1.0f);
    }

    void mouseDown (const juce::MouseEvent& e) override { setStepFromMouse (e); }
    void mouseDrag (const juce::MouseEvent& e) override { setStepFromMouse (e); }

private:
    void setStepFromMouse (const juce::MouseEvent& e)
    {
        auto bounds = getLocalBounds().toFloat().reduced (1.0f);
        float stepW = bounds.getWidth() / static_cast<float> (numSteps);

        int step = static_cast<int> ((static_cast<float> (e.x) - bounds.getX()) / stepW);
        step = std::clamp (step, 0, numSteps - 1);

        // Map Y to value: top = +1, center = 0, bottom = -1
        float t = (static_cast<float> (e.y) - bounds.getY()) / bounds.getHeight();
        float val = 1.0f - 2.0f * t;
        val = std::clamp (val, -1.0f, 1.0f);

        stepValues[step] = val;
        repaint();

        if (onChange)
            onChange();
    }

    int numSteps = 16;
    float stepValues[LFO::kMaxStepSeqSteps] = {};
    juce::Colour accent { SynthColours::sectionLfo };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StepSeqEditor)
};
