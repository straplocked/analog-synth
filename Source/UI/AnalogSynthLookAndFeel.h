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
#include <BinaryData.h>

namespace SynthColours
{
    // Faceplate metal
    const juce::Colour faceplateLight     { 0xff8a8a88 };
    const juce::Colour faceplateMid       { 0xff5a5a58 };
    const juce::Colour faceplateDark      { 0xff3a3a38 };
    const juce::Colour faceplateEdge      { 0xff2a2a28 };

    // Recessed panel areas
    const juce::Colour recessDark         { 0xff252523 };
    const juce::Colour recessMid          { 0xff2f2f2d };
    const juce::Colour recessHighlight    { 0x18ffffff };

    // Knob body
    const juce::Colour knobBodyLight      { 0xff555555 };
    const juce::Colour knobBodyDark       { 0xff1a1a1a };
    const juce::Colour knobEdge           { 0xff0f0f0f };
    const juce::Colour knobPointer        { 0xffe0e0e0 };
    const juce::Colour knobSkirt          { 0xff111111 };

    // LED / Backlit indicators
    const juce::Colour ledOn              { 0xff00ff88 };
    const juce::Colour ledOff             { 0xff1a2a1a };
    const juce::Colour ledRed             { 0xffff3333 };
    const juce::Colour ledAmber           { 0xffffaa22 };

    // Silkscreen text
    const juce::Colour silkscreen         { 0xffd8d8d5 };
    const juce::Colour silkscreenDim      { 0xff888885 };

    // Wood side cheeks
    const juce::Colour woodLight          { 0xff8b5e3c };
    const juce::Colour woodDark           { 0xff4a2e1a };
    const juce::Colour woodGrain          { 0xff3a2010 };

    // Display / LCD
    const juce::Colour lcdBackground      { 0xff0a1a0a };
    const juce::Colour lcdGrid            { 0xff0f2f0f };
    const juce::Colour lcdWaveform        { 0xff33ff99 };
    const juce::Colour lcdText            { 0xff33ff99 };

    // Screw heads
    const juce::Colour screwBody          { 0xff444444 };
    const juce::Colour screwSlot          { 0xff222222 };
    const juce::Colour screwHighlight     { 0xff666666 };

    // Section accents
    const juce::Colour sectionOsc         { 0xff4488ff };
    const juce::Colour sectionFilter      { 0xffff7744 };
    const juce::Colour sectionEnv         { 0xff44cc66 };
    const juce::Colour sectionLfo         { 0xffaa66ff };
    const juce::Colour sectionFx          { 0xffff4466 };

    // Backward-compatible aliases
    const juce::Colour background         { 0xff2a2a28 };
    const juce::Colour panelBg            { 0xff333330 };
    const juce::Colour panelBorder        { 0xff444440 };
    const juce::Colour accent             { 0xff4488ff };
    const juce::Colour accentDim          { 0xff223366 };
    const juce::Colour knobTrack          { 0xff222222 };
    const juce::Colour knobFill           { 0xff4488ff };
    const juce::Colour textPrimary        { 0xffd8d8d5 };
    const juce::Colour textSecondary      { 0xff888885 };
    const juce::Colour textValue          { 0xff4488ff };
    const juce::Colour glassBg            { 0x18ffffff };
    const juce::Colour glassBorder        { 0x30ffffff };

    // Envelope colours
    const juce::Colour envAttack          { 0xff33ff99 };
    const juce::Colour envDecay           { 0xff33cc77 };
    const juce::Colour envSustain         { 0xff229955 };
    const juce::Colour envRelease         { 0xff117744 };
}

class AnalogSynthLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AnalogSynthLookAndFeel();
    ~AnalogSynthLookAndFeel() override = default;

    juce::Typeface::Ptr getTypefaceForFont (const juce::Font& font) override;

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics&, juce::TextButton&,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;

    void drawLabel (juce::Graphics&, juce::Label&) override;

    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;

    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu,
                            const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;

    // Legacy glass panel (kept for compat, now wraps drawRecessedPanel)
    static void drawGlassPanel (juce::Graphics& g, juce::Rectangle<float> area,
                                float cornerRadius = 6.0f);

    void drawTabbedButtonBarBackground (juce::TabbedButtonBar&, juce::Graphics&) override;

    void drawTabButton (juce::TabBarButton&, juce::Graphics&, bool isMouseOver, bool isMouseDown) override;

    // --- Photorealistic helpers ---

    static void drawRecessedPanel (juce::Graphics& g, juce::Rectangle<float> area,
                                   float cornerRadius = 3.0f, float depth = 2.0f);

    static void fillBrushedMetal (juce::Graphics& g, juce::Rectangle<float> area,
                                  juce::Colour baseColour, float grainIntensity = 0.03f);

    static void drawScrewHead (juce::Graphics& g, float cx, float cy, float radius = 4.0f,
                               float rotationOverride = -999.0f);

    static void drawSectionTitle (juce::Graphics& g, const juce::String& text,
                                   juce::Rectangle<float> area, juce::Colour sectionColour,
                                   float fontSize = 11.0f);

    static void drawSilkscreenLabel (juce::Graphics& g, const juce::String& text,
                                      juce::Rectangle<float> area,
                                      juce::Justification just = juce::Justification::centred,
                                      float fontSize = 10.0f, bool embossed = true);

    static void drawLED (juce::Graphics& g, float cx, float cy, float radius,
                         bool isOn, juce::Colour colour = SynthColours::ledOn);

    static void fillWoodGrain (juce::Graphics& g, juce::Rectangle<float> area, bool vertical = true);

    static void drawLCDBackground (juce::Graphics& g, juce::Rectangle<float> area,
                                    float bezelWidth = 3.0f);

private:
    juce::Typeface::Ptr interRegular;
    juce::Typeface::Ptr interBold;
};
