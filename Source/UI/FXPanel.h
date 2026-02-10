#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "AnalogSynthLookAndFeel.h"
#include "RotaryKnob.h"
#include "../ParameterIDs.h"

class FXPanel : public juce::Component
{
public:
    FXPanel()
    {
        // Distortion
        distDrive = std::make_unique<RotaryKnob> ("DRIVE", "");
        distTone  = std::make_unique<RotaryKnob> ("TONE", "");
        distMix   = std::make_unique<RotaryKnob> ("MIX", "%");
        distEnable.setButtonText ("DIST");
        distEnable.setClickingTogglesState (true);
        distEnable.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        distEnable.setColour (juce::TextButton::buttonOnColourId, SynthColours::sectionFx.withAlpha (0.3f));

        // Chorus
        chorusRate  = std::make_unique<RotaryKnob> ("RATE", "Hz");
        chorusDepth = std::make_unique<RotaryKnob> ("DEPTH", "");
        chorusMix   = std::make_unique<RotaryKnob> ("MIX", "%");
        chorusEnable.setButtonText ("CHORUS");
        chorusEnable.setClickingTogglesState (true);
        chorusEnable.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        chorusEnable.setColour (juce::TextButton::buttonOnColourId, SynthColours::sectionFx.withAlpha (0.3f));
        chorusFlanger.setButtonText ("FLG");
        chorusFlanger.setClickingTogglesState (true);
        chorusFlanger.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        chorusFlanger.setColour (juce::TextButton::buttonOnColourId, SynthColours::sectionFx.withAlpha (0.3f));

        // Delay
        delayTime     = std::make_unique<RotaryKnob> ("TIME", "ms");
        delayFeedback = std::make_unique<RotaryKnob> ("FDBK", "");
        delayMix      = std::make_unique<RotaryKnob> ("MIX", "%");
        delayEnable.setButtonText ("DELAY");
        delayEnable.setClickingTogglesState (true);
        delayEnable.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        delayEnable.setColour (juce::TextButton::buttonOnColourId, SynthColours::sectionFx.withAlpha (0.3f));
        delayPingPong.setButtonText ("P-P");
        delayPingPong.setClickingTogglesState (true);
        delayPingPong.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        delayPingPong.setColour (juce::TextButton::buttonOnColourId, SynthColours::sectionFx.withAlpha (0.3f));
        delayBBD.setButtonText ("BBD");
        delayBBD.setClickingTogglesState (true);
        delayBBD.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        delayBBD.setColour (juce::TextButton::buttonOnColourId, SynthColours::sectionFx.withAlpha (0.3f));

        // Reverb
        reverbSize    = std::make_unique<RotaryKnob> ("SIZE", "");
        reverbDamping = std::make_unique<RotaryKnob> ("DAMP", "");
        reverbMix     = std::make_unique<RotaryKnob> ("MIX", "%");
        reverbEnable.setButtonText ("REVERB");
        reverbEnable.setClickingTogglesState (true);
        reverbEnable.setColour (juce::TextButton::buttonColourId, SynthColours::panelBg);
        reverbEnable.setColour (juce::TextButton::buttonOnColourId, SynthColours::sectionFx.withAlpha (0.3f));
        reverbType.addItemList ({"Room", "Hall", "Plate", "Spring"}, 1);
        reverbType.setSelectedId (1, juce::dontSendNotification);

        for (auto* k : getAllKnobs())
        {
            k->setAccentColour (SynthColours::sectionFx);
            addAndMakeVisible (*k);
        }

        for (auto* b : getAllButtons())
            addAndMakeVisible (*b);

        addAndMakeVisible (reverbType);
    }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        distDrive->attach (apvts, ParameterIDs::distDrive);
        distTone->attach (apvts, ParameterIDs::distTone);
        distMix->attach (apvts, ParameterIDs::distMix);
        distEnableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::distEnabled, distEnable);

        chorusRate->attach (apvts, ParameterIDs::chorusRate);
        chorusDepth->attach (apvts, ParameterIDs::chorusDepth);
        chorusMix->attach (apvts, ParameterIDs::chorusMix);
        chorusEnableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::chorusEnabled, chorusEnable);
        chorusFlangerAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::chorusFlanger, chorusFlanger);

        delayTime->attach (apvts, ParameterIDs::delayTime);
        delayFeedback->attach (apvts, ParameterIDs::delayFeedback);
        delayMix->attach (apvts, ParameterIDs::delayMix);
        delayEnableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::delayEnabled, delayEnable);
        delayPPAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::delayPingPong, delayPingPong);
        delayBBDAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::delayBBD, delayBBD);

        reverbSize->attach (apvts, ParameterIDs::reverbSize);
        reverbDamping->attach (apvts, ParameterIDs::reverbDamping);
        reverbMix->attach (apvts, ParameterIDs::reverbMix);
        reverbEnableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParameterIDs::reverbEnabled, reverbEnable);
        reverbTypeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, ParameterIDs::reverbType, reverbType);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Brushed metal background
        AnalogSynthLookAndFeel::fillBrushedMetal (g, bounds, SynthColours::faceplateDark);

        // Silkscreen title with section color
        auto titleArea = bounds.reduced (10, 4).removeFromTop (16);
        AnalogSynthLookAndFeel::drawSectionTitle (g, "EFFECTS", titleArea, SynthColours::sectionFx);

        // Horizontal rule
        g.setColour (SynthColours::silkscreen.withAlpha (0.15f));
        g.fillRect (bounds.getX() + 8.0f, titleArea.getBottom() + 1.0f, bounds.getWidth() - 16.0f, 1.0f);

        // Vertical dividers between FX sections
        float sectionW = (bounds.getWidth() - 8.0f) / 4.0f;
        float divTop = titleArea.getBottom() + 20.0f;
        float divBot = bounds.getBottom() - 4.0f;
        for (int i = 1; i < 4; ++i)
        {
            float dx = bounds.getX() + 4.0f + sectionW * static_cast<float> (i);
            g.setColour (SynthColours::faceplateEdge.darker (0.1f));
            g.fillRect (dx, divTop, 1.0f, divBot - divTop);
            g.setColour (SynthColours::faceplateMid.brighter (0.06f));
            g.fillRect (dx + 1.0f, divTop, 1.0f, divBot - divTop);
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (6);
        bounds.removeFromTop (20);

        int sectionW = bounds.getWidth() / 4;

        auto layoutFXSection = [&] (juce::Rectangle<int> area,
                                     juce::TextButton& enableBtn,
                                     std::vector<RotaryKnob*> knobs,
                                     std::vector<juce::Component*> extras = {})
        {
            area = area.reduced (4);
            auto header = area.removeFromTop (24);
            enableBtn.setBounds (header.removeFromLeft (header.getWidth() / 2).reduced (1, 2));
            int extX = header.getX();
            for (auto* extra : extras)
            {
                int w = juce::jmin (header.getWidth(), 60);
                extra->setBounds (extX, header.getY() + 2, w, header.getHeight() - 4);
                extX += w + 2;
            }

            int knobW = area.getWidth() / static_cast<int> (knobs.size());
            for (int i = 0; i < static_cast<int> (knobs.size()); ++i)
                knobs[static_cast<size_t> (i)]->setBounds (area.getX() + i * knobW, area.getY(),
                                                            knobW, area.getHeight());
        };

        auto distArea   = bounds.removeFromLeft (sectionW);
        auto chorusArea = bounds.removeFromLeft (sectionW);
        auto delayArea  = bounds.removeFromLeft (sectionW);
        auto reverbArea = bounds;

        layoutFXSection (distArea, distEnable,
                         { distDrive.get(), distTone.get(), distMix.get() });

        layoutFXSection (chorusArea, chorusEnable,
                         { chorusRate.get(), chorusDepth.get(), chorusMix.get() },
                         { &chorusFlanger });

        layoutFXSection (delayArea, delayEnable,
                         { delayTime.get(), delayFeedback.get(), delayMix.get() },
                         { &delayPingPong, &delayBBD });

        layoutFXSection (reverbArea, reverbEnable,
                         { reverbSize.get(), reverbDamping.get(), reverbMix.get() },
                         { &reverbType });
    }

private:
    std::vector<RotaryKnob*> getAllKnobs()
    {
        return { distDrive.get(), distTone.get(), distMix.get(),
                 chorusRate.get(), chorusDepth.get(), chorusMix.get(),
                 delayTime.get(), delayFeedback.get(), delayMix.get(),
                 reverbSize.get(), reverbDamping.get(), reverbMix.get() };
    }

    std::vector<juce::Component*> getAllButtons()
    {
        return { &distEnable, &chorusEnable, &chorusFlanger,
                 &delayEnable, &delayPingPong, &delayBBD,
                 &reverbEnable };
    }

    // Distortion
    std::unique_ptr<RotaryKnob> distDrive, distTone, distMix;
    juce::TextButton distEnable;

    // Chorus
    std::unique_ptr<RotaryKnob> chorusRate, chorusDepth, chorusMix;
    juce::TextButton chorusEnable, chorusFlanger;

    // Delay
    std::unique_ptr<RotaryKnob> delayTime, delayFeedback, delayMix;
    juce::TextButton delayEnable, delayPingPong, delayBBD;

    // Reverb
    std::unique_ptr<RotaryKnob> reverbSize, reverbDamping, reverbMix;
    juce::TextButton reverbEnable;
    juce::ComboBox reverbType;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
        distEnableAttach, chorusEnableAttach, chorusFlangerAttach,
        delayEnableAttach, delayPPAttach, delayBBDAttach, reverbEnableAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> reverbTypeAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FXPanel)
};
