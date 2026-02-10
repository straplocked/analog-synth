# AnalogSynth

Virtual analog synthesizer plugin modeled after the Sequential Prophet-6, built with JUCE 7.0.12.

## Build

```bash
cmake -B build
cmake --build build
```

Or with Xcode:
```bash
cmake -B build -G Xcode
open build/AnalogSynth.xcodeproj
```

Standalone app: `build/AnalogSynth_artefacts/Debug/Standalone/AnalogSynth.app`

## Architecture

- **DSP-first** development: sound engine in `Source/DSP/`, effects in `Source/FX/`
- **UI** in `Source/UI/` with Serum-inspired dark theme
- All parameters defined in `Source/ParameterIDs.h` and `Source/ParameterLayout.h`
- Uses `AudioProcessorValueTreeState` (APVTS) for all parameter management
- C++17, uses `juce::` prefix (no `using namespace juce`)
- `#pragma once` for header guards

## Conventions

- Class names: PascalCase
- Methods: camelCase
- Member variables: camelCase (no prefix)
- Constants: camelCase in namespaces
- All classes use `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR`
- Prefer `float` for audio processing, `double` for phase accumulators
