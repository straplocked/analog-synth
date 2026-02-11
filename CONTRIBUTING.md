# Contributing to AnalogSynth

Thank you for your interest in contributing to AnalogSynth! This document covers how to get started.

## Prerequisites

- **CMake** 3.22+
- **C++17** compiler (Clang, GCC 9+, or MSVC 2019+)
- **Git** with submodule support
- **macOS**: Xcode or Command Line Tools
- **Linux**: ALSA and X11 development packages (`libasound2-dev`, `libx11-dev`, `libxrandr-dev`, `libxcursor-dev`, `libfreetype-dev`, `libfontconfig1-dev`)
- **Windows**: Visual Studio 2019+ with C++ workload

## Building

```bash
git clone --recurse-submodules https://github.com/analogsynth/analog-synth.git
cd analog-synth
cmake -B build
cmake --build build
```

Build artifacts are placed in `build/AnalogSynth_artefacts/`.

## Running Tests

```bash
cmake --build build --target AnalogSynthTests
./build/AnalogSynthTests
```

All tests must pass before submitting a pull request.

## Coding Conventions

- **C++17** standard
- **`juce::` prefix** everywhere — no `using namespace juce`
- **`#pragma once`** for header guards
- **PascalCase** for class names
- **camelCase** for methods, members, and constants
- **`float`** for audio processing, **`double`** for phase accumulators
- All classes use `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR`
- JUCE 7.0.12 API — use `juce::Font(float)` not `FontOptions`, `ParameterID(stringID, versionHint)`

## Pull Request Process

1. Fork the repository and create a feature branch from `main`
2. Make your changes, following the coding conventions above
3. Add or update tests for any new functionality
4. Ensure all tests pass: `./build/AnalogSynthTests`
5. Ensure the plugin builds cleanly: `cmake -B build && cmake --build build`
6. Submit a pull request with a clear description of your changes

## License

By contributing to AnalogSynth, you agree that your contributions will be licensed under the [GNU General Public License v3.0](LICENSE). All new source files should include the GPL-3.0 copyright header found at the top of existing source files.
