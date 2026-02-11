# Unit Tests Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add comprehensive unit tests covering all pure DSP, math, utility, and FX classes in the AnalogSynth project.

**Architecture:** Use Catch2 (header-only v3) as the test framework, integrated via CMake as a separate test executable that links against JUCE for types like `juce::AudioBuffer`. Tests organized by module: math/utility, DSP oscillators, DSP filters, DSP envelopes/modulators, FX chain, and arpeggiator.

**Tech Stack:** C++17, Catch2 v3, CMake, JUCE 7.0.12

---

### Task 1: Test Infrastructure Setup

**Files:**
- Create: `Tests/catch_amalgamated.hpp` (Catch2 header-only)
- Create: `Tests/catch_amalgamated.cpp` (Catch2 main)
- Create: `Tests/TestMain.cpp` (test runner entry)
- Modify: `CMakeLists.txt` (add test target)

**Step 1: Download Catch2 amalgamated header**

```bash
mkdir -p Tests
curl -L https://raw.githubusercontent.com/catchorg/Catch2/v3.5.2/extras/catch_amalgamated.hpp -o Tests/catch_amalgamated.hpp
curl -L https://raw.githubusercontent.com/catchorg/Catch2/v3.5.2/extras/catch_amalgamated.cpp -o Tests/catch_amalgamated.cpp
```

**Step 2: Add test target to CMakeLists.txt**

Add after the plugin target:

```cmake
# Unit Tests
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
```

**Step 3: Build test target to verify infrastructure**

```bash
cmake -B build && cmake --build build --target AnalogSynthTests
```

**Step 4: Run empty test suite**

```bash
./build/Tests/AnalogSynthTests
```

Expected: 0 tests passed

---

### Task 2: MathHelpers Tests

**Files:**
- Create: `Tests/TestMathHelpers.cpp`

Tests: noteToFrequency (A4=440Hz, C4, octave relationships), frequencyToNote roundtrip, dbToGain/gainToDb roundtrip, centsToRatio (100 cents = semitone)

---

### Task 3: FastMath Tests

**Files:**
- Create: `Tests/TestFastMath.cpp`

Tests: exp2 accuracy vs std::pow, exp accuracy vs std::exp, tan accuracy vs std::tan, edge cases (0, negative, large values), clamping behavior

---

### Task 4: Saturation Tests

**Files:**
- Create: `Tests/TestSaturation.cpp`

Tests: softClip output range [-1,1], warmSaturate asymmetry, tapeStyle piecewise continuity at ±1, zero-input behavior

---

### Task 5: FXOrderUtils Tests

**Files:**
- Create: `Tests/TestFXOrderUtils.cpp`

Tests: identity permutation (index 0), all 720 roundtrip conversions, boundary clamping, each permutation is unique

---

### Task 6: SmoothParam Tests

**Files:**
- Create: `Tests/TestSmoothParam.cpp`

Tests: convergence to target, reset behavior, initial state, smoothing time affects rate

---

### Task 7-17: All remaining DSP/FX test files

Each test file covers the public interface of its respective class.

---

### Final Task: Build, run, fix, commit

```bash
cmake -B build && cmake --build build --target AnalogSynthTests
./build/AnalogSynthTests --reporter console
```
