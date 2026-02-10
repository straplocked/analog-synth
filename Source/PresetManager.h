#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class PresetManager
{
public:
    PresetManager (juce::AudioProcessorValueTreeState& state)
        : apvts (state)
    {
        presetDirectory = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                              .getChildFile ("AnalogSynth")
                              .getChildFile ("Presets");

        if (! presetDirectory.exists())
            presetDirectory.createDirectory();

        buildFactoryPresets();
        scanPresets();
    }

    int getNumPresets() const { return static_cast<int> (presetNames.size()); }

    juce::String getPresetName (int index) const
    {
        if (index >= 0 && index < getNumPresets())
            return presetNames[static_cast<size_t> (index)];
        return "INIT";
    }

    int getCurrentPresetIndex() const { return currentPreset; }
    juce::String getCurrentPresetName() const { return getPresetName (currentPreset); }

    void loadPreset (int index)
    {
        if (index < 0 || index >= getNumPresets()) return;

        currentPreset = index;
        auto file = presetDirectory.getChildFile (presetNames[static_cast<size_t> (index)] + ".xml");

        if (file.existsAsFile())
        {
            auto xml = juce::parseXML (file);
            if (xml != nullptr)
                apvts.replaceState (juce::ValueTree::fromXml (*xml));
        }
    }

    void loadNextPreset()
    {
        if (getNumPresets() > 0)
            loadPreset ((currentPreset + 1) % getNumPresets());
    }

    void loadPreviousPreset()
    {
        if (getNumPresets() > 0)
            loadPreset ((currentPreset - 1 + getNumPresets()) % getNumPresets());
    }

    void savePreset (const juce::String& name)
    {
        auto state = apvts.copyState();
        auto xml = state.createXml();
        if (xml == nullptr) return;

        auto file = presetDirectory.getChildFile (name + ".xml");
        xml->writeTo (file);

        scanPresets();

        for (int i = 0; i < getNumPresets(); ++i)
        {
            if (presetNames[static_cast<size_t> (i)] == name)
            {
                currentPreset = i;
                break;
            }
        }
    }

    void deleteCurrentPreset()
    {
        if (currentPreset < 0 || currentPreset >= getNumPresets()) return;

        auto file = presetDirectory.getChildFile (presetNames[static_cast<size_t> (currentPreset)] + ".xml");
        if (file.existsAsFile())
            file.deleteFile();

        scanPresets();
        currentPreset = juce::jmin (currentPreset, getNumPresets() - 1);
    }

private:
    void scanPresets()
    {
        presetNames.clear();
        auto files = presetDirectory.findChildFiles (juce::File::findFiles, false, "*.xml");
        files.sort();

        for (auto& f : files)
            presetNames.push_back (f.getFileNameWithoutExtension());
    }

    static constexpr int kFactoryPresetVersion = 2;

    void buildFactoryPresets()
    {
        // Check version marker — rebuild factory presets when version changes
        auto markerFile = presetDirectory.getChildFile ("_factory_v" + juce::String (kFactoryPresetVersion) + ".marker");
        if (markerFile.existsAsFile()) return;

        // Remove old version markers
        auto oldMarkers = presetDirectory.findChildFiles (juce::File::findFiles, false, "_factory_v*.marker");
        for (auto& m : oldMarkers)
            m.deleteFile();

        createFactoryPreset ("01 - Classic Saw Pad", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 0.0f);   // Saw
            setParam (a, "osc1_level", 0.8f);
            setParam (a, "osc2_waveform", 0.0f);    // Saw
            setParam (a, "osc2_level", 0.7f);
            setParam (a, "osc2_fine", 7.0f);         // Slight detune
            setParam (a, "lpf_cutoff", 2500.0f);
            setParam (a, "lpf_resonance", 0.2f);
            setParam (a, "lpf_envAmount", 3000.0f);
            setParam (a, "filterEnv_attack", 0.3f);
            setParam (a, "filterEnv_decay", 0.8f);
            setParam (a, "filterEnv_sustain", 0.4f);
            setParam (a, "filterEnv_release", 1.0f);
            setParam (a, "ampEnv_attack", 0.15f);
            setParam (a, "ampEnv_decay", 0.5f);
            setParam (a, "ampEnv_sustain", 0.7f);
            setParam (a, "ampEnv_release", 1.5f);
            setParam (a, "global_slop", 0.3f);
            setParam (a, "reverb_enabled", 1.0f);
            setParam (a, "reverb_size", 0.6f);
            setParam (a, "reverb_mix", 0.25f);
        });

        createFactoryPreset ("02 - Warm Bass", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 0.0f);   // Saw
            setParam (a, "osc1_level", 1.0f);
            setParam (a, "osc1_octave", -1.0f);
            setParam (a, "sub_level", 0.6f);
            setParam (a, "lpf_cutoff", 800.0f);
            setParam (a, "lpf_resonance", 0.15f);
            setParam (a, "lpf_envAmount", 2000.0f);
            setParam (a, "lpf_drive", 0.3f);
            setParam (a, "filterEnv_attack", 0.001f);
            setParam (a, "filterEnv_decay", 0.3f);
            setParam (a, "filterEnv_sustain", 0.2f);
            setParam (a, "filterEnv_release", 0.2f);
            setParam (a, "ampEnv_attack", 0.001f);
            setParam (a, "ampEnv_decay", 0.4f);
            setParam (a, "ampEnv_sustain", 0.6f);
            setParam (a, "ampEnv_release", 0.3f);
        });

        createFactoryPreset ("03 - Pulse Lead", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 1.0f);    // Pulse
            setParam (a, "osc1_pulseWidth", 0.35f);
            setParam (a, "osc1_level", 1.0f);
            setParam (a, "lpf_cutoff", 5000.0f);
            setParam (a, "lpf_resonance", 0.1f);
            setParam (a, "lpf_envAmount", 2000.0f);
            setParam (a, "filterEnv_attack", 0.001f);
            setParam (a, "filterEnv_decay", 0.15f);
            setParam (a, "filterEnv_sustain", 0.5f);
            setParam (a, "filterEnv_release", 0.4f);
            setParam (a, "ampEnv_attack", 0.001f);
            setParam (a, "ampEnv_sustain", 0.8f);
            setParam (a, "ampEnv_release", 0.3f);
            setParam (a, "delay_enabled", 1.0f);
            setParam (a, "delay_time", 0.35f);
            setParam (a, "delay_feedback", 0.4f);
            setParam (a, "delay_mix", 0.2f);
        });

        createFactoryPreset ("04 - Dreamy Keys", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 2.0f);    // Triangle
            setParam (a, "osc1_level", 0.8f);
            setParam (a, "osc2_waveform", 1.0f);     // Pulse
            setParam (a, "osc2_level", 0.5f);
            setParam (a, "osc2_pulseWidth", 0.5f);
            setParam (a, "osc2_octave", 1.0f);
            setParam (a, "lpf_cutoff", 3000.0f);
            setParam (a, "lpf_resonance", 0.1f);
            setParam (a, "filterEnv_attack", 0.01f);
            setParam (a, "filterEnv_decay", 0.6f);
            setParam (a, "filterEnv_sustain", 0.3f);
            setParam (a, "filterEnv_release", 0.8f);
            setParam (a, "ampEnv_attack", 0.01f);
            setParam (a, "ampEnv_decay", 0.3f);
            setParam (a, "ampEnv_sustain", 0.5f);
            setParam (a, "ampEnv_release", 0.8f);
            setParam (a, "chorus_enabled", 1.0f);
            setParam (a, "chorus_depth", 0.5f);
            setParam (a, "chorus_mix", 0.3f);
            setParam (a, "reverb_enabled", 1.0f);
            setParam (a, "reverb_size", 0.7f);
            setParam (a, "reverb_mix", 0.3f);
        });

        createFactoryPreset ("05 - FM Metallic", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 0.0f);   // Saw
            setParam (a, "osc1_level", 0.8f);
            setParam (a, "osc2_waveform", 0.0f);    // Saw
            setParam (a, "osc2_level", 0.0f);
            setParam (a, "osc2_semitone", 7.0f);
            setParam (a, "polyMod_osc2ToOsc1Freq", 0.4f);
            setParam (a, "lpf_cutoff", 6000.0f);
            setParam (a, "lpf_envAmount", -3000.0f);
            setParam (a, "filterEnv_attack", 0.001f);
            setParam (a, "filterEnv_decay", 0.4f);
            setParam (a, "filterEnv_sustain", 0.1f);
            setParam (a, "filterEnv_release", 0.3f);
            setParam (a, "ampEnv_attack", 0.001f);
            setParam (a, "ampEnv_decay", 0.5f);
            setParam (a, "ampEnv_sustain", 0.0f);
            setParam (a, "ampEnv_release", 0.3f);
        });

        createFactoryPreset ("06 - Thick Unison", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 0.0f);   // Saw
            setParam (a, "osc1_level", 1.0f);
            setParam (a, "global_unison", 1.0f);
            setParam (a, "global_unisonDetune", 15.0f);
            setParam (a, "lpf_cutoff", 4000.0f);
            setParam (a, "lpf_resonance", 0.05f);
            setParam (a, "lpf_envAmount", 2000.0f);
            setParam (a, "filterEnv_attack", 0.1f);
            setParam (a, "filterEnv_decay", 0.5f);
            setParam (a, "filterEnv_sustain", 0.6f);
            setParam (a, "filterEnv_release", 0.8f);
            setParam (a, "ampEnv_attack", 0.05f);
            setParam (a, "ampEnv_sustain", 0.8f);
            setParam (a, "ampEnv_release", 0.5f);
            setParam (a, "global_slop", 0.5f);
            setParam (a, "dist_enabled", 1.0f);
            setParam (a, "dist_drive", 0.3f);
            setParam (a, "dist_mix", 0.15f);
        });

        createFactoryPreset ("07 - Resonant Sweep", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 0.0f);   // Saw
            setParam (a, "osc1_level", 0.8f);
            setParam (a, "osc2_waveform", 3.0f);    // Square
            setParam (a, "osc2_level", 0.6f);
            setParam (a, "osc2_octave", -1.0f);
            setParam (a, "lpf_cutoff", 200.0f);
            setParam (a, "lpf_resonance", 0.6f);
            setParam (a, "lpf_envAmount", 8000.0f);
            setParam (a, "filterEnv_attack", 0.5f);
            setParam (a, "filterEnv_decay", 1.5f);
            setParam (a, "filterEnv_sustain", 0.0f);
            setParam (a, "filterEnv_release", 0.5f);
            setParam (a, "ampEnv_attack", 0.01f);
            setParam (a, "ampEnv_sustain", 0.7f);
            setParam (a, "ampEnv_release", 0.5f);
        });

        createFactoryPreset ("08 - Ethereal Pad", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 2.0f);    // Triangle
            setParam (a, "osc1_level", 0.6f);
            setParam (a, "osc2_waveform", 2.0f);
            setParam (a, "osc2_level", 0.6f);
            setParam (a, "osc2_fine", 5.0f);
            setParam (a, "noise_level", 0.05f);
            setParam (a, "lpf_cutoff", 1500.0f);
            setParam (a, "lpf_resonance", 0.15f);
            setParam (a, "lfo_toFilter", 300.0f);
            setParam (a, "lfo_rate", 0.3f);
            setParam (a, "filterEnv_attack", 1.0f);
            setParam (a, "filterEnv_decay", 1.0f);
            setParam (a, "filterEnv_sustain", 0.5f);
            setParam (a, "filterEnv_release", 3.0f);
            setParam (a, "ampEnv_attack", 1.0f);
            setParam (a, "ampEnv_decay", 0.5f);
            setParam (a, "ampEnv_sustain", 0.7f);
            setParam (a, "ampEnv_release", 3.0f);
            setParam (a, "chorus_enabled", 1.0f);
            setParam (a, "chorus_mix", 0.4f);
            setParam (a, "reverb_enabled", 1.0f);
            setParam (a, "reverb_size", 0.9f);
            setParam (a, "reverb_damping", 0.3f);
            setParam (a, "reverb_mix", 0.45f);
            setParam (a, "global_slop", 0.4f);
        });

        createFactoryPreset ("09 - Pluck", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 0.0f);   // Saw
            setParam (a, "osc1_level", 0.9f);
            setParam (a, "lpf_cutoff", 8000.0f);
            setParam (a, "lpf_envAmount", -5000.0f);
            setParam (a, "filterEnv_attack", 0.001f);
            setParam (a, "filterEnv_decay", 0.2f);
            setParam (a, "filterEnv_sustain", 0.0f);
            setParam (a, "filterEnv_release", 0.15f);
            setParam (a, "ampEnv_attack", 0.001f);
            setParam (a, "ampEnv_decay", 0.25f);
            setParam (a, "ampEnv_sustain", 0.0f);
            setParam (a, "ampEnv_release", 0.15f);
            setParam (a, "delay_enabled", 1.0f);
            setParam (a, "delay_time", 0.25f);
            setParam (a, "delay_feedback", 0.35f);
            setParam (a, "delay_mix", 0.25f);
            setParam (a, "delay_pingPong", 1.0f);
        });

        createFactoryPreset ("10 - PWM Strings", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 1.0f);    // Pulse
            setParam (a, "osc1_pulseWidth", 0.5f);
            setParam (a, "osc1_level", 0.8f);
            setParam (a, "osc2_waveform", 1.0f);
            setParam (a, "osc2_pulseWidth", 0.5f);
            setParam (a, "osc2_level", 0.7f);
            setParam (a, "osc2_fine", 8.0f);
            setParam (a, "lfo_toOscShape", 0.3f);
            setParam (a, "lfo_rate", 0.5f);
            setParam (a, "lpf_cutoff", 3000.0f);
            setParam (a, "lpf_resonance", 0.05f);
            setParam (a, "filterEnv_attack", 0.5f);
            setParam (a, "filterEnv_sustain", 0.5f);
            setParam (a, "filterEnv_release", 1.5f);
            setParam (a, "ampEnv_attack", 0.4f);
            setParam (a, "ampEnv_sustain", 0.8f);
            setParam (a, "ampEnv_release", 1.5f);
            setParam (a, "chorus_enabled", 1.0f);
            setParam (a, "chorus_depth", 0.6f);
            setParam (a, "chorus_mix", 0.35f);
            setParam (a, "global_slop", 0.3f);
        });

        // --- v2 factory presets (new features) ---

        createFactoryPreset ("11 - Acid Squelch", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 0.0f);    // Saw
            setParam (a, "osc1_level", 1.0f);
            setParam (a, "osc1_octave", -1.0f);
            setParam (a, "lpf_type", 3.0f);          // Acid filter
            setParam (a, "lpf_slope", 1.0f);          // 24dB
            setParam (a, "lpf_cutoff", 400.0f);
            setParam (a, "lpf_resonance", 0.7f);
            setParam (a, "lpf_envAmount", 6000.0f);
            setParam (a, "lpf_drive", 1.5f);
            setParam (a, "filterEnv_attack", 0.001f);
            setParam (a, "filterEnv_decay", 0.2f);
            setParam (a, "filterEnv_sustain", 0.0f);
            setParam (a, "filterEnv_release", 0.1f);
            setParam (a, "ampEnv_attack", 0.001f);
            setParam (a, "ampEnv_decay", 0.3f);
            setParam (a, "ampEnv_sustain", 0.0f);
            setParam (a, "ampEnv_release", 0.15f);
            setParam (a, "global_glide", 1.0f);
            setParam (a, "global_glideTime", 0.05f);
            setParam (a, "global_monoMode", 1.0f);
            setParam (a, "delay_enabled", 1.0f);
            setParam (a, "delay_time", 0.188f);
            setParam (a, "delay_feedback", 0.3f);
            setParam (a, "delay_mix", 0.15f);
        });

        createFactoryPreset ("12 - MS20 Growl", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 0.0f);    // Saw
            setParam (a, "osc1_level", 0.8f);
            setParam (a, "osc3_waveform", 0.0f);     // Saw
            setParam (a, "osc3_level", 0.7f);
            setParam (a, "osc3_fine", -8.0f);         // Detuned
            setParam (a, "lpf_type", 4.0f);           // MS20 filter
            setParam (a, "lpf_slope", 0.0f);          // 12dB
            setParam (a, "lpf_cutoff", 1200.0f);
            setParam (a, "lpf_resonance", 0.5f);
            setParam (a, "lpf_envAmount", 3000.0f);
            setParam (a, "lpf_drive", 2.0f);
            setParam (a, "filterEnv_attack", 0.01f);
            setParam (a, "filterEnv_decay", 0.4f);
            setParam (a, "filterEnv_sustain", 0.3f);
            setParam (a, "filterEnv_release", 0.3f);
            setParam (a, "ampEnv_attack", 0.01f);
            setParam (a, "ampEnv_decay", 0.3f);
            setParam (a, "ampEnv_sustain", 0.6f);
            setParam (a, "ampEnv_release", 0.4f);
            setParam (a, "global_slop", 0.4f);
        });

        createFactoryPreset ("13 - Triple Osc Pad", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 0.0f);    // Saw
            setParam (a, "osc1_level", 0.6f);
            setParam (a, "osc2_waveform", 2.0f);     // Triangle
            setParam (a, "osc2_level", 0.5f);
            setParam (a, "osc2_octave", 1.0f);
            setParam (a, "osc3_waveform", 1.0f);     // Pulse
            setParam (a, "osc3_level", 0.4f);
            setParam (a, "osc3_pulseWidth", 0.4f);
            setParam (a, "osc3_fine", 5.0f);
            setParam (a, "lpf_type", 2.0f);           // Sequential SVF
            setParam (a, "lpf_cutoff", 2000.0f);
            setParam (a, "lpf_resonance", 0.15f);
            setParam (a, "filterEnv_attack", 0.8f);
            setParam (a, "filterEnv_decay", 1.0f);
            setParam (a, "filterEnv_sustain", 0.5f);
            setParam (a, "filterEnv_release", 2.0f);
            setParam (a, "ampEnv_attack", 0.6f);
            setParam (a, "ampEnv_decay", 0.5f);
            setParam (a, "ampEnv_sustain", 0.7f);
            setParam (a, "ampEnv_release", 2.5f);
            setParam (a, "chorus_enabled", 1.0f);
            setParam (a, "chorus_depth", 0.4f);
            setParam (a, "chorus_mix", 0.3f);
            setParam (a, "reverb_enabled", 1.0f);
            setParam (a, "reverb_size", 0.8f);
            setParam (a, "reverb_mix", 0.35f);
            setParam (a, "global_slop", 0.3f);
        });

        createFactoryPreset ("14 - Deep Sub Sine", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 1.0f);    // Pulse
            setParam (a, "osc1_pulseWidth", 0.5f);
            setParam (a, "osc1_level", 0.7f);
            setParam (a, "sub_level", 0.8f);
            setParam (a, "sub_waveform", 0.0f);       // Sine sub
            setParam (a, "sub_octave", 1.0f);          // -2 Oct
            setParam (a, "lfo_toOscShape", 0.25f);     // PWM
            setParam (a, "lfo_rate", 0.4f);
            setParam (a, "lpf_cutoff", 1500.0f);
            setParam (a, "lpf_resonance", 0.1f);
            setParam (a, "lpf_envAmount", 2000.0f);
            setParam (a, "filterEnv_attack", 0.001f);
            setParam (a, "filterEnv_decay", 0.4f);
            setParam (a, "filterEnv_sustain", 0.3f);
            setParam (a, "filterEnv_release", 0.3f);
            setParam (a, "ampEnv_attack", 0.001f);
            setParam (a, "ampEnv_decay", 0.4f);
            setParam (a, "ampEnv_sustain", 0.7f);
            setParam (a, "ampEnv_release", 0.3f);
        });

        createFactoryPreset ("15 - Acid Unison", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 0.0f);    // Saw
            setParam (a, "osc1_level", 0.9f);
            setParam (a, "sub_level", 0.5f);
            setParam (a, "sub_waveform", 2.0f);       // Square sub
            setParam (a, "global_unison", 1.0f);
            setParam (a, "global_unisonDetune", 12.0f);
            setParam (a, "global_unisonSpread", 0.7f);
            setParam (a, "lpf_type", 3.0f);           // Acid filter
            setParam (a, "lpf_cutoff", 600.0f);
            setParam (a, "lpf_resonance", 0.55f);
            setParam (a, "lpf_envAmount", 5000.0f);
            setParam (a, "lpf_drive", 1.3f);
            setParam (a, "filterEnv_attack", 0.001f);
            setParam (a, "filterEnv_decay", 0.35f);
            setParam (a, "filterEnv_sustain", 0.1f);
            setParam (a, "filterEnv_release", 0.2f);
            setParam (a, "ampEnv_attack", 0.01f);
            setParam (a, "ampEnv_sustain", 0.7f);
            setParam (a, "ampEnv_release", 0.4f);
            setParam (a, "global_slop", 0.5f);
            setParam (a, "dist_enabled", 1.0f);
            setParam (a, "dist_drive", 0.2f);
            setParam (a, "dist_mix", 0.1f);
        });

        // Write version marker
        markerFile.create();
        markerFile.replaceWithText ("Factory presets version " + juce::String (kFactoryPresetVersion));
    }

    void createFactoryPreset (const juce::String& name,
                               std::function<void (juce::AudioProcessorValueTreeState&)> configure)
    {
        // Save current state
        auto savedState = apvts.copyState();

        // Reset to defaults by creating fresh state
        // (parameters already have defaults from ParameterLayout)

        // Apply preset configuration
        configure (apvts);

        // Save to file
        auto state = apvts.copyState();
        auto xml = state.createXml();
        if (xml != nullptr)
        {
            auto file = presetDirectory.getChildFile (name + ".xml");
            xml->writeTo (file);
        }

        // Restore previous state
        apvts.replaceState (savedState);
    }

    static void setParam (juce::AudioProcessorValueTreeState& a,
                           const juce::String& id, float value)
    {
        if (auto* param = a.getRawParameterValue (id))
            a.getParameter (id)->setValueNotifyingHost (
                a.getParameter (id)->convertTo0to1 (value));
    }

    juce::AudioProcessorValueTreeState& apvts;
    juce::File presetDirectory;
    std::vector<juce::String> presetNames;
    int currentPreset = -1;
};
