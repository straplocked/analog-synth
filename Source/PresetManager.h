#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class PresetManager : private juce::ValueTree::Listener
{
public:
    struct PresetInfo
    {
        juce::String name;
        juce::File file;
        bool isFactory;
    };

    struct Listener
    {
        virtual ~Listener() = default;
        virtual void presetChanged() = 0;
    };

    PresetManager (juce::AudioProcessorValueTreeState& state)
        : apvts (state)
    {
        auto baseDir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                            .getChildFile ("AnalogSynth")
                            .getChildFile ("Presets");

        factoryDirectory = baseDir.getChildFile ("Factory");
        userDirectory    = baseDir.getChildFile ("User");

        if (! factoryDirectory.exists())
            factoryDirectory.createDirectory();
        if (! userDirectory.exists())
            userDirectory.createDirectory();

        migrateOldLayout (baseDir);
        buildFactoryPresets();
        scanPresets();

        apvts.state.addListener (this);
    }

    ~PresetManager() override
    {
        apvts.state.removeListener (this);
    }

    // --- Query ---

    int getNumPresets() const { return static_cast<int> (presets.size()); }

    juce::String getPresetName (int index) const
    {
        if (index >= 0 && index < getNumPresets())
            return presets[static_cast<size_t> (index)].name;
        return "INIT";
    }

    int getCurrentPresetIndex() const { return currentPreset; }
    juce::String getCurrentPresetName() const { return getPresetName (currentPreset); }

    bool isFactoryPreset (int index) const
    {
        if (index >= 0 && index < getNumPresets())
            return presets[static_cast<size_t> (index)].isFactory;
        return false;
    }

    bool isCurrentPresetFactory() const { return isFactoryPreset (currentPreset); }

    bool isDirty() const { return dirty; }

    const std::vector<PresetInfo>& getAllPresets() const { return presets; }

    int getNumFactoryPresets() const
    {
        int count = 0;
        for (auto& p : presets)
            if (p.isFactory) ++count;
        return count;
    }

    int getNumUserPresets() const
    {
        return getNumPresets() - getNumFactoryPresets();
    }

    // --- Load ---

    void loadPreset (int index)
    {
        if (index < 0 || index >= getNumPresets()) return;

        currentPreset = index;
        auto& info = presets[static_cast<size_t> (index)];

        if (info.file.existsAsFile())
        {
            auto xml = juce::parseXML (info.file);
            if (xml != nullptr)
            {
                ignoreCallbacks = true;
                apvts.replaceState (juce::ValueTree::fromXml (*xml));
                ignoreCallbacks = false;
            }
        }

        dirty = false;
        notifyListeners();
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

    void initPreset()
    {
        ignoreCallbacks = true;

        for (auto* param : apvts.processor.getParameters())
        {
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
                ranged->setValueNotifyingHost (ranged->getDefaultValue());
        }

        ignoreCallbacks = false;
        currentPreset = -1;
        dirty = false;
        notifyListeners();
    }

    // --- Save ---

    void saveUserPreset (const juce::String& name)
    {
        auto state = apvts.copyState();
        auto xml = state.createXml();
        if (xml == nullptr) return;

        auto file = userDirectory.getChildFile (name + ".xml");
        xml->writeTo (file);

        scanPresets();

        for (int i = 0; i < getNumPresets(); ++i)
        {
            if (presets[static_cast<size_t> (i)].name == name
                && ! presets[static_cast<size_t> (i)].isFactory)
            {
                currentPreset = i;
                break;
            }
        }

        dirty = false;
        notifyListeners();
    }

    // --- Edit ---

    void renameCurrentPreset (const juce::String& newName)
    {
        if (currentPreset < 0 || currentPreset >= getNumPresets()) return;
        if (isCurrentPresetFactory()) return;

        auto& info = presets[static_cast<size_t> (currentPreset)];
        auto newFile = userDirectory.getChildFile (newName + ".xml");

        if (info.file.existsAsFile())
            info.file.moveFileTo (newFile);

        scanPresets();

        for (int i = 0; i < getNumPresets(); ++i)
        {
            if (presets[static_cast<size_t> (i)].name == newName
                && ! presets[static_cast<size_t> (i)].isFactory)
            {
                currentPreset = i;
                break;
            }
        }

        notifyListeners();
    }

    void deleteCurrentPreset()
    {
        if (currentPreset < 0 || currentPreset >= getNumPresets()) return;
        if (isCurrentPresetFactory()) return;

        auto& info = presets[static_cast<size_t> (currentPreset)];
        if (info.file.existsAsFile())
            info.file.deleteFile();

        scanPresets();
        currentPreset = juce::jmin (currentPreset, getNumPresets() - 1);
        dirty = false;
        notifyListeners();
    }

    // --- Listeners ---

    void addListener (Listener* l)    { listeners.add (l); }
    void removeListener (Listener* l) { listeners.remove (l); }

private:
    void notifyListeners()
    {
        listeners.call ([] (Listener& l) { l.presetChanged(); });
    }

    // ValueTree::Listener — track dirty state
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override
    {
        if (! ignoreCallbacks)
        {
            dirty = true;
            notifyListeners();
        }
    }

    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override {}
    void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override {}
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override {}
    void valueTreeParentChanged (juce::ValueTree&) override {}

    // --- Scanning ---

    void scanPresets()
    {
        presets.clear();

        auto factoryFiles = factoryDirectory.findChildFiles (juce::File::findFiles, false, "*.xml");
        factoryFiles.sort();
        for (auto& f : factoryFiles)
            presets.push_back ({ f.getFileNameWithoutExtension(), f, true });

        auto userFiles = userDirectory.findChildFiles (juce::File::findFiles, false, "*.xml");
        userFiles.sort();
        for (auto& f : userFiles)
            presets.push_back ({ f.getFileNameWithoutExtension(), f, false });
    }

    // --- Migration from old flat layout ---

    void migrateOldLayout (const juce::File& baseDir)
    {
        auto oldMarkers = baseDir.findChildFiles (juce::File::findFiles, false, "_factory_v*.marker");
        if (oldMarkers.isEmpty()) return;

        // Old layout detected — move factory XMLs and markers into Factory/
        auto oldXmls = baseDir.findChildFiles (juce::File::findFiles, false, "*.xml");
        for (auto& f : oldXmls)
            f.moveFileTo (factoryDirectory.getChildFile (f.getFileName()));

        for (auto& m : oldMarkers)
            m.deleteFile();
    }

    // --- Factory Presets ---

    static constexpr int kFactoryPresetVersion = 4;

    void buildFactoryPresets()
    {
        auto markerFile = factoryDirectory.getChildFile (
            "_factory_v" + juce::String (kFactoryPresetVersion) + ".marker");
        if (markerFile.existsAsFile()) return;

        // Remove old version markers
        auto oldMarkers = factoryDirectory.findChildFiles (juce::File::findFiles, false, "_factory_v*.marker");
        for (auto& m : oldMarkers)
            m.deleteFile();

        // Remove old factory presets so numbering stays clean
        auto oldPresets = factoryDirectory.findChildFiles (juce::File::findFiles, false, "*.xml");
        for (auto& f : oldPresets)
            f.deleteFile();

        // --- v1 factory presets ---

        createFactoryPreset ("01 - Classic Saw Pad", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 0.0f);
            setParam (a, "osc1_level", 0.8f);
            setParam (a, "osc2_waveform", 0.0f);
            setParam (a, "osc2_level", 0.7f);
            setParam (a, "osc2_fine", 7.0f);
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
            setParam (a, "osc1_waveform", 0.0f);
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
            setParam (a, "osc1_waveform", 1.0f);
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
            setParam (a, "osc1_waveform", 2.0f);
            setParam (a, "osc1_level", 0.8f);
            setParam (a, "osc2_waveform", 1.0f);
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
            setParam (a, "osc1_waveform", 0.0f);
            setParam (a, "osc1_level", 0.8f);
            setParam (a, "osc2_waveform", 0.0f);
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
            setParam (a, "osc1_waveform", 0.0f);
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
            setParam (a, "osc1_waveform", 0.0f);
            setParam (a, "osc1_level", 0.8f);
            setParam (a, "osc2_waveform", 3.0f);
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
            setParam (a, "osc1_waveform", 2.0f);
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
            setParam (a, "osc1_waveform", 0.0f);
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
            setParam (a, "osc1_waveform", 1.0f);
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

        // --- v2 factory presets ---

        createFactoryPreset ("11 - Acid Squelch", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 0.0f);
            setParam (a, "osc1_level", 1.0f);
            setParam (a, "osc1_octave", -1.0f);
            setParam (a, "lpf_type", 3.0f);
            setParam (a, "lpf_slope", 1.0f);
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
            setParam (a, "osc1_waveform", 0.0f);
            setParam (a, "osc1_level", 0.8f);
            setParam (a, "osc3_waveform", 0.0f);
            setParam (a, "osc3_level", 0.7f);
            setParam (a, "osc3_fine", -8.0f);
            setParam (a, "lpf_type", 4.0f);
            setParam (a, "lpf_slope", 0.0f);
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
            setParam (a, "osc1_waveform", 0.0f);
            setParam (a, "osc1_level", 0.6f);
            setParam (a, "osc2_waveform", 2.0f);
            setParam (a, "osc2_level", 0.5f);
            setParam (a, "osc2_octave", 1.0f);
            setParam (a, "osc3_waveform", 1.0f);
            setParam (a, "osc3_level", 0.4f);
            setParam (a, "osc3_pulseWidth", 0.4f);
            setParam (a, "osc3_fine", 5.0f);
            setParam (a, "lpf_type", 2.0f);
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
            setParam (a, "osc1_waveform", 1.0f);
            setParam (a, "osc1_pulseWidth", 0.5f);
            setParam (a, "osc1_level", 0.7f);
            setParam (a, "sub_level", 0.8f);
            setParam (a, "sub_waveform", 0.0f);
            setParam (a, "sub_octave", 1.0f);
            setParam (a, "lfo_toOscShape", 0.25f);
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
            setParam (a, "osc1_waveform", 0.0f);
            setParam (a, "osc1_level", 0.9f);
            setParam (a, "sub_level", 0.5f);
            setParam (a, "sub_waveform", 2.0f);
            setParam (a, "global_unison", 1.0f);
            setParam (a, "global_unisonDetune", 12.0f);
            setParam (a, "global_unisonSpread", 0.7f);
            setParam (a, "lpf_type", 3.0f);
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

        // --- v3 factory presets ---

        // Basses
        createFactoryPreset ("16 - Finger Bass", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 0.0f);      // Saw
            setParam (a, "osc1_level", 0.9f);
            setParam (a, "osc1_octave", -1.0f);
            setParam (a, "sub_level", 0.5f);
            setParam (a, "sub_waveform", 0.0f);         // Sine sub
            setParam (a, "lpf_cutoff", 1200.0f);
            setParam (a, "lpf_resonance", 0.1f);
            setParam (a, "lpf_envAmount", 2500.0f);
            setParam (a, "lpf_drive", 0.2f);
            setParam (a, "filterEnv_attack", 0.001f);
            setParam (a, "filterEnv_decay", 0.15f);
            setParam (a, "filterEnv_sustain", 0.15f);
            setParam (a, "filterEnv_release", 0.15f);
            setParam (a, "ampEnv_attack", 0.001f);
            setParam (a, "ampEnv_decay", 0.3f);
            setParam (a, "ampEnv_sustain", 0.5f);
            setParam (a, "ampEnv_release", 0.2f);
            setParam (a, "global_monoMode", 1.0f);
        });

        createFactoryPreset ("17 - Reese Bass", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 0.0f);      // Saw
            setParam (a, "osc1_level", 0.8f);
            setParam (a, "osc1_octave", -1.0f);
            setParam (a, "osc2_waveform", 0.0f);       // Saw
            setParam (a, "osc2_level", 0.8f);
            setParam (a, "osc2_octave", -1.0f);
            setParam (a, "osc2_fine", 15.0f);
            setParam (a, "lpf_cutoff", 600.0f);
            setParam (a, "lpf_resonance", 0.1f);
            setParam (a, "lpf_drive", 0.5f);
            setParam (a, "lfo_toOscFreq", 3.0f);
            setParam (a, "lfo_rate", 0.15f);
            setParam (a, "filterEnv_attack", 0.01f);
            setParam (a, "filterEnv_decay", 0.5f);
            setParam (a, "filterEnv_sustain", 0.4f);
            setParam (a, "filterEnv_release", 0.3f);
            setParam (a, "ampEnv_attack", 0.01f);
            setParam (a, "ampEnv_sustain", 0.8f);
            setParam (a, "ampEnv_release", 0.3f);
            setParam (a, "global_monoMode", 1.0f);
            setParam (a, "dist_enabled", 1.0f);
            setParam (a, "dist_drive", 0.4f);
            setParam (a, "dist_mix", 0.2f);
        });

        createFactoryPreset ("18 - Acid Bass 303", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 3.0f);      // Square
            setParam (a, "osc1_level", 1.0f);
            setParam (a, "osc1_octave", -1.0f);
            setParam (a, "lpf_type", 3.0f);             // Acid
            setParam (a, "lpf_slope", 1.0f);             // 24dB
            setParam (a, "lpf_cutoff", 350.0f);
            setParam (a, "lpf_resonance", 0.75f);
            setParam (a, "lpf_envAmount", 7000.0f);
            setParam (a, "lpf_drive", 1.8f);
            setParam (a, "filterEnv_attack", 0.001f);
            setParam (a, "filterEnv_decay", 0.15f);
            setParam (a, "filterEnv_sustain", 0.0f);
            setParam (a, "filterEnv_release", 0.08f);
            setParam (a, "ampEnv_attack", 0.001f);
            setParam (a, "ampEnv_decay", 0.2f);
            setParam (a, "ampEnv_sustain", 0.0f);
            setParam (a, "ampEnv_release", 0.1f);
            setParam (a, "global_monoMode", 1.0f);
            setParam (a, "global_glide", 1.0f);
            setParam (a, "global_glideTime", 0.04f);
            setParam (a, "dist_enabled", 1.0f);
            setParam (a, "dist_drive", 0.5f);
            setParam (a, "dist_mix", 0.25f);
        });

        // Leads
        createFactoryPreset ("19 - Sync Lead", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 0.0f);      // Saw
            setParam (a, "osc1_level", 0.9f);
            setParam (a, "osc2_waveform", 0.0f);       // Saw
            setParam (a, "osc2_level", 0.0f);
            setParam (a, "osc2_sync", 1.0f);
            setParam (a, "osc2_semitone", 5.0f);
            setParam (a, "lpf_cutoff", 6000.0f);
            setParam (a, "lpf_resonance", 0.15f);
            setParam (a, "lpf_envAmount", 4000.0f);
            setParam (a, "filterEnv_attack", 0.001f);
            setParam (a, "filterEnv_decay", 0.3f);
            setParam (a, "filterEnv_sustain", 0.3f);
            setParam (a, "filterEnv_release", 0.4f);
            setParam (a, "ampEnv_attack", 0.001f);
            setParam (a, "ampEnv_sustain", 0.8f);
            setParam (a, "ampEnv_release", 0.3f);
            setParam (a, "delay_enabled", 1.0f);
            setParam (a, "delay_time", 0.3f);
            setParam (a, "delay_feedback", 0.35f);
            setParam (a, "delay_mix", 0.2f);
        });

        createFactoryPreset ("20 - Mono Screamer", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 0.0f);      // Saw
            setParam (a, "osc1_level", 0.8f);
            setParam (a, "osc2_waveform", 1.0f);       // Pulse
            setParam (a, "osc2_level", 0.6f);
            setParam (a, "osc2_pulseWidth", 0.3f);
            setParam (a, "lpf_type", 4.0f);             // MS20
            setParam (a, "lpf_cutoff", 3500.0f);
            setParam (a, "lpf_resonance", 0.6f);
            setParam (a, "lpf_envAmount", 3000.0f);
            setParam (a, "lpf_drive", 1.5f);
            setParam (a, "lfo_toOscFreq", 5.0f);
            setParam (a, "lfo_rate", 5.5f);
            setParam (a, "filterEnv_attack", 0.001f);
            setParam (a, "filterEnv_decay", 0.2f);
            setParam (a, "filterEnv_sustain", 0.5f);
            setParam (a, "filterEnv_release", 0.3f);
            setParam (a, "ampEnv_attack", 0.001f);
            setParam (a, "ampEnv_sustain", 0.9f);
            setParam (a, "ampEnv_release", 0.2f);
            setParam (a, "global_monoMode", 1.0f);
        });

        createFactoryPreset ("21 - Portamento Lead", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 2.0f);      // Triangle
            setParam (a, "osc1_level", 0.9f);
            setParam (a, "lpf_cutoff", 4000.0f);
            setParam (a, "lpf_resonance", 0.1f);
            setParam (a, "lpf_envAmount", 1500.0f);
            setParam (a, "filterEnv_attack", 0.01f);
            setParam (a, "filterEnv_decay", 0.3f);
            setParam (a, "filterEnv_sustain", 0.4f);
            setParam (a, "filterEnv_release", 0.5f);
            setParam (a, "ampEnv_attack", 0.01f);
            setParam (a, "ampEnv_sustain", 0.8f);
            setParam (a, "ampEnv_release", 0.4f);
            setParam (a, "global_monoMode", 1.0f);
            setParam (a, "global_glide", 1.0f);
            setParam (a, "global_glideTime", 0.15f);
            setParam (a, "chorus_enabled", 1.0f);
            setParam (a, "chorus_depth", 0.4f);
            setParam (a, "chorus_mix", 0.25f);
        });

        // Pads
        createFactoryPreset ("22 - Dark Choir", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 1.0f);      // Pulse
            setParam (a, "osc1_pulseWidth", 0.4f);
            setParam (a, "osc1_level", 0.6f);
            setParam (a, "osc2_waveform", 2.0f);       // Triangle
            setParam (a, "osc2_level", 0.5f);
            setParam (a, "osc2_fine", 6.0f);
            setParam (a, "osc3_waveform", 0.0f);       // Saw
            setParam (a, "osc3_level", 0.4f);
            setParam (a, "osc3_octave", -1.0f);
            setParam (a, "lpf_cutoff", 800.0f);
            setParam (a, "lpf_resonance", 0.1f);
            setParam (a, "lpf_envAmount", 1000.0f);
            setParam (a, "filterEnv_attack", 1.5f);
            setParam (a, "filterEnv_decay", 1.0f);
            setParam (a, "filterEnv_sustain", 0.4f);
            setParam (a, "filterEnv_release", 3.0f);
            setParam (a, "ampEnv_attack", 1.2f);
            setParam (a, "ampEnv_decay", 0.5f);
            setParam (a, "ampEnv_sustain", 0.6f);
            setParam (a, "ampEnv_release", 3.5f);
            setParam (a, "reverb_enabled", 1.0f);
            setParam (a, "reverb_size", 0.95f);
            setParam (a, "reverb_damping", 0.5f);
            setParam (a, "reverb_mix", 0.5f);
            setParam (a, "global_slop", 0.3f);
        });

        createFactoryPreset ("23 - Glass Shimmer", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 2.0f);      // Triangle
            setParam (a, "osc1_level", 0.7f);
            setParam (a, "osc2_waveform", 1.0f);       // Pulse
            setParam (a, "osc2_level", 0.4f);
            setParam (a, "osc2_octave", 1.0f);
            setParam (a, "osc2_pulseWidth", 0.45f);
            setParam (a, "lpf_cutoff", 6000.0f);
            setParam (a, "lpf_resonance", 0.05f);
            setParam (a, "filterEnv_attack", 1.0f);
            setParam (a, "filterEnv_decay", 0.8f);
            setParam (a, "filterEnv_sustain", 0.6f);
            setParam (a, "filterEnv_release", 4.0f);
            setParam (a, "ampEnv_attack", 1.5f);
            setParam (a, "ampEnv_decay", 0.5f);
            setParam (a, "ampEnv_sustain", 0.6f);
            setParam (a, "ampEnv_release", 4.0f);
            setParam (a, "chorus_enabled", 1.0f);
            setParam (a, "chorus_depth", 0.7f);
            setParam (a, "chorus_mix", 0.4f);
            setParam (a, "reverb_enabled", 1.0f);
            setParam (a, "reverb_size", 0.85f);
            setParam (a, "reverb_mix", 0.45f);
        });

        createFactoryPreset ("24 - Analog Strings", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 0.0f);      // Saw
            setParam (a, "osc1_level", 0.7f);
            setParam (a, "osc2_waveform", 0.0f);       // Saw
            setParam (a, "osc2_level", 0.7f);
            setParam (a, "osc2_fine", 10.0f);
            setParam (a, "lpf_type", 2.0f);             // Sequential SVF
            setParam (a, "lpf_cutoff", 2500.0f);
            setParam (a, "lpf_resonance", 0.05f);
            setParam (a, "lfo_toFilter", 200.0f);
            setParam (a, "lfo_rate", 0.4f);
            setParam (a, "filterEnv_attack", 0.5f);
            setParam (a, "filterEnv_decay", 0.8f);
            setParam (a, "filterEnv_sustain", 0.5f);
            setParam (a, "filterEnv_release", 1.5f);
            setParam (a, "ampEnv_attack", 0.3f);
            setParam (a, "ampEnv_sustain", 0.8f);
            setParam (a, "ampEnv_release", 1.5f);
            setParam (a, "chorus_enabled", 1.0f);
            setParam (a, "chorus_depth", 0.5f);
            setParam (a, "chorus_mix", 0.35f);
            setParam (a, "global_slop", 0.25f);
        });

        // Keys / Plucks
        createFactoryPreset ("25 - Electric Piano", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 2.0f);      // Triangle
            setParam (a, "osc1_level", 0.8f);
            setParam (a, "osc2_waveform", 1.0f);       // Pulse
            setParam (a, "osc2_level", 0.4f);
            setParam (a, "osc2_pulseWidth", 0.5f);
            setParam (a, "osc2_octave", 1.0f);
            setParam (a, "lpf_cutoff", 3500.0f);
            setParam (a, "lpf_resonance", 0.08f);
            setParam (a, "lpf_envAmount", 2000.0f);
            setParam (a, "filterEnv_attack", 0.001f);
            setParam (a, "filterEnv_decay", 0.4f);
            setParam (a, "filterEnv_sustain", 0.2f);
            setParam (a, "filterEnv_release", 0.5f);
            setParam (a, "filterEnv_velocity", 0.6f);
            setParam (a, "ampEnv_attack", 0.001f);
            setParam (a, "ampEnv_decay", 0.5f);
            setParam (a, "ampEnv_sustain", 0.4f);
            setParam (a, "ampEnv_release", 0.5f);
            setParam (a, "ampEnv_velocity", 0.5f);
            setParam (a, "chorus_enabled", 1.0f);
            setParam (a, "chorus_depth", 0.3f);
            setParam (a, "chorus_mix", 0.2f);
        });

        createFactoryPreset ("26 - Clav", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 1.0f);      // Pulse
            setParam (a, "osc1_pulseWidth", 0.15f);
            setParam (a, "osc1_level", 1.0f);
            setParam (a, "lpf_type", 1.0f);             // Diode
            setParam (a, "lpf_cutoff", 4000.0f);
            setParam (a, "lpf_resonance", 0.2f);
            setParam (a, "lpf_envAmount", 3000.0f);
            setParam (a, "filterEnv_attack", 0.001f);
            setParam (a, "filterEnv_decay", 0.1f);
            setParam (a, "filterEnv_sustain", 0.0f);
            setParam (a, "filterEnv_release", 0.1f);
            setParam (a, "filterEnv_velocity", 0.7f);
            setParam (a, "ampEnv_attack", 0.001f);
            setParam (a, "ampEnv_decay", 0.15f);
            setParam (a, "ampEnv_sustain", 0.0f);
            setParam (a, "ampEnv_release", 0.1f);
            setParam (a, "ampEnv_velocity", 0.6f);
        });

        createFactoryPreset ("27 - Bell", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 2.0f);      // Triangle
            setParam (a, "osc1_level", 0.7f);
            setParam (a, "osc2_waveform", 2.0f);       // Triangle
            setParam (a, "osc2_level", 0.0f);
            setParam (a, "osc2_semitone", 7.0f);        // Fifth interval
            setParam (a, "polyMod_osc2ToOsc1Freq", 0.35f);
            setParam (a, "lpf_cutoff", 5000.0f);
            setParam (a, "lpf_resonance", 0.05f);
            setParam (a, "filterEnv_attack", 0.001f);
            setParam (a, "filterEnv_decay", 0.8f);
            setParam (a, "filterEnv_sustain", 0.0f);
            setParam (a, "filterEnv_release", 2.0f);
            setParam (a, "ampEnv_attack", 0.001f);
            setParam (a, "ampEnv_decay", 1.0f);
            setParam (a, "ampEnv_sustain", 0.0f);
            setParam (a, "ampEnv_release", 2.5f);
            setParam (a, "delay_enabled", 1.0f);
            setParam (a, "delay_time", 0.4f);
            setParam (a, "delay_feedback", 0.3f);
            setParam (a, "delay_mix", 0.2f);
            setParam (a, "reverb_enabled", 1.0f);
            setParam (a, "reverb_size", 0.7f);
            setParam (a, "reverb_mix", 0.3f);
        });

        // FX / Textures
        createFactoryPreset ("28 - Wind Noise", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 2.0f);      // Triangle
            setParam (a, "osc1_level", 0.2f);
            setParam (a, "noise_level", 0.7f);
            setParam (a, "lpf_cutoff", 1000.0f);
            setParam (a, "lpf_resonance", 0.5f);
            setParam (a, "lpf_envAmount", 2000.0f);
            setParam (a, "lfo_toFilter", 800.0f);
            setParam (a, "lfo_rate", 0.2f);
            setParam (a, "lfo_shape", 2.0f);            // Triangle LFO
            setParam (a, "filterEnv_attack", 2.0f);
            setParam (a, "filterEnv_decay", 1.0f);
            setParam (a, "filterEnv_sustain", 0.3f);
            setParam (a, "filterEnv_release", 3.0f);
            setParam (a, "ampEnv_attack", 1.5f);
            setParam (a, "ampEnv_decay", 0.5f);
            setParam (a, "ampEnv_sustain", 0.5f);
            setParam (a, "ampEnv_release", 4.0f);
            setParam (a, "reverb_enabled", 1.0f);
            setParam (a, "reverb_size", 0.95f);
            setParam (a, "reverb_mix", 0.6f);
        });

        createFactoryPreset ("29 - Sci-Fi Sweep", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 0.0f);      // Saw
            setParam (a, "osc1_level", 0.7f);
            setParam (a, "osc2_waveform", 0.0f);       // Saw
            setParam (a, "osc2_level", 0.0f);
            setParam (a, "osc2_octave", 1.0f);
            setParam (a, "polyMod_fenvToOsc1Freq", 0.5f);
            setParam (a, "lpf_cutoff", 500.0f);
            setParam (a, "lpf_resonance", 0.4f);
            setParam (a, "lpf_envAmount", 6000.0f);
            setParam (a, "filterEnv_attack", 2.0f);
            setParam (a, "filterEnv_decay", 1.5f);
            setParam (a, "filterEnv_sustain", 0.0f);
            setParam (a, "filterEnv_release", 2.0f);
            setParam (a, "ampEnv_attack", 0.5f);
            setParam (a, "ampEnv_sustain", 0.7f);
            setParam (a, "ampEnv_release", 3.0f);
            setParam (a, "delay_enabled", 1.0f);
            setParam (a, "delay_time", 0.5f);
            setParam (a, "delay_feedback", 0.45f);
            setParam (a, "delay_mix", 0.25f);
            setParam (a, "reverb_enabled", 1.0f);
            setParam (a, "reverb_size", 0.8f);
            setParam (a, "reverb_mix", 0.35f);
        });

        // Arpeggiated
        createFactoryPreset ("30 - Arp Sequences", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 0.0f);      // Saw
            setParam (a, "osc1_level", 0.8f);
            setParam (a, "sub_level", 0.4f);
            setParam (a, "lpf_cutoff", 3000.0f);
            setParam (a, "lpf_resonance", 0.15f);
            setParam (a, "lpf_envAmount", 2000.0f);
            setParam (a, "filterEnv_attack", 0.001f);
            setParam (a, "filterEnv_decay", 0.15f);
            setParam (a, "filterEnv_sustain", 0.1f);
            setParam (a, "filterEnv_release", 0.1f);
            setParam (a, "ampEnv_attack", 0.001f);
            setParam (a, "ampEnv_decay", 0.15f);
            setParam (a, "ampEnv_sustain", 0.0f);
            setParam (a, "ampEnv_release", 0.1f);
            setParam (a, "arp_enabled", 1.0f);
            setParam (a, "arp_mode", 0.0f);             // Up
            setParam (a, "arp_syncDiv", 3.0f);           // 1/16
            setParam (a, "arp_octaves", 2.0f);
            setParam (a, "arp_gate", 0.5f);
            setParam (a, "delay_enabled", 1.0f);
            setParam (a, "delay_time", 0.188f);
            setParam (a, "delay_feedback", 0.4f);
            setParam (a, "delay_mix", 0.2f);
            setParam (a, "delay_pingPong", 1.0f);
        });

        createFactoryPreset ("31 - Ambient Arp", [] (juce::AudioProcessorValueTreeState& a) {
            setParam (a, "osc1_waveform", 2.0f);      // Triangle
            setParam (a, "osc1_level", 0.7f);
            setParam (a, "lpf_cutoff", 2000.0f);
            setParam (a, "lpf_resonance", 0.1f);
            setParam (a, "lpf_envAmount", 1500.0f);
            setParam (a, "filterEnv_attack", 0.01f);
            setParam (a, "filterEnv_decay", 0.4f);
            setParam (a, "filterEnv_sustain", 0.2f);
            setParam (a, "filterEnv_release", 0.8f);
            setParam (a, "ampEnv_attack", 0.01f);
            setParam (a, "ampEnv_decay", 0.3f);
            setParam (a, "ampEnv_sustain", 0.3f);
            setParam (a, "ampEnv_release", 0.8f);
            setParam (a, "arp_enabled", 1.0f);
            setParam (a, "arp_mode", 3.0f);             // Random
            setParam (a, "arp_syncDiv", 2.0f);           // 1/8T
            setParam (a, "arp_octaves", 3.0f);
            setParam (a, "arp_gate", 0.7f);
            setParam (a, "chorus_enabled", 1.0f);
            setParam (a, "chorus_depth", 0.5f);
            setParam (a, "chorus_mix", 0.3f);
            setParam (a, "reverb_enabled", 1.0f);
            setParam (a, "reverb_size", 0.9f);
            setParam (a, "reverb_damping", 0.4f);
            setParam (a, "reverb_mix", 0.5f);
        });

        // Write version marker
        markerFile.create();
        markerFile.replaceWithText ("Factory presets version " + juce::String (kFactoryPresetVersion));
    }

    void createFactoryPreset (const juce::String& name,
                               std::function<void (juce::AudioProcessorValueTreeState&)> configure)
    {
        auto savedState = apvts.copyState();

        configure (apvts);

        auto state = apvts.copyState();
        auto xml = state.createXml();
        if (xml != nullptr)
        {
            auto file = factoryDirectory.getChildFile (name + ".xml");
            xml->writeTo (file);
        }

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
    juce::File factoryDirectory;
    juce::File userDirectory;
    std::vector<PresetInfo> presets;
    int currentPreset = -1;
    bool dirty = false;
    bool ignoreCallbacks = false;
    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
