#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "AnalogSynthLookAndFeel.h"
#include "EnvelopePanel.h"
#include "LFOPanel.h"
#include "PolyModPanel.h"
#include "FXPanel.h"
#include "GlobalPanel.h"

class TabbedBottomPanel : public juce::Component
{
public:
    TabbedBottomPanel()
        : tabbedComponent (juce::TabbedButtonBar::TabsAtTop)
    {
        tabbedComponent.setTabBarDepth (30);

        envPanel     = std::make_unique<EnvelopePanel>();
        lfoPanel     = std::make_unique<LFOPanel>();
        polyModPanel = std::make_unique<PolyModPanel>();
        fxPanel      = std::make_unique<FXPanel>();
        globalPanel  = std::make_unique<GlobalPanel>();

        tabbedComponent.addTab ("ENV",      SynthColours::panelBg, envPanel.get(),      false);
        tabbedComponent.addTab ("LFO",      SynthColours::panelBg, lfoPanel.get(),      false);
        tabbedComponent.addTab ("POLY MOD", SynthColours::panelBg, polyModPanel.get(),  false);
        tabbedComponent.addTab ("FX",       SynthColours::panelBg, fxPanel.get(),       false);
        tabbedComponent.addTab ("GLOBAL",   SynthColours::panelBg, globalPanel.get(),   false);

        addAndMakeVisible (tabbedComponent);
    }

    void attach (juce::AudioProcessorValueTreeState& apvts)
    {
        envPanel->attach (apvts);
        lfoPanel->attach (apvts);
        polyModPanel->attach (apvts);
        fxPanel->attach (apvts);
        globalPanel->attach (apvts);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Groove line at top edge (1px dark + 1px light)
        g.setColour (SynthColours::faceplateEdge.darker (0.2f));
        g.fillRect (bounds.getX(), bounds.getY(), bounds.getWidth(), 1.0f);
        g.setColour (SynthColours::faceplateMid.brighter (0.08f));
        g.fillRect (bounds.getX(), bounds.getY() + 1.0f, bounds.getWidth(), 1.0f);
    }

    void resized() override
    {
        tabbedComponent.setBounds (getLocalBounds());
    }

private:
    juce::TabbedComponent tabbedComponent;

    std::unique_ptr<EnvelopePanel> envPanel;
    std::unique_ptr<LFOPanel> lfoPanel;
    std::unique_ptr<PolyModPanel> polyModPanel;
    std::unique_ptr<FXPanel> fxPanel;
    std::unique_ptr<GlobalPanel> globalPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TabbedBottomPanel)
};
