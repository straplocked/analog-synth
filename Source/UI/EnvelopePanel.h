#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "AnalogSynthLookAndFeel.h"
#include "RotaryKnob.h"
#include "ADSREditor.h"
#include "../ParameterIDs.h"

class EnvelopePanel : public juce::Component
{
public:
    EnvelopePanel()
        : filterEnvEditor ("FILTER ENV", SynthColours::sectionEnv),
          ampEnvEditor ("AMP ENV", SynthColours::accent)
    {
        addAndMakeVisible (filterEnvEditor);
        addAndMakeVisible (ampEnvEditor);

        // Filter Env knobs
        fAttack   = std::make_unique<RotaryKnob> ("A", "s");
        fDecay    = std::make_unique<RotaryKnob> ("D", "s");
        fSustain  = std::make_unique<RotaryKnob> ("S", "");
        fRelease  = std::make_unique<RotaryKnob> ("R", "s");
        fVelocity = std::make_unique<RotaryKnob> ("VEL", "");

        for (auto* k : getFilterKnobs())
        {
            k->setAccentColour (SynthColours::sectionEnv);
            addAndMakeVisible (*k);
        }

        // Amp Env knobs
        aAttack   = std::make_unique<RotaryKnob> ("A", "s");
        aDecay    = std::make_unique<RotaryKnob> ("D", "s");
        aSustain  = std::make_unique<RotaryKnob> ("S", "");
        aRelease  = std::make_unique<RotaryKnob> ("R", "s");
        aVelocity = std::make_unique<RotaryKnob> ("VEL", "");

        for (auto* k : getAmpKnobs())
        {
            k->setAccentColour (SynthColours::accent);
            addAndMakeVisible (*k);
        }
    }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        filterEnvEditor.attach (apvts,
                                ParameterIDs::filterEnvAttack,
                                ParameterIDs::filterEnvDecay,
                                ParameterIDs::filterEnvSustain,
                                ParameterIDs::filterEnvRelease);

        ampEnvEditor.attach (apvts,
                             ParameterIDs::ampEnvAttack,
                             ParameterIDs::ampEnvDecay,
                             ParameterIDs::ampEnvSustain,
                             ParameterIDs::ampEnvRelease);

        fAttack->attach (apvts, ParameterIDs::filterEnvAttack);
        fDecay->attach (apvts, ParameterIDs::filterEnvDecay);
        fSustain->attach (apvts, ParameterIDs::filterEnvSustain);
        fRelease->attach (apvts, ParameterIDs::filterEnvRelease);
        fVelocity->attach (apvts, ParameterIDs::filterEnvVelocity);

        aAttack->attach (apvts, ParameterIDs::ampEnvAttack);
        aDecay->attach (apvts, ParameterIDs::ampEnvDecay);
        aSustain->attach (apvts, ParameterIDs::ampEnvSustain);
        aRelease->attach (apvts, ParameterIDs::ampEnvRelease);
        aVelocity->attach (apvts, ParameterIDs::ampEnvVelocity);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Brushed metal background
        AnalogSynthLookAndFeel::fillBrushedMetal (g, bounds, SynthColours::faceplateDark);

        // Vertical divider between filter and amp envelope halves
        float halfX = bounds.getWidth() * 0.5f;
        g.setColour (SynthColours::faceplateEdge.darker (0.1f));
        g.fillRect (halfX, bounds.getY() + 4.0f, 1.0f, bounds.getHeight() - 8.0f);
        g.setColour (SynthColours::faceplateMid.brighter (0.08f));
        g.fillRect (halfX + 1.0f, bounds.getY() + 4.0f, 1.0f, bounds.getHeight() - 8.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        int halfW = bounds.getWidth() / 2;

        auto leftHalf  = bounds.removeFromLeft (halfW).reduced (6);
        auto rightHalf = bounds.reduced (6);

        // Filter env: visual on top, knobs below
        filterEnvEditor.setBounds (leftHalf.removeFromTop (leftHalf.getHeight() * 2 / 3));
        layoutKnobRow (leftHalf, getFilterKnobs());

        // Amp env: visual on top, knobs below
        ampEnvEditor.setBounds (rightHalf.removeFromTop (rightHalf.getHeight() * 2 / 3));
        layoutKnobRow (rightHalf, getAmpKnobs());
    }

private:
    void layoutKnobRow (juce::Rectangle<int> area, std::vector<RotaryKnob*> knobs)
    {
        int knobW = area.getWidth() / static_cast<int> (knobs.size());
        for (int i = 0; i < static_cast<int> (knobs.size()); ++i)
            knobs[static_cast<size_t> (i)]->setBounds (area.getX() + i * knobW, area.getY(),
                                                        knobW, area.getHeight());
    }

    std::vector<RotaryKnob*> getFilterKnobs()
    {
        return { fAttack.get(), fDecay.get(), fSustain.get(), fRelease.get(), fVelocity.get() };
    }

    std::vector<RotaryKnob*> getAmpKnobs()
    {
        return { aAttack.get(), aDecay.get(), aSustain.get(), aRelease.get(), aVelocity.get() };
    }

    ADSREditor filterEnvEditor, ampEnvEditor;

    std::unique_ptr<RotaryKnob> fAttack, fDecay, fSustain, fRelease, fVelocity;
    std::unique_ptr<RotaryKnob> aAttack, aDecay, aSustain, aRelease, aVelocity;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvelopePanel)
};
