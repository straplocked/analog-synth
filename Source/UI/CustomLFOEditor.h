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
#include <algorithm>

class CustomLFOEditor : public juce::Component
{
public:
    CustomLFOEditor()
    {
        // Default: simple ramp from -1 to +1
        numPoints = 2;
        pointsX[0] = 0.0f;  pointsY[0] = -1.0f;
        pointsX[1] = 1.0f;  pointsY[1] = 1.0f;
    }

    void setAccentColour (juce::Colour c) { accent = c; repaint(); }

    int getNumPoints() const { return numPoints; }

    void setPoint (int index, float x, float y)
    {
        if (index >= 0 && index < LFO::kMaxCustomPoints)
        {
            pointsX[index] = std::clamp (x, 0.0f, 1.0f);
            pointsY[index] = std::clamp (y, -1.0f, 1.0f);
        }
    }

    void setNumPoints (int n)
    {
        numPoints = std::clamp (n, 2, LFO::kMaxCustomPoints);
    }

    float getPointX (int index) const { return (index >= 0 && index < numPoints) ? pointsX[index] : 0.0f; }
    float getPointY (int index) const { return (index >= 0 && index < numPoints) ? pointsY[index] : 0.0f; }

    std::function<void()> onChange;

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (1.0f);

        // Background
        g.setColour (SynthColours::panelBg.darker (0.3f));
        g.fillRoundedRectangle (bounds, 3.0f);

        // Center line (zero)
        float midY = bounds.getCentreY();
        g.setColour (SynthColours::silkscreen.withAlpha (0.2f));
        g.drawHorizontalLine (static_cast<int> (midY), bounds.getX(), bounds.getRight());

        // Quarter lines
        g.setColour (SynthColours::silkscreen.withAlpha (0.1f));
        g.drawHorizontalLine (static_cast<int> (bounds.getY() + bounds.getHeight() * 0.25f), bounds.getX(), bounds.getRight());
        g.drawHorizontalLine (static_cast<int> (bounds.getY() + bounds.getHeight() * 0.75f), bounds.getX(), bounds.getRight());

        if (numPoints < 2) return;

        // Draw waveform path
        juce::Path path;
        auto pt0 = pointToScreen (0, bounds);
        path.startNewSubPath (pt0);

        for (int i = 1; i < numPoints; ++i)
            path.lineTo (pointToScreen (i, bounds));

        g.setColour (accent.withAlpha (0.8f));
        g.strokePath (path, juce::PathStrokeType (2.0f));

        // Fill under curve
        juce::Path fillPath (path);
        fillPath.lineTo (pointToScreen (numPoints - 1, bounds).getX(), midY);
        fillPath.lineTo (pointToScreen (0, bounds).getX(), midY);
        fillPath.closeSubPath();
        g.setColour (accent.withAlpha (0.15f));
        g.fillPath (fillPath);

        // Draw points
        for (int i = 0; i < numPoints; ++i)
        {
            auto pt = pointToScreen (i, bounds);
            float radius = (i == draggedPoint) ? 6.0f : 4.0f;

            g.setColour (accent);
            g.fillEllipse (pt.getX() - radius, pt.getY() - radius, radius * 2.0f, radius * 2.0f);

            g.setColour (SynthColours::silkscreen);
            g.drawEllipse (pt.getX() - radius, pt.getY() - radius, radius * 2.0f, radius * 2.0f, 1.0f);
        }

        // Border
        g.setColour (SynthColours::silkscreen.withAlpha (0.3f));
        g.drawRoundedRectangle (bounds, 3.0f, 1.0f);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (1.0f);

        // Check if we're clicking near an existing point
        draggedPoint = findNearestPoint (e.position, bounds);

        if (draggedPoint >= 0)
        {
            // Double-click to delete (except first/last)
            if (e.getNumberOfClicks() >= 2 && draggedPoint > 0 && draggedPoint < numPoints - 1)
            {
                removePoint (draggedPoint);
                draggedPoint = -1;
                repaint();
                if (onChange) onChange();
                return;
            }
        }
        else
        {
            // Click on empty space: add a point
            auto pos = screenToPoint (e.position, bounds);
            addPoint (pos.getX(), pos.getY());
            draggedPoint = findNearestPoint (e.position, bounds);
            repaint();
            if (onChange) onChange();
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (draggedPoint < 0) return;

        auto bounds = getLocalBounds().toFloat().reduced (1.0f);
        auto pos = screenToPoint (e.position, bounds);

        float newX = std::clamp (pos.getX(), 0.0f, 1.0f);
        float newY = std::clamp (pos.getY(), -1.0f, 1.0f);

        // First and last points are pinned to x=0 and x=1
        if (draggedPoint == 0)
            newX = 0.0f;
        else if (draggedPoint == numPoints - 1)
            newX = 1.0f;
        else
        {
            // Constrain X between neighbors
            float minX = pointsX[draggedPoint - 1] + 0.001f;
            float maxX = pointsX[draggedPoint + 1] - 0.001f;
            newX = std::clamp (newX, minX, maxX);
        }

        pointsX[draggedPoint] = newX;
        pointsY[draggedPoint] = newY;

        repaint();
        if (onChange) onChange();
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        draggedPoint = -1;
        repaint();
    }

private:
    juce::Point<float> pointToScreen (int index, juce::Rectangle<float> bounds) const
    {
        float x = bounds.getX() + pointsX[index] * bounds.getWidth();
        float y = bounds.getCentreY() - pointsY[index] * (bounds.getHeight() * 0.5f);
        return { x, y };
    }

    juce::Point<float> screenToPoint (juce::Point<float> screen, juce::Rectangle<float> bounds) const
    {
        float x = (screen.getX() - bounds.getX()) / bounds.getWidth();
        float y = (bounds.getCentreY() - screen.getY()) / (bounds.getHeight() * 0.5f);
        return { std::clamp (x, 0.0f, 1.0f), std::clamp (y, -1.0f, 1.0f) };
    }

    int findNearestPoint (juce::Point<float> screen, juce::Rectangle<float> bounds) const
    {
        constexpr float hitRadius = 10.0f;
        int nearest = -1;
        float nearestDist = hitRadius * hitRadius;

        for (int i = 0; i < numPoints; ++i)
        {
            auto pt = pointToScreen (i, bounds);
            float dx = screen.getX() - pt.getX();
            float dy = screen.getY() - pt.getY();
            float dist2 = dx * dx + dy * dy;
            if (dist2 < nearestDist)
            {
                nearestDist = dist2;
                nearest = i;
            }
        }
        return nearest;
    }

    void addPoint (float x, float y)
    {
        if (numPoints >= LFO::kMaxCustomPoints) return;

        // Find insertion position to keep sorted by X
        int insertPos = numPoints;
        for (int i = 0; i < numPoints; ++i)
        {
            if (x < pointsX[i])
            {
                insertPos = i;
                break;
            }
        }

        // Shift points right
        for (int i = numPoints; i > insertPos; --i)
        {
            pointsX[i] = pointsX[i - 1];
            pointsY[i] = pointsY[i - 1];
        }

        pointsX[insertPos] = std::clamp (x, 0.0f, 1.0f);
        pointsY[insertPos] = std::clamp (y, -1.0f, 1.0f);
        ++numPoints;
    }

    void removePoint (int index)
    {
        if (index <= 0 || index >= numPoints - 1) return; // keep first/last
        if (numPoints <= 2) return;

        for (int i = index; i < numPoints - 1; ++i)
        {
            pointsX[i] = pointsX[i + 1];
            pointsY[i] = pointsY[i + 1];
        }
        --numPoints;
    }

    int numPoints = 2;
    float pointsX[LFO::kMaxCustomPoints] = {};
    float pointsY[LFO::kMaxCustomPoints] = {};
    int draggedPoint = -1;
    juce::Colour accent { SynthColours::sectionLfo };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomLFOEditor)
};
