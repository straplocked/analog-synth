cmake_minimum_required(VERSION 3.22)
project(AnalogSynth VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(ThirdParty/JUCE)

# Embed font resources as binary data
juce_add_binary_data(AnalogSynthBinaryData
    SOURCES
        Resources/Fonts/Inter-Regular.ttf
        Resources/Fonts/Inter-Bold.ttf
)

juce_add_plugin(AnalogSynth
    COMPANY_NAME "AnalogSynth"
    PLUGIN_MANUFACTURER_CODE AnSy
    PLUGIN_CODE ASyn
    FORMATS VST3 Standalone AU
    PRODUCT_NAME "AnalogSynth"
    BUNDLE_ID "com.analogsynth.plugin"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(AnalogSynth PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/DSP/SynthVoice.cpp
    Source/DSP/VoiceManager.cpp
    Source/DSP/Arpeggiator.cpp
    Source/FX/FXChain.cpp
    Source/UI/AnalogSynthLookAndFeel.cpp
)

target_compile_definitions(AnalogSynth PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(AnalogSynth
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        AnalogSynthBinaryData
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ---- Unit Tests ----
add_executable(AnalogSynthTests
    Tests/catch_amalgamated.cpp
    Tests/TestMathHelpers.cpp
    Tests/TestFastMath.cpp
    Tests/TestSaturation.cpp
    Tests/TestFXOrderUtils.cpp
    Tests/TestSmoothParam.cpp
    Tests/TestPolyBLEPOscillator.cpp
    Tests/TestSubOscillator.cpp
    Tests/TestNoiseGenerator.cpp
    Tests/TestADSREnvelope.cpp
    Tests/TestLFO.cpp
    Tests/TestAnalogDrift.cpp
    Tests/TestMixer.cpp
    Tests/TestPolyModRouter.cpp
    Tests/TestLadderFilter.cpp
    Tests/TestFilterEngine.cpp
    Tests/TestStateVariableHPF.cpp
    Tests/TestDistortionEffect.cpp
    Tests/TestCompressorEffect.cpp
    Tests/TestFXChain.cpp
    Tests/TestArpeggiator.cpp
    Source/DSP/Arpeggiator.cpp
    Source/FX/FXChain.cpp
)

target_include_directories(AnalogSynthTests PRIVATE Source Tests)

target_link_libraries(AnalogSynthTests
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
)

target_compile_definitions(AnalogSynthTests PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_STANDALONE_APPLICATION=1
)
