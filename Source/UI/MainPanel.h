#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "HeaderPanel.h"
#include "OscillatorPanel.h"
#include "WaveformDisplay.h"
#include "FilterPanel.h"
#include "MixerPanel.h"
#include "PolyModPanel.h"
#include "TabbedBottomPanel.h"
#include "PitchModWheelPanel.h"
#include "../ParameterIDs.h"

class MainPanel : public juce::Component,
                  private juce::FocusChangeListener
{
public:
    MainPanel()
    {
        addAndMakeVisible (headerPanel);
        addAndMakeVisible (oscAPanel);
        addAndMakeVisible (waveformDisplay);
        addAndMakeVisible (oscBPanel);
        addAndMakeVisible (oscCPanel);
        addAndMakeVisible (filterPanel);
        addAndMakeVisible (mixerPanel);
        addAndMakeVisible (polyModPanel);
        addAndMakeVisible (tabbedPanel);
        addAndMakeVisible (wheelPanel);

        // Octave buttons
        octaveDownBtn.setButtonText ("<");
        octaveDownBtn.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        octaveDownBtn.onClick = [this] { shiftOctave (-1); };
        addAndMakeVisible (octaveDownBtn);

        octaveUpBtn.setButtonText (">");
        octaveUpBtn.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        octaveUpBtn.onClick = [this] { shiftOctave (1); };
        addAndMakeVisible (octaveUpBtn);

        octaveLabel.setText ("C4", juce::dontSendNotification);
        octaveLabel.setFont (juce::Font (10.0f));
        octaveLabel.setColour (juce::Label::textColourId, SynthColours::textSecondary);
        octaveLabel.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (octaveLabel);

        // Wire wheel callbacks
        wheelPanel.onPitchBendChanged = [this] (float val) { pitchBendValue.store (val); };
        wheelPanel.onModWheelChanged  = [this] (float val) { modWheelValue.store (val); };

        juce::Desktop::getInstance().addFocusChangeListener (this);
    }

    ~MainPanel() override
    {
        juce::Desktop::getInstance().removeFocusChangeListener (this);
    }

    void setKeyboardState (juce::MidiKeyboardState* state)
    {
        if (state != nullptr)
        {
            keyboard = std::make_unique<juce::MidiKeyboardComponent> (
                *state, juce::MidiKeyboardComponent::horizontalKeyboard);
            keyboard->setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xff2a2a2a));
            keyboard->setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff111111));
            keyboard->setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff333333));
            keyboard->setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, SynthColours::accent.withAlpha (0.6f));
            keyboard->setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, SynthColours::accent.withAlpha (0.2f));
            keyboard->setVelocity (1.0f, false);
            keyboard->setKeyWidth (18.0f);
            keyboard->setAvailableRange (36, 96);  // C2 to C7
            keyboard->setKeyPressBaseOctave (currentOctave);
            addAndMakeVisible (*keyboard);
            keyboard->grabKeyboardFocus();
            resized();
        }
    }

    void setPresetManager (PresetManager* mgr) { headerPanel.setPresetManager (mgr); }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        headerPanel.attach (apvts);

        oscAPanel.attach (apvts,
                          ParameterIDs::osc1Waveform,
                          ParameterIDs::osc1Octave,
                          ParameterIDs::osc1Semitone,
                          ParameterIDs::osc1Fine,
                          ParameterIDs::osc1PulseWidth,
                          ParameterIDs::osc1Level);

        oscBPanel.attach (apvts,
                          ParameterIDs::osc2Waveform,
                          ParameterIDs::osc2Octave,
                          ParameterIDs::osc2Semitone,
                          ParameterIDs::osc2Fine,
                          ParameterIDs::osc2PulseWidth,
                          ParameterIDs::osc2Level,
                          ParameterIDs::osc2Sync);

        oscCPanel.attach (apvts,
                          ParameterIDs::osc3Waveform,
                          ParameterIDs::osc3Octave,
                          ParameterIDs::osc3Semitone,
                          ParameterIDs::osc3Fine,
                          ParameterIDs::osc3PulseWidth,
                          ParameterIDs::osc3Level);

        filterPanel.attach (apvts);
        mixerPanel.attach (apvts);
        polyModPanel.attach (apvts);
        tabbedPanel.attach (apvts);
    }

    WaveformDisplay& getWaveformDisplay() { return waveformDisplay; }

    float getPitchBend() const { return pitchBendValue.load(); }
    float getModWheel() const  { return modWheelValue.load(); }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Brushed metal faceplate
        AnalogSynthLookAndFeel::fillBrushedMetal (g, bounds, SynthColours::faceplateMid);

        // Wood end-cheeks
        auto leftWood = juce::Rectangle<float> (bounds.getX(), bounds.getY(), woodCheekWidth, bounds.getHeight());
        auto rightWood = juce::Rectangle<float> (bounds.getRight() - woodCheekWidth, bounds.getY(), woodCheekWidth, bounds.getHeight());
        AnalogSynthLookAndFeel::fillWoodGrain (g, leftWood);
        AnalogSynthLookAndFeel::fillWoodGrain (g, rightWood);

        // Dark border between wood and metal
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRect (leftWood.getRight() - 1.0f, leftWood.getY(), 1.0f, leftWood.getHeight());
        g.fillRect (rightWood.getX(), rightWood.getY(), 1.0f, rightWood.getHeight());

        // Corner screws
        float inset = 20.0f;
        float screwY1 = 30.0f;
        float screwY2 = static_cast<float> (getHeight()) - 30.0f;
        AnalogSynthLookAndFeel::drawScrewHead (g, inset, screwY1, 3.5f);
        AnalogSynthLookAndFeel::drawScrewHead (g, static_cast<float> (getWidth()) - inset, screwY1, 3.5f);
        AnalogSynthLookAndFeel::drawScrewHead (g, inset, screwY2, 3.5f);
        AnalogSynthLookAndFeel::drawScrewHead (g, static_cast<float> (getWidth()) - inset, screwY2, 3.5f);

        // Subtle vignette
        auto fullBounds = getLocalBounds().toFloat();
        juce::ColourGradient vignette (juce::Colours::transparentBlack, fullBounds.getCentreX(), fullBounds.getCentreY(),
                                        juce::Colours::black.withAlpha (0.06f), 0, 0, true);
        g.setGradientFill (vignette);
        g.fillRect (fullBounds);
    }

    void paintOverChildren (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Top edge highlight
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.fillRect (bounds.getX(), bounds.getY(), bounds.getWidth(), 1.0f);

        // Bottom shadow gradient
        juce::ColourGradient bottomShadow (juce::Colours::black.withAlpha (0.15f), 0, bounds.getBottom(),
                                            juce::Colours::transparentBlack, 0, bounds.getBottom() - 10.0f, false);
        g.setGradientFill (bottomShadow);
        g.fillRect (bounds.getX(), bounds.getBottom() - 10.0f, bounds.getWidth(), 10.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // Inset for wood cheeks
        bounds.removeFromLeft (static_cast<int> (woodCheekWidth));
        bounds.removeFromRight (static_cast<int> (woodCheekWidth));

        // Header: 50px
        headerPanel.setBounds (bounds.removeFromTop (50));

        // Bottom bar: 60px — wheels | octave | keyboard
        auto bottomBar = bounds.removeFromBottom (60);

        wheelPanel.setBounds (bottomBar.removeFromLeft (70));

        auto octaveStrip = bottomBar.removeFromLeft (30);
        int btnH = 18;
        octaveDownBtn.setBounds (octaveStrip.removeFromTop (btnH).reduced (2, 1));
        octaveUpBtn.setBounds (octaveStrip.removeFromBottom (btnH).reduced (2, 1));
        octaveLabel.setBounds (octaveStrip);

        if (keyboard != nullptr)
            keyboard->setBounds (bottomBar);

        // Bottom tabbed panel: 260px
        tabbedPanel.setBounds (bounds.removeFromBottom (260));

        // Separator line
        bounds.removeFromBottom (2);

        // Middle area splits into two rows
        // Row 1 (top): OSC A | OSC B | OSC C | Waveform — 200px
        auto topRow = bounds.removeFromTop (200);
        int waveW = 220;
        int oscW = (topRow.getWidth() - waveW) / 3;
        oscAPanel.setBounds (topRow.removeFromLeft (oscW).reduced (4));
        oscBPanel.setBounds (topRow.removeFromLeft (oscW).reduced (4));
        oscCPanel.setBounds (topRow.removeFromLeft (oscW).reduced (4));
        waveformDisplay.setBounds (topRow.reduced (4));

        // Row 2 (bottom): Filter | Mixer | PolyMod
        auto midRow = bounds.reduced (0, 2);
        int filterW = midRow.getWidth() * 40 / 100;
        int mixerW  = midRow.getWidth() * 22 / 100;

        filterPanel.setBounds (midRow.removeFromLeft (filterW).reduced (4));
        mixerPanel.setBounds (midRow.removeFromLeft (mixerW).reduced (4));
        polyModPanel.setBounds (midRow.reduced (4));
    }

    bool keyPressed (const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress ('z') || key == juce::KeyPress ('Z'))
        {
            shiftOctave (-1);
            return true;
        }
        if (key == juce::KeyPress ('x') || key == juce::KeyPress ('X'))
        {
            shiftOctave (1);
            return true;
        }
        return false;
    }

    void visibilityChanged() override
    {
        if (isVisible() && keyboard != nullptr)
            keyboard->grabKeyboardFocus();
    }

private:
    void globalFocusChanged (juce::Component* focusedComponent) override
    {
        // Whenever focus moves to something that isn't the keyboard,
        // grab it back so computer keyboard always triggers notes
        if (keyboard != nullptr && focusedComponent != keyboard.get())
        {
            // Disable focus on the newly-focused component (and its subtree)
            // so it won't steal focus again, then return focus to keyboard
            if (focusedComponent != nullptr)
                focusedComponent->setWantsKeyboardFocus (false);

            keyboard->grabKeyboardFocus();
        }
    }

    void shiftOctave (int delta)
    {
        currentOctave = juce::jlimit (1, 7, currentOctave + delta);
        if (keyboard != nullptr)
            keyboard->setKeyPressBaseOctave (currentOctave);
        octaveLabel.setText ("C" + juce::String (currentOctave), juce::dontSendNotification);
    }

    HeaderPanel     headerPanel;
    OscillatorPanel oscAPanel  { "OSC A", SynthColours::sectionOsc };
    OscillatorPanel oscBPanel  { "OSC B", SynthColours::sectionOsc, true };  // showSync
    OscillatorPanel oscCPanel  { "OSC C", SynthColours::sectionOsc };
    WaveformDisplay waveformDisplay;
    FilterPanel     filterPanel;
    MixerPanel      mixerPanel;
    PolyModPanel    polyModPanel;
    TabbedBottomPanel tabbedPanel;
    PitchModWheelPanel wheelPanel;
    std::unique_ptr<juce::MidiKeyboardComponent> keyboard;

    // Octave controls
    juce::TextButton octaveDownBtn;
    juce::TextButton octaveUpBtn;
    juce::Label octaveLabel;
    int currentOctave = 4;

    // Wheel values (read by audio thread)
    std::atomic<float> pitchBendValue { 0.0f };
    std::atomic<float> modWheelValue { 0.0f };

    static constexpr float woodCheekWidth = 12.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainPanel)
};
