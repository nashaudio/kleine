# klang/sfx — Kleine

Procedural sound models in C++17 using [Klang](https://github.com/nashaudio/klang) and the [Kleine](https://github.com/nashaudio/kleine) audio-enabled console framework. The project recreates and adapts Andy Farnell's *Designing Sound* examples and develops sounds for This Little World and the Future Sound of Bristol demo.

See [AGENTS.md](AGENTS.md) for implementation principles, the evolving comparison process, and game priorities; see [farnell/README.md](farnell/README.md) for reference provenance and PD-version findings.

## Build and run

Use a C++17 compiler and CMake. The supplied version-3 presets require CMake 3.21 or newer. On Windows, install the Visual Studio C++ tools/Windows SDK and Ninja, then run from an **x64 Native Tools Command Prompt** in the project root:

```text
cmake --preset x64-debug
cmake --build build/x64-debug
build\x64-debug\kleine.exe
```

For Release use `x64-release` and `build/x64-release`. VS Code's **Run Kleine** launch configuration uses the active CMake target and the project root as its working directory.

The current `kleine.cpp` demonstration plays a DX7 through a ping-pong delay, reads `input.flac` from its working directory, and writes `output.wav` there. Generated output is ignored by Git. It does not yet offer command-line model selection or a general offline renderer.

The initial Windows checks passed with CMake 4.4.3 and Visual Studio 18 Community/MSVC 19.51. From ordinary PowerShell on that installation, the developer environment can be scoped to a build command:

```powershell
cmd.exe /d /s /c '"C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 -no_logo && cmake --build build/x64-debug'
```

Adjust the Visual Studio path for other installations. A missing `stdio.h` during compilation can mean the compiler environment/SDK include paths have not been loaded.

Linux and macOS presets are included, but were not exercised in the initial local checks.

## Comparison tools and current limitations

- PD-Vanilla 0.55.2 is installed at `C:/Program Files/Pd/bin/pd.exe` on the current development machine; this is the initial runnable reference, not the final historical baseline.
- Use headless PD and direct Klang DSP rendering for reproducible comparisons. `Engine::SIMULATED` records MIDI events and does not render audio.
- A finite PD batch render using array capture and `soundfiler` worked. A short `writesf~` batch test exited with an incomplete file; verify completed audio, not just the process exit code. Headless `writesf~` without batch mode also worked.
- Start with peak/RMS envelopes and Hann-window sonograms: FFT 16384, hop 1024, plus a shorter FFT for transients. Listening and useful control behavior determine fidelity; CPU/memory measurements inform later optimisation.
- The existing CMake project registers no CTest cases yet. Reusable comparison scripts/tests should be tracked; generated WAVs, plots, and analysis caches go under `build/`.
- On Windows, including `klang/pd.h` after the engine currently exposes a `min` macro collision. The initial integration harness compiled with `NOMINMAX` defined before Windows headers. A project-level fix remains to be made.

## Licensing and attribution

The aim is an open resource. The licence for new project work and remaining provenance are **TBD**, with MIT or a similar permissive licence under consideration. This is not a blanket MIT grant for the repository's existing dependencies, source patches, or recordings.

| Material | Attribution and current licence information |
| --- | --- |
| Klang language (`include/klang.h`) | Copyright Chris Nash / nash.audio. **Klang Open License 1.0**, confirmed by Chris Nash on 14 September 2026; see the [full licence text](licenses/Klang-Open-License-1.0.txt). It is based on Apache 2.0 with an additional visible Klang logo/attribution condition for interactive audiovisual works. |
| Existing procedural sound models | Chris Nash / nash.audio; Farnell-derived models also credit Andy Farnell. The related [procedural library](https://github.com/nashaudio/procedural) publishes [Klang Open License 1.0](https://github.com/nashaudio/procedural/blob/main/LICENCE). Record provenance and applicable terms per local model as the collection develops. |
| Farnell patches and reference audio | Andy Farnell, *Designing Sound* (MIT Press, 2010), and any separately credited contributors. Preserve source attribution; exact redistribution terms remain TBD. The supplied archive lacks its advertised README/version notes and contains no licence file. |
| Code copied or adapted from Pure Data | Miller Puckette and other contributors, under the [upstream Standard Improved BSD licence](https://github.com/pure-data/pure-data/blob/master/LICENSE.txt), subject to individual-file notices. Retain notices and record the source revision when porting code. |
| `include/klang/audio.h` (miniaudio and bundled audio code) | David Reid and contributors. The embedded notice offers a choice of the Unlicense or MIT No Attribution; retain applicable bundled notices. |
| `include/klang/midi.h` (TinyMidiLoader) | Copyright 2017–2018 Bernhard Schelling; zlib licence, reproduced in the header. |

The book PDF is a privately owned local reference and is **not part of the public repository**. The `book/` directory and generated extracts are excluded from tracking. Source patches and supporting reference assets are tracked; preserve their original terms as provenance is resolved.

The local Klang licence text is copied verbatim from [nashaudio/procedural/LICENCE](https://github.com/nashaudio/procedural/blob/main/LICENCE), retrieved on 14 September 2026. Its presence documents the Klang dependency's terms; it does not assign that licence to every file in this repository.
