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

#include "AnalogSynthLookAndFeel.h"

AnalogSynthLookAndFeel::AnalogSynthLookAndFeel()
{
    fontRegular = juce::Typeface::createSystemTypefaceFor (
        BinaryData::SpaceGroteskRegular_ttf,
        BinaryData::SpaceGroteskRegular_ttfSize);

    fontBold = juce::Typeface::createSystemTypefaceFor (
        BinaryData::SpaceGroteskBold_ttf,
        BinaryData::SpaceGroteskBold_ttfSize);

    fontDisplay = juce::Typeface::createSystemTypefaceFor (
        BinaryData::AudiowideRegular_ttf,
        BinaryData::AudiowideRegular_ttfSize);

    setColourScheme (getDarkColourScheme());

    setColour (juce::TextButton::textColourOffId, SynthColours::textPrimary);
    setColour (juce::TextButton::textColourOnId, SynthColours::textPrimary);
    setColour (juce::ComboBox::backgroundColourId, SynthColours::recessDark);
    setColour (juce::ComboBox::textColourId, SynthColours::textPrimary);
    setColour (juce::ComboBox::outlineColourId, SynthColours::faceplateEdge);
    setColour (juce::Label::textColourId, SynthColours::textPrimary);
    setColour (juce::Slider::thumbColourId, SynthColours::accent);
    setColour (juce::Slider::trackColourId, SynthColours::knobTrack);
    setColour (juce::Slider::backgroundColourId, SynthColours::panelBg);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xf0222222));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0x30ffffff));
    setColour (juce::PopupMenu::textColourId, SynthColours::textPrimary);
    setColour (juce::PopupMenu::highlightedTextColourId, SynthColours::textPrimary);
}

juce::Typeface::Ptr AnalogSynthLookAndFeel::getTypefaceForFont (const juce::Font& font)
{
    if (font.isBold())
        return fontBold;
    return fontRegular;
}

//==============================================================================
// Utility helpers
//==============================================================================

void AnalogSynthLookAndFeel::fillBrushedMetal (juce::Graphics& g, juce::Rectangle<float> area,
                                                juce::Colour baseColour, float grainIntensity)
{
    g.setColour (baseColour);
    g.fillRect (area);

    // Horizontal grain lines with deterministic pseudo-random brightness variation
    int startY = static_cast<int> (area.getY());
    int endY   = static_cast<int> (area.getBottom());

    for (int y = startY; y < endY; ++y)
    {
        unsigned int hash = static_cast<unsigned int> (y * 73);
        hash ^= 0x5bd1e995;
        hash *= 0x5bd1e995;
        hash ^= hash >> 15;
        float offset = (static_cast<float> (hash % 1000) / 500.0f - 1.0f) * grainIntensity;

        auto lineColour = offset > 0.0f ? baseColour.brighter (offset) : baseColour.darker (-offset);
        g.setColour (lineColour);
        g.fillRect (area.getX(), static_cast<float> (y), area.getWidth(), 1.0f);
    }

    // Subtle edge darkening for curvature illusion
    float edgeW = area.getWidth() * 0.08f;
    juce::ColourGradient leftEdge (baseColour.darker (0.06f), area.getX(), 0,
                                   baseColour.withAlpha (0.0f), area.getX() + edgeW, 0, false);
    g.setGradientFill (leftEdge);
    g.fillRect (area.getX(), area.getY(), edgeW, area.getHeight());

    juce::ColourGradient rightEdge (baseColour.darker (0.06f), area.getRight(), 0,
                                    baseColour.withAlpha (0.0f), area.getRight() - edgeW, 0, false);
    g.setGradientFill (rightEdge);
    g.fillRect (area.getRight() - edgeW, area.getY(), edgeW, area.getHeight());

    // Micro-scratch overlay — near-horizontal hairlines simulating surface wear
    int scratchSeed = static_cast<int> (area.getY() * 31 + area.getX() * 17);
    int numScratches = static_cast<int> (area.getHeight()) / 80;

    for (int s = 0; s < numScratches; ++s)
    {
        unsigned int h = static_cast<unsigned int> ((scratchSeed + s * 7919) & 0x7fffffff);
        h ^= 0x27d4eb2d;
        h *= 0x27d4eb2d;
        h ^= h >> 16;

        float sy = area.getY() + (static_cast<float> (h % 1000) / 1000.0f) * area.getHeight();
        float sx = area.getX() + (static_cast<float> ((h >> 10) % 1000) / 1000.0f) * area.getWidth() * 0.3f;
        float scratchLen = 20.0f + static_cast<float> ((h >> 20) % 60);
        float scratchAngle = -0.05f + (static_cast<float> ((h >> 5) % 100) / 1000.0f);

        float ex = sx + scratchLen * std::cos (scratchAngle);
        float ey = sy + scratchLen * std::sin (scratchAngle);

        g.setColour (juce::Colours::white.withAlpha (0.018f));
        g.drawLine (sx, sy, ex, ey, 0.5f);
    }

    // Metallic sparkle flecks — sparse bright micro-dots
    int fleckCount = static_cast<int> (area.getWidth() * area.getHeight()) / 8000;
    for (int f = 0; f < fleckCount; ++f)
    {
        unsigned int fh = static_cast<unsigned int> (f * 6151 + static_cast<int> (area.getX()) * 29);
        fh ^= 0x12345678;
        fh *= 0x5bd1e995;
        fh ^= fh >> 15;

        float fx = area.getX() + (static_cast<float> (fh % 10000) / 10000.0f) * area.getWidth();
        float fy = area.getY() + (static_cast<float> ((fh >> 14) % 10000) / 10000.0f) * area.getHeight();

        g.setColour (juce::Colours::white.withAlpha (0.06f + (static_cast<float> ((fh >> 7) % 50) / 1000.0f)));
        g.fillRect (fx, fy, 1.0f, 1.0f);
    }
}

void AnalogSynthLookAndFeel::drawRecessedPanel (juce::Graphics& g, juce::Rectangle<float> area,
                                                  float cornerRadius, float depth)
{
    // Inner fill
    g.setColour (SynthColours::recessDark);
    g.fillRoundedRectangle (area, cornerRadius);

    // Layer 1: Ambient occlusion — soft shadow around entire inner perimeter
    {
        float aoDepth = depth * 1.5f;

        juce::ColourGradient topAO (juce::Colours::black.withAlpha (0.15f),
                                     area.getX(), area.getY(),
                                     juce::Colours::transparentBlack,
                                     area.getX(), area.getY() + aoDepth, false);
        g.setGradientFill (topAO);
        g.fillRoundedRectangle (area.getX(), area.getY(), area.getWidth(), aoDepth, cornerRadius);

        juce::ColourGradient botAO (juce::Colours::black.withAlpha (0.06f),
                                     area.getX(), area.getBottom(),
                                     juce::Colours::transparentBlack,
                                     area.getX(), area.getBottom() - aoDepth, false);
        g.setGradientFill (botAO);
        g.fillRoundedRectangle (area.getX(), area.getBottom() - aoDepth, area.getWidth(), aoDepth, cornerRadius);

        juce::ColourGradient leftAO (juce::Colours::black.withAlpha (0.12f),
                                      area.getX(), area.getY(),
                                      juce::Colours::transparentBlack,
                                      area.getX() + aoDepth, area.getY(), false);
        g.setGradientFill (leftAO);
        g.fillRoundedRectangle (area.getX(), area.getY(), aoDepth, area.getHeight(), cornerRadius);

        juce::ColourGradient rightAO (juce::Colours::black.withAlpha (0.05f),
                                       area.getRight(), area.getY(),
                                       juce::Colours::transparentBlack,
                                       area.getRight() - aoDepth, area.getY(), false);
        g.setGradientFill (rightAO);
        g.fillRoundedRectangle (area.getRight() - aoDepth, area.getY(), aoDepth, area.getHeight(), cornerRadius);
    }

    // Layer 2: Directional inner shadow (light from upper-left)
    {
        juce::ColourGradient topShadow (SynthColours::recessDark.darker (0.5f),
                                         area.getX(), area.getY(),
                                         juce::Colours::transparentBlack,
                                         area.getX(), area.getY() + depth * 3.0f, false);
        g.setGradientFill (topShadow);
        g.fillRoundedRectangle (area.getX(), area.getY(), area.getWidth(), depth * 4.0f, cornerRadius);

        juce::ColourGradient leftShadow (SynthColours::recessDark.darker (0.4f),
                                          area.getX(), area.getY(),
                                          juce::Colours::transparentBlack,
                                          area.getX() + depth * 3.0f, area.getY(), false);
        g.setGradientFill (leftShadow);
        g.fillRoundedRectangle (area.getX(), area.getY(), depth * 3.5f, area.getHeight(), cornerRadius);
    }

    // Layer 3: Warm catch light on bottom/right edges
    g.setColour (juce::Colour (0x1afff8f0));
    g.fillRect (area.getX() + cornerRadius, area.getBottom() - 1.0f,
                area.getWidth() - cornerRadius * 2.0f, 1.0f);

    g.setColour (juce::Colour (0x14fff8f0));
    g.fillRect (area.getRight() - 1.0f, area.getY() + cornerRadius,
                1.0f, area.getHeight() - cornerRadius * 2.0f);

    // Outer bevel: dark top-left, light bottom-right
    g.setColour (SynthColours::faceplateEdge.darker (0.2f));
    g.drawLine (area.getX(), area.getY(), area.getRight(), area.getY(), 1.0f);
    g.drawLine (area.getX(), area.getY(), area.getX(), area.getBottom(), 1.0f);

    g.setColour (SynthColours::faceplateMid.brighter (0.08f));
    g.drawLine (area.getX(), area.getBottom(), area.getRight(), area.getBottom(), 1.0f);
    g.drawLine (area.getRight(), area.getY(), area.getRight(), area.getBottom(), 1.0f);

    // Corner wear — slightly brighter where paint would wear from use
    float wearRadius = cornerRadius * 2.5f;
    g.setColour (juce::Colours::white.withAlpha (0.025f));
    g.fillEllipse (area.getX() - wearRadius * 0.3f, area.getY() - wearRadius * 0.3f,
                   wearRadius, wearRadius);
    g.fillEllipse (area.getRight() - wearRadius * 0.7f, area.getY() - wearRadius * 0.3f,
                   wearRadius, wearRadius);
    g.fillEllipse (area.getX() - wearRadius * 0.3f, area.getBottom() - wearRadius * 0.7f,
                   wearRadius, wearRadius);
    g.fillEllipse (area.getRight() - wearRadius * 0.7f, area.getBottom() - wearRadius * 0.7f,
                   wearRadius, wearRadius);
}

void AnalogSynthLookAndFeel::drawScrewHead (juce::Graphics& g, float cx, float cy, float radius,
                                              float rotationOverride)
{
    // Shadow behind screw
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillEllipse (cx - radius + 0.5f, cy - radius + 0.5f, radius * 2.0f, radius * 2.0f);

    // Main body with radial gradient
    juce::ColourGradient bodyGrad (SynthColours::screwHighlight, cx - radius * 0.3f, cy - radius * 0.3f,
                                   SynthColours::screwBody, cx + radius, cy + radius, true);
    g.setGradientFill (bodyGrad);
    g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

    // Edge ring
    g.setColour (SynthColours::screwSlot.withAlpha (0.6f));
    g.drawEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 0.5f);

    // Phillips cross slot — per-screw rotation variance
    float screwAngle;
    if (rotationOverride > -998.0f)
    {
        screwAngle = rotationOverride;
    }
    else
    {
        unsigned int posHash = static_cast<unsigned int> (cx * 73.0f + cy * 131.0f);
        posHash ^= 0xdeadbeef;
        posHash *= 0x01000193;
        screwAngle = (static_cast<float> (posHash % 3600) / 3600.0f) * juce::MathConstants<float>::twoPi;
    }

    float slotLen = radius * 0.7f;
    float slotW = 0.8f;

    juce::Path crossSlot;
    crossSlot.addRectangle (-slotLen, -slotW * 0.5f, slotLen * 2.0f, slotW);
    crossSlot.addRectangle (-slotW * 0.5f, -slotLen, slotW, slotLen * 2.0f);
    crossSlot.applyTransform (juce::AffineTransform::rotation (screwAngle).translated (cx, cy));

    g.setColour (SynthColours::screwSlot);
    g.fillPath (crossSlot);

    // Specular dot
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.fillEllipse (cx - radius * 0.35f, cy - radius * 0.35f, radius * 0.4f, radius * 0.4f);
}

void AnalogSynthLookAndFeel::drawSilkscreenLabel (juce::Graphics& g, const juce::String& text,
                                                    juce::Rectangle<float> area,
                                                    juce::Justification just,
                                                    float fontSize, bool embossed)
{
    g.setFont (juce::Font (fontSize).boldened());

    if (embossed)
    {
        // Light catching bottom edge
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawText (text, area.translated (0, 1.0f), just, false);

        // Dark inset above
        g.setColour (juce::Colours::black.withAlpha (0.3f));
        g.drawText (text, area.translated (0, -0.5f), just, false);
    }

    // Main text — with subtle position-dependent wear
    unsigned int wearHash = static_cast<unsigned int> (area.getX() * 41.0f + area.getY() * 67.0f);
    wearHash ^= 0xbaadf00d;
    wearHash *= 0x01000193;
    float wearFactor = 0.92f + (static_cast<float> (wearHash % 160) / 2000.0f);

    g.setColour (SynthColours::silkscreen.withMultipliedAlpha (wearFactor));
    g.drawText (text, area, just, false);
}

void AnalogSynthLookAndFeel::drawLED (juce::Graphics& g, float cx, float cy, float radius,
                                       bool isOn, juce::Colour colour)
{
    if (isOn)
    {
        // Per-LED brightness variation (component tolerance simulation)
        unsigned int ledHash = static_cast<unsigned int> (cx * 61.0f + cy * 37.0f);
        ledHash ^= 0xcafebabe;
        ledHash *= 0x01000193;
        float brightnessVar = 0.9f + (static_cast<float> (ledHash % 200) / 1000.0f);

        // Outer glow layers
        g.setColour (colour.withAlpha (0.10f * brightnessVar));
        g.fillEllipse (cx - radius - 5.0f, cy - radius - 5.0f,
                       (radius + 5.0f) * 2.0f, (radius + 5.0f) * 2.0f);

        g.setColour (colour.withAlpha (0.20f * brightnessVar));
        g.fillEllipse (cx - radius - 2.5f, cy - radius - 2.5f,
                       (radius + 2.5f) * 2.0f, (radius + 2.5f) * 2.0f);

        // LED body
        g.setColour (colour);
        g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

        // Specular dot
        g.setColour (juce::Colours::white.withAlpha (0.5f * brightnessVar));
        g.fillEllipse (cx - radius * 0.3f, cy - radius * 0.4f, radius * 0.5f, radius * 0.5f);
    }
    else
    {
        g.setColour (SynthColours::ledOff);
        g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

        g.setColour (SynthColours::faceplateEdge);
        g.drawEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 0.5f);
    }
}

void AnalogSynthLookAndFeel::fillWoodGrain (juce::Graphics& g, juce::Rectangle<float> area, bool vertical)
{
    g.setColour (SynthColours::woodDark);
    g.fillRect (area);

    int startY = static_cast<int> (area.getY());
    int endY   = static_cast<int> (area.getBottom());
    float w    = area.getWidth();

    for (int y = startY; y < endY; ++y)
    {
        float fy = static_cast<float> (y);
        float grain = std::sin (fy * 0.3f) * 0.5f
                    + std::sin (fy * 0.7f + 1.3f) * 0.3f
                    + std::sin (fy * 1.9f + 0.7f) * 0.2f;

        float blend = (grain + 1.0f) * 0.5f;
        auto lineColour = SynthColours::woodDark.interpolatedWith (SynthColours::woodLight, blend * 0.6f);

        g.setColour (lineColour);
        g.fillRect (area.getX(), fy, w, 1.0f);
    }

    // Wood knots — darker elliptical spots for natural character
    for (int k = 0; k < 2; ++k)
    {
        unsigned int kh = static_cast<unsigned int> (area.getX() * 31.0f + static_cast<float> (k) * 4919.0f);
        kh ^= 0xfeedface;
        kh *= 0x01000193;

        float knotX = area.getX() + area.getWidth() * 0.3f + (static_cast<float> (kh % 100) / 250.0f) * area.getWidth();
        float knotY = area.getY() + area.getHeight() * 0.25f + (static_cast<float> ((kh >> 8) % 1000) / 2000.0f) * area.getHeight();
        float knotW = 4.0f + static_cast<float> ((kh >> 16) % 30) / 10.0f;
        float knotH = knotW * (1.5f + static_cast<float> ((kh >> 20) % 10) / 10.0f);

        juce::ColourGradient knotGrad (
            SynthColours::woodGrain.withAlpha (0.35f), knotX, knotY,
            juce::Colours::transparentBlack, knotX, knotY + knotH * 0.5f, true);
        g.setGradientFill (knotGrad);
        g.fillEllipse (knotX - knotW * 0.5f, knotY - knotH * 0.5f, knotW, knotH);
    }

    // Edge darkening
    float edgeH = area.getHeight() * 0.1f;
    juce::ColourGradient topDarken (juce::Colours::black.withAlpha (0.15f), area.getX(), area.getY(),
                                    juce::Colours::transparentBlack, area.getX(), area.getY() + edgeH, false);
    g.setGradientFill (topDarken);
    g.fillRect (area.getX(), area.getY(), w, edgeH);

    juce::ColourGradient botDarken (juce::Colours::black.withAlpha (0.15f), area.getX(), area.getBottom(),
                                    juce::Colours::transparentBlack, area.getX(), area.getBottom() - edgeH, false);
    g.setGradientFill (botDarken);
    g.fillRect (area.getX(), area.getBottom() - edgeH, w, edgeH);

    (void) vertical;
}

void AnalogSynthLookAndFeel::drawLCDBackground (juce::Graphics& g, juce::Rectangle<float> area,
                                                  float bezelWidth)
{
    // Outer bevel / bezel
    g.setColour (SynthColours::faceplateEdge.darker (0.3f));
    g.drawLine (area.getX(), area.getY(), area.getRight(), area.getY(), bezelWidth * 0.5f);
    g.drawLine (area.getX(), area.getY(), area.getX(), area.getBottom(), bezelWidth * 0.5f);
    g.setColour (SynthColours::faceplateMid.brighter (0.1f));
    g.drawLine (area.getX(), area.getBottom(), area.getRight(), area.getBottom(), bezelWidth * 0.5f);
    g.drawLine (area.getRight(), area.getY(), area.getRight(), area.getBottom(), bezelWidth * 0.5f);

    auto inner = area.reduced (bezelWidth);

    // LCD background
    g.setColour (SynthColours::lcdBackground);
    g.fillRect (inner);

    // Scanline effect
    int startY = static_cast<int> (inner.getY());
    int endY   = static_cast<int> (inner.getBottom());
    g.setColour (juce::Colours::black.withAlpha (0.08f));
    for (int y = startY; y < endY; y += 2)
        g.fillRect (inner.getX(), static_cast<float> (y), inner.getWidth(), 1.0f);

    // Grid lines
    g.setColour (SynthColours::lcdGrid);
    float gridSpacing = 20.0f;
    for (float gx = inner.getX() + gridSpacing; gx < inner.getRight(); gx += gridSpacing)
        g.fillRect (gx, inner.getY(), 0.5f, inner.getHeight());
    for (float gy = inner.getY() + gridSpacing; gy < inner.getBottom(); gy += gridSpacing)
        g.fillRect (inner.getX(), gy, inner.getWidth(), 0.5f);

    // Inner shadow at edges
    float shadowDepth = 3.0f;
    {
        juce::ColourGradient topShadow (juce::Colours::black.withAlpha (0.3f), inner.getX(), inner.getY(),
                                         juce::Colours::transparentBlack, inner.getX(), inner.getY() + shadowDepth, false);
        g.setGradientFill (topShadow);
        g.fillRect (inner.getX(), inner.getY(), inner.getWidth(), shadowDepth);
    }
    {
        juce::ColourGradient leftShadow (juce::Colours::black.withAlpha (0.2f), inner.getX(), inner.getY(),
                                          juce::Colours::transparentBlack, inner.getX() + shadowDepth, inner.getY(), false);
        g.setGradientFill (leftShadow);
        g.fillRect (inner.getX(), inner.getY(), shadowDepth, inner.getHeight());
    }

    // Subtle glass reflection
    g.setColour (juce::Colours::white.withAlpha (0.03f));
    g.fillRect (inner.getX() + inner.getWidth() * 0.1f, inner.getY() + 1.0f,
                inner.getWidth() * 0.8f, 2.0f);
}

//==============================================================================
// drawRotarySlider — Photorealistic 3D Knob
//==============================================================================

void AnalogSynthLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                                float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                                juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
    float diameter = juce::jmin (bounds.getWidth(), bounds.getHeight()) - 4.0f;
    auto centre = bounds.getCentre();
    float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto accentColour = slider.findColour (juce::Slider::thumbColourId);

    // --- Layer 1: Shadow / Skirt ---
    float skirtSize = diameter * 0.58f;
    for (int i = 3; i >= 1; --i)
    {
        float expand = static_cast<float> (i) * 2.0f;
        float alpha = 0.06f * static_cast<float> (4 - i);
        g.setColour (juce::Colours::black.withAlpha (alpha));
        g.fillEllipse (centre.x - skirtSize * 0.5f - expand,
                       centre.y - skirtSize * 0.5f - expand + 1.0f,
                       skirtSize + expand * 2.0f, skirtSize + expand * 2.0f);
    }

    g.setColour (SynthColours::knobSkirt);
    g.fillEllipse (centre.x - skirtSize * 0.5f, centre.y - skirtSize * 0.5f,
                   skirtSize, skirtSize);

    // --- Layer 2: Value ring (tick marks around perimeter) ---
    float ringRadius = diameter * 0.46f;
    int numTicks = 31;
    for (int i = 0; i < numTicks; ++i)
    {
        float tickFrac = static_cast<float> (i) / static_cast<float> (numTicks - 1);
        float tickAngle = rotaryStartAngle + tickFrac * (rotaryEndAngle - rotaryStartAngle);
        float tickInner = ringRadius - 2.0f;
        float tickOuter = ringRadius + 1.0f;

        float cosA = std::cos (tickAngle);
        float sinA = std::sin (tickAngle);

        float tx1 = centre.x + tickInner * sinA;
        float ty1 = centre.y - tickInner * cosA;
        float tx2 = centre.x + tickOuter * sinA;
        float ty2 = centre.y - tickOuter * cosA;

        bool filled = tickFrac <= sliderPos;
        if (filled)
            g.setColour (accentColour.withAlpha (0.8f));
        else
            g.setColour (SynthColours::knobTrack.withAlpha (0.5f));

        g.drawLine (tx1, ty1, tx2, ty2, filled ? 1.5f : 1.0f);
    }

    // Glow behind filled portion
    {
        juce::Path glowArc;
        glowArc.addCentredArc (centre.x, centre.y, ringRadius, ringRadius,
                               0.0f, rotaryStartAngle, angle, true);
        g.setColour (accentColour.withAlpha (0.12f));
        g.strokePath (glowArc, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
    }

    // --- Layer 3: Knob body (3D convex cylinder) ---
    // Per-knob brightness micro-variation (manufacturing tolerance simulation)
    unsigned int knobHash = static_cast<unsigned int> (centre.x * 97.0f + centre.y * 53.0f);
    knobHash ^= 0x5bd1e995;
    knobHash *= 0x01000193;
    float brightnessVariation = (static_cast<float> (knobHash % 1000) / 500.0f - 1.0f) * 0.02f;

    auto knobLight = brightnessVariation > 0.0f
        ? SynthColours::knobBodyLight.brighter (brightnessVariation)
        : SynthColours::knobBodyLight.darker (-brightnessVariation);
    auto knobDark = brightnessVariation > 0.0f
        ? SynthColours::knobBodyDark.brighter (brightnessVariation * 0.5f)
        : SynthColours::knobBodyDark.darker (-brightnessVariation * 0.5f);

    float knobSize = diameter * 0.55f;
    auto knobBounds = juce::Rectangle<float> (centre.x - knobSize * 0.5f,
                                                centre.y - knobSize * 0.5f,
                                                knobSize, knobSize);

    float lightOffsetX = knobSize * -0.15f;
    float lightOffsetY = knobSize * -0.15f;
    juce::ColourGradient knobGrad (knobLight,
                                    centre.x + lightOffsetX, centre.y + lightOffsetY,
                                    knobDark,
                                    centre.x + knobSize * 0.4f, centre.y + knobSize * 0.4f,
                                    true);
    g.setGradientFill (knobGrad);
    g.fillEllipse (knobBounds);

    // --- Layer 4: Metallic edge ring ---
    juce::ColourGradient edgeGrad (juce::Colour (0xff666666), centre.x, knobBounds.getY(),
                                    juce::Colour (0xff111111), centre.x, knobBounds.getBottom(),
                                    false);
    g.setGradientFill (edgeGrad);
    g.drawEllipse (knobBounds, 1.5f);

    // --- Layer 5: Center cap ---
    float capSize = knobSize * 0.3f;
    auto capBounds = juce::Rectangle<float> (centre.x - capSize * 0.5f,
                                               centre.y - capSize * 0.5f,
                                               capSize, capSize);
    juce::ColourGradient capGrad (SynthColours::knobBodyLight.darker (0.1f),
                                   centre.x + lightOffsetX * 0.5f, centre.y + lightOffsetY * 0.5f,
                                   SynthColours::knobBodyDark.brighter (0.05f),
                                   centre.x + capSize * 0.3f, centre.y + capSize * 0.3f,
                                   true);
    g.setGradientFill (capGrad);
    g.fillEllipse (capBounds);

    g.setColour (SynthColours::knobEdge);
    g.drawEllipse (capBounds, 0.5f);

    // --- Layer 6: Indicator line ---
    float pointerLength = knobSize * 0.42f;

    // Glow pass
    {
        juce::Path glowPointer;
        glowPointer.addRectangle (-1.5f, -pointerLength, 3.0f, pointerLength * 0.6f);
        glowPointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
        g.setColour (accentColour.withAlpha (0.25f));
        g.fillPath (glowPointer);
    }

    // Main pointer (tapered wedge)
    {
        juce::Path pointer;
        pointer.startNewSubPath (-0.5f, -pointerLength);
        pointer.lineTo (0.5f, -pointerLength);
        pointer.lineTo (1.2f, -pointerLength + pointerLength * 0.4f);
        pointer.lineTo (-1.2f, -pointerLength + pointerLength * 0.4f);
        pointer.closeSubPath();
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
        g.setColour (SynthColours::knobPointer);
        g.fillPath (pointer);
    }

    // --- Layer 7: Multi-layer specular highlights ---

    // Diffuse specular — large, soft ambient reflection
    {
        float diffSize = knobSize * 0.45f;
        juce::ColourGradient diffSpec (
            juce::Colours::white.withAlpha (0.09f),
            centre.x - knobSize * 0.2f, centre.y - knobSize * 0.25f,
            juce::Colours::transparentBlack,
            centre.x - knobSize * 0.2f + diffSize, centre.y - knobSize * 0.25f + diffSize * 0.7f,
            true);
        g.setGradientFill (diffSpec);
        g.fillEllipse (centre.x - knobSize * 0.32f, centre.y - knobSize * 0.35f,
                       diffSize, diffSize * 0.65f);
    }

    // Focused specular — small, bright point light reflection
    {
        float focusSize = knobSize * 0.15f;
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.fillEllipse (centre.x - knobSize * 0.18f, centre.y - knobSize * 0.22f,
                       focusSize, focusSize * 0.5f);
    }

    // Rim light — subtle arc at bottom edge from surface reflection
    {
        float rimStartAngle = juce::MathConstants<float>::pi * 0.6f;
        float rimEndAngle = juce::MathConstants<float>::pi * 1.4f;
        juce::Path rimArc;
        rimArc.addCentredArc (centre.x, centre.y, knobSize * 0.48f, knobSize * 0.48f,
                               0.0f, rimStartAngle, rimEndAngle, true);
        g.setColour (juce::Colours::white.withAlpha (0.04f));
        g.strokePath (rimArc, juce::PathStrokeType (1.5f));
    }
}

//==============================================================================
// drawLinearSlider — Realistic vertical slider / wheel
//==============================================================================

void AnalogSynthLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                                float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                                                juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style == juce::Slider::LinearVertical)
    {
        auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                               static_cast<float> (width), static_cast<float> (height));

        float trackW = 8.0f;
        auto trackBounds = bounds.withSizeKeepingCentre (trackW, bounds.getHeight() - 4.0f);

        // Recessed channel
        g.setColour (SynthColours::recessDark);
        g.fillRoundedRectangle (trackBounds, trackW * 0.5f);

        // Inner shadow at top
        juce::ColourGradient trackShadow (juce::Colours::black.withAlpha (0.3f),
                                           trackBounds.getCentreX(), trackBounds.getY(),
                                           juce::Colours::transparentBlack,
                                           trackBounds.getCentreX(), trackBounds.getY() + 4.0f, false);
        g.setGradientFill (trackShadow);
        g.fillRoundedRectangle (trackBounds.getX(), trackBounds.getY(), trackW, 6.0f, trackW * 0.5f);

        // Bottom highlight
        g.setColour (SynthColours::recessHighlight);
        g.fillRect (trackBounds.getCentreX() - 2.0f, trackBounds.getBottom() - 1.0f, 4.0f, 1.0f);

        // Filled portion
        float fillTop = sliderPos;
        float fillBottom = trackBounds.getBottom();
        if (fillBottom > fillTop)
        {
            auto fillBounds = juce::Rectangle<float> (trackBounds.getX(), fillTop,
                                                       trackW, fillBottom - fillTop);
            auto accentColour = slider.findColour (juce::Slider::thumbColourId);
            g.setColour (accentColour.withAlpha (0.5f));
            g.fillRoundedRectangle (fillBounds, trackW * 0.5f);
        }

        // Thumb: horizontal capsule with grip lines
        auto accentColour = slider.findColour (juce::Slider::thumbColourId);
        float thumbW = 18.0f;
        float thumbH = 10.0f;
        auto thumbBounds = juce::Rectangle<float> (bounds.getCentreX() - thumbW * 0.5f,
                                                     sliderPos - thumbH * 0.5f,
                                                     thumbW, thumbH);

        // Shadow
        g.setColour (juce::Colours::black.withAlpha (0.3f));
        g.fillRoundedRectangle (thumbBounds.translated (0, 1.0f), 3.0f);

        // Body gradient
        juce::ColourGradient thumbGrad (juce::Colour (0xff555555), thumbBounds.getCentreX(), thumbBounds.getY(),
                                         juce::Colour (0xff2a2a2a), thumbBounds.getCentreX(), thumbBounds.getBottom(), false);
        g.setGradientFill (thumbGrad);
        g.fillRoundedRectangle (thumbBounds, 3.0f);

        // Top highlight
        g.setColour (juce::Colour (0xff666666));
        g.fillRect (thumbBounds.getX() + 3.0f, thumbBounds.getY(), thumbBounds.getWidth() - 6.0f, 1.0f);

        // Grip lines
        g.setColour (juce::Colour (0xff3a3a3a));
        for (int i = -1; i <= 1; ++i)
        {
            float ly = thumbBounds.getCentreY() + static_cast<float> (i) * 2.5f;
            g.fillRect (thumbBounds.getX() + 4.0f, ly, thumbBounds.getWidth() - 8.0f, 0.5f);
        }

        // Accent indicator dot
        g.setColour (accentColour);
        g.fillEllipse (thumbBounds.getCentreX() - 1.5f, thumbBounds.getCentreY() - 1.5f, 3.0f, 3.0f);
    }
    else
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, 0.0f, 0.0f, style, slider);
    }
}

//==============================================================================
// drawButtonBackground — Physical button with depth
//==============================================================================

void AnalogSynthLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                     const juce::Colour& backgroundColour,
                                                     bool isHighlighted, bool isDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    float cornerRadius = 3.0f;

    // Outer shadow
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (bounds.translated (0, 1.0f), cornerRadius);

    auto drawBounds = isDown ? bounds.translated (0, 0.5f) : bounds;

    auto baseColour = backgroundColour;
    if (isHighlighted && !isDown)
        baseColour = baseColour.brighter (0.06f);

    bool isToggled = button.getToggleState();
    if (isToggled)
    {
        auto accentColour = button.findColour (juce::TextButton::buttonOnColourId);
        baseColour = accentColour.withAlpha (1.0f);
    }

    juce::ColourGradient bodyGrad (isDown ? baseColour.darker (0.1f) : baseColour.brighter (0.08f),
                                    drawBounds.getCentreX(), drawBounds.getY(),
                                    isDown ? baseColour.brighter (0.04f) : baseColour.darker (0.06f),
                                    drawBounds.getCentreX(), drawBounds.getBottom(),
                                    false);
    g.setGradientFill (bodyGrad);
    g.fillRoundedRectangle (drawBounds, cornerRadius);

    // Top highlight
    if (!isDown)
    {
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.fillRect (drawBounds.getX() + cornerRadius, drawBounds.getY(),
                    drawBounds.getWidth() - cornerRadius * 2.0f, 1.0f);
    }

    // Bottom shadow edge
    g.setColour (juce::Colours::black.withAlpha (0.15f));
    g.fillRect (drawBounds.getX() + cornerRadius, drawBounds.getBottom() - 1.0f,
                drawBounds.getWidth() - cornerRadius * 2.0f, 1.0f);

    // Border
    g.setColour (SynthColours::faceplateEdge);
    g.drawRoundedRectangle (drawBounds, cornerRadius, 0.5f);
}

void AnalogSynthLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                               bool, bool isDown)
{
    auto textBounds = button.getLocalBounds();
    if (isDown) textBounds.translate (0, 1);

    bool isToggled = button.getToggleState();

    // LED indicator dot for toggled buttons
    if (isToggled)
    {
        float ledX = static_cast<float> (textBounds.getX()) + 6.0f;
        float ledY = static_cast<float> (textBounds.getCentreY());
        drawLED (g, ledX, ledY, 2.5f, true, SynthColours::ledOn);
        textBounds.removeFromLeft (14);
    }

    auto colour = button.findColour (button.getToggleState()
                                         ? juce::TextButton::textColourOnId
                                         : juce::TextButton::textColourOffId);
    g.setColour (colour);
    g.setFont (juce::Font (11.0f).boldened());
    g.drawText (button.getButtonText(), textBounds, juce::Justification::centred, false);
}

//==============================================================================
// drawToggleButton — Physical toggle switch with LED
//==============================================================================

void AnalogSynthLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                                 bool isHighlighted, bool isDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    bool isOn = button.getToggleState();

    // Switch recess
    float switchW = 12.0f;
    float switchH = 20.0f;
    float switchX = bounds.getX() + 4.0f;
    float switchY = bounds.getCentreY() - switchH * 0.5f;
    auto switchBounds = juce::Rectangle<float> (switchX, switchY, switchW, switchH);

    g.setColour (SynthColours::recessDark.darker (0.2f));
    g.fillRoundedRectangle (switchBounds, 2.0f);

    // Inner shadow
    juce::ColourGradient recessShadow (juce::Colours::black.withAlpha (0.3f),
                                        switchBounds.getCentreX(), switchBounds.getY(),
                                        juce::Colours::transparentBlack,
                                        switchBounds.getCentreX(), switchBounds.getY() + 3.0f, false);
    g.setGradientFill (recessShadow);
    g.fillRect (switchBounds.getX(), switchBounds.getY(), switchW, 4.0f);

    // Toggle lever
    float leverH = switchH * 0.45f;
    float leverY = isOn ? switchY + 1.0f : switchY + switchH - leverH - 1.0f;
    auto leverBounds = juce::Rectangle<float> (switchX + 1.0f, leverY, switchW - 2.0f, leverH);

    auto leverColour = juce::Colour (0xff888888);
    if (isDown) leverColour = leverColour.darker (0.1f);
    else if (isHighlighted) leverColour = leverColour.brighter (0.05f);

    juce::ColourGradient leverGrad (leverColour.brighter (0.15f), leverBounds.getCentreX(), leverBounds.getY(),
                                     leverColour.darker (0.1f), leverBounds.getCentreX(), leverBounds.getBottom(), false);
    g.setGradientFill (leverGrad);
    g.fillRoundedRectangle (leverBounds, 1.5f);

    // Lever highlight
    g.setColour (juce::Colours::white.withAlpha (0.1f));
    g.fillRect (leverBounds.getX() + 2.0f, leverBounds.getY(), leverBounds.getWidth() - 4.0f, 1.0f);

    // Lever grip line
    g.setColour (juce::Colours::black.withAlpha (0.15f));
    g.fillRect (leverBounds.getX() + 3.0f, leverBounds.getCentreY(), leverBounds.getWidth() - 6.0f, 0.5f);

    // LED indicator
    float ledX = switchX + switchW + 6.0f;
    float ledY = bounds.getCentreY();
    drawLED (g, ledX, ledY, 2.5f, isOn, SynthColours::ledOn);

    // Label text
    auto textBounds = bounds.withTrimmedLeft (switchW + 18.0f);
    g.setColour (SynthColours::silkscreen);
    g.setFont (juce::Font (11.0f));
    g.drawText (button.getButtonText(), textBounds, juce::Justification::centredLeft, false);
}

//==============================================================================
// drawComboBox — Metal dropdown
//==============================================================================

void AnalogSynthLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height,
                                             bool isButtonDown, int, int, int, int,
                                             juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float> (0, 0, static_cast<float> (width), static_cast<float> (height));
    float cornerRadius = 3.0f;

    auto bgColour = box.findColour (juce::ComboBox::backgroundColourId);
    if (isButtonDown) bgColour = bgColour.brighter (0.06f);

    juce::ColourGradient bodyGrad (bgColour.brighter (0.04f), bounds.getCentreX(), bounds.getY(),
                                    bgColour.darker (0.04f), bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill (bodyGrad);
    g.fillRoundedRectangle (bounds, cornerRadius);

    // Inset bevel
    g.setColour (SynthColours::faceplateEdge.darker (0.1f));
    g.drawLine (bounds.getX(), bounds.getY(), bounds.getRight(), bounds.getY(), 0.5f);
    g.drawLine (bounds.getX(), bounds.getY(), bounds.getX(), bounds.getBottom(), 0.5f);

    g.setColour (SynthColours::faceplateMid.withAlpha (0.3f));
    g.drawLine (bounds.getX(), bounds.getBottom(), bounds.getRight(), bounds.getBottom(), 0.5f);
    g.drawLine (bounds.getRight(), bounds.getY(), bounds.getRight(), bounds.getBottom(), 0.5f);

    // Engraved chevron
    float arrowSize = 5.0f;
    float arrowX = static_cast<float> (width) - 14.0f;
    float arrowY = static_cast<float> (height) * 0.5f;

    juce::Path chevron;
    chevron.startNewSubPath (arrowX, arrowY - arrowSize * 0.35f);
    chevron.lineTo (arrowX + arrowSize * 0.5f, arrowY + arrowSize * 0.35f);
    chevron.lineTo (arrowX + arrowSize, arrowY - arrowSize * 0.35f);

    g.setColour (SynthColours::silkscreenDim);
    g.strokePath (chevron, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
}

//==============================================================================
// drawLabel — Silkscreen text
//==============================================================================

void AnalogSynthLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.fillAll (label.findColour (juce::Label::backgroundColourId));

    auto textArea = label.getLocalBounds().toFloat();
    auto textColour = label.findColour (juce::Label::textColourId);
    g.setFont (label.getFont());

    // Subtle emboss
    g.setColour (juce::Colours::black.withAlpha (0.15f));
    g.drawText (label.getText(), textArea.translated (0, -0.5f), label.getJustificationType(), true);

    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.drawText (label.getText(), textArea.translated (0, 0.5f), label.getJustificationType(), true);

    g.setColour (textColour);
    g.drawText (label.getText(), textArea, label.getJustificationType(), true);
}

//==============================================================================
// Popup menu
//==============================================================================

void AnalogSynthLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    auto bounds = juce::Rectangle<float> (0, 0, static_cast<float> (width), static_cast<float> (height));

    // Drop shadow
    g.setColour (juce::Colours::black.withAlpha (0.3f));
    g.fillRoundedRectangle (bounds.translated (2.0f, 2.0f), 4.0f);

    g.setColour (SynthColours::faceplateDark);
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.fillRect (bounds.getX() + 4.0f, bounds.getY(), bounds.getWidth() - 8.0f, 1.0f);

    g.setColour (SynthColours::faceplateEdge);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 0.5f);
}

void AnalogSynthLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                                  bool isSeparator, bool isActive, bool isHighlighted,
                                                  bool isTicked, bool,
                                                  const juce::String& text, const juce::String&,
                                                  const juce::Drawable*, const juce::Colour*)
{
    if (isSeparator)
    {
        g.setColour (SynthColours::faceplateEdge);
        g.fillRect (area.reduced (5, 0).withHeight (1).withY (area.getCentreY()));
        return;
    }

    if (isHighlighted && isActive)
    {
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.fillRoundedRectangle (area.toFloat().reduced (2, 1), 3.0f);
    }

    g.setColour (isActive ? SynthColours::silkscreen : SynthColours::silkscreenDim);
    g.setFont (juce::Font (12.0f));
    g.drawText (text, area.reduced (10, 0), juce::Justification::centredLeft);

    if (isTicked)
    {
        float ledX = static_cast<float> (area.getRight()) - 12.0f;
        float ledY = static_cast<float> (area.getCentreY());
        drawLED (g, ledX, ledY, 2.5f, true, SynthColours::ledOn);
    }
}

//==============================================================================
// Glass panel (legacy wrapper)
//==============================================================================

void AnalogSynthLookAndFeel::drawSectionTitle (juce::Graphics& g, const juce::String& text,
                                                 juce::Rectangle<float> area,
                                                 juce::Colour sectionColour, float fontSize)
{
    g.setFont (juce::Font (fontSize).boldened());

    // Subtle glow behind text (section color at low alpha)
    g.setColour (sectionColour.withAlpha (0.06f));
    g.drawText (text, area.expanded (2, 1), juce::Justification::centredLeft, false);

    // Dark inset shadow
    g.setColour (juce::Colours::black.withAlpha (0.3f));
    g.drawText (text, area.translated (0, -0.5f), juce::Justification::centredLeft, false);

    // Light catch edge
    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.drawText (text, area.translated (0, 1.0f), juce::Justification::centredLeft, false);

    // Main title in section color
    g.setColour (sectionColour);
    g.drawText (text, area, juce::Justification::centredLeft, false);
}

void AnalogSynthLookAndFeel::drawGlassPanel (juce::Graphics& g, juce::Rectangle<float> area,
                                               float cornerRadius)
{
    drawRecessedPanel (g, area, cornerRadius);
}

//==============================================================================
// Tab bar
//==============================================================================

void AnalogSynthLookAndFeel::drawTabbedButtonBarBackground (juce::TabbedButtonBar&, juce::Graphics& g)
{
    auto bounds = g.getClipBounds().toFloat();
    fillBrushedMetal (g, bounds, SynthColours::faceplateDark);

    // Bottom groove
    g.setColour (SynthColours::faceplateEdge.darker (0.2f));
    g.fillRect (bounds.getX(), bounds.getBottom() - 2.0f, bounds.getWidth(), 1.0f);
    g.setColour (SynthColours::faceplateMid.brighter (0.05f));
    g.fillRect (bounds.getX(), bounds.getBottom() - 1.0f, bounds.getWidth(), 1.0f);
}

void AnalogSynthLookAndFeel::drawTabButton (juce::TabBarButton& button, juce::Graphics& g,
                                              bool isMouseOver, bool /*isMouseDown*/)
{
    auto area = button.getActiveArea().toFloat();
    bool isFrontTab = button.isFrontTab();

    if (isFrontTab)
    {
        juce::ColourGradient tabGrad (SynthColours::faceplateMid, area.getCentreX(), area.getY(),
                                       SynthColours::faceplateDark, area.getCentreX(), area.getBottom(), false);
        g.setGradientFill (tabGrad);
        g.fillRoundedRectangle (area.reduced (2, 2), 3.0f);

        // Top highlight
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.fillRect (area.getX() + 6.0f, area.getY() + 2.0f, area.getWidth() - 12.0f, 1.0f);

        // LED underline
        g.setColour (SynthColours::accent.withAlpha (0.15f));
        g.fillRect (area.getX() + 8.0f, area.getBottom() - 4.0f, area.getWidth() - 16.0f, 3.0f);
        g.setColour (SynthColours::accent);
        g.fillRect (area.getX() + 8.0f, area.getBottom() - 3.0f, area.getWidth() - 16.0f, 2.0f);
    }
    else if (isMouseOver)
    {
        g.setColour (juce::Colours::white.withAlpha (0.03f));
        g.fillRoundedRectangle (area.reduced (2, 2), 3.0f);
    }

    g.setColour (isFrontTab ? SynthColours::silkscreen : SynthColours::silkscreenDim);
    g.setFont (juce::Font (11.0f).boldened());
    g.drawText (button.getButtonText(), button.getActiveArea(), juce::Justification::centred, false);
}
