# Supplied UE header and game-source review

Follow-up: the [OSM repair](../../../../klang/OSM-IMPROVED.md#archived-material) is implemented in the experimental
candidate. This supplied-header comparison remains historical; neither the UE
header nor its other API changes were copied wholesale into the candidate.

16 September 2026. Chris's supplied plugin uses Klang **0.7.9**, not the 0.7.8
header in Kleine. SHA-256:
`2d3b505f92bc8a2f99f51f9c00f791ab3fd65417b05cd4ca75fa8afe7ea878d3`.
The plugin and Future sources were read only; no Unreal build was run.

## Why the game sounds compile

The supplied header restores scalar-right arithmetic and changes the competing
Function conversions: Function no longer implicitly converts to float, and
`abs` becomes a scalar function returning `signal`. It also adds channel-sum
conversion, explicit Bank-to-mono routing, revised Control routing, a native
Phasor, Sound generator classes and interleaved stereo buffers. These are
substantial core differences, not special compiler settings alone.

The actual game Bicycle, Helicopter, Motors, Nature and Train headers match the
local sounds apart from line endings. Mini subsequently replaced its UE-only
`FMath::Tanh` call with Klang's standard-backed `tanh`; Harrier differs only in
the local temporary that materialises the turbine output before arithmetic.
The game's `Audio/Reverb.k` declares Reverb2 and matches
`examples/Delay/Reverb2.k` apart
from line endings, confirming the earlier test dependency.

## Results

Standalone MSVC 19.51.36257 and Clang 22.1.3, Windows x64, `/O2 /fp:precise`:

| Check | Supplied 0.7.9 result |
| --- | --- |
| Raw sounds, MSVC | 3/7 files compile; other files require FMath or the correct Reverb2 include |
| Sounds with explicit host dependencies, MSVC C++17 | **7/7 files, 9/9 model roots compile and render** |
| Same fixtures with saved UE language/conformance flags | 9/9 roots compile and render |
| Same fixtures, Clang C++17 | 6/7 files, 6/9 roots; Motors fails on ambiguous routing from a `const double` expression |
| All unchanged models, MSVC | 44/53 examples, 3/7 raw sounds, 20/20 Farnell files: **67/80** |
| Runtime language contract, MSVC | 231 checks pass, plus compile-time assertions and literal checks |
| Runtime language contract, Clang | Compile failure: float-lvalue arithmetic competes with the restored free overloads |

The sound fixtures use a labelled `std::tanh(float)` stand-in for FMath and the
confirmed Reverb2 dependency. They render six seconds at each of 44.1/48 kHz,
with controls at 0/2/4 seconds, prepare every 64 samples, seed 12345 and no
normalisation. All 18 successful MSVC game renders are finite and non-silent.
These are smoke/comparison tests, not full UE host or listening acceptance.

Harrier and Rain remain sample-identical to the experimental candidate at both
rates. Bicycle differs (residual RMS 0.002738/0.002840); Train also differs on
Clang (0.023728/0.022647). A changed output is not automatically a regression:
the [independent oscillator review](../../../../klang/OSM-IMPROVED.md#archived-material) establishes a genuine phase
correction alongside remaining OSM faults.

The supplied core fixes the minimal Function-left-scalar, Bank-mono and Control
routing cases on MSVC. Inline Output routing and Envelope-to-param remain open.
Six examples that compile with the candidate fail with 0.7.9: Compressor
(`sqrt` ambiguity), EQ, IIR, PingPong and TB303 (`Control` to `signal` ambiguity
inside Function), and Vocoder (routing to the new scalar `abs`). Thus the game
header is valuable source material, but is not a drop-in replacement for the
whole collection. All three headers also reproduce `signals<3>(mono)` selecting
the one-channel variadic constructor and yielding `{2,0,0}` for mono value 2;
the separate test explicitly records this known runtime failure.

## Host evidence and lifecycle follow-ups

`Klang.Build.cs` enables exceptions and explicit/shared PCH. The saved
`Klang.Shared.rsp` uses C++20, `/GR-`, `/permissive-`, `/EHsc`, `/fp:precise`,
`/Zc:strictStrings-`, `/Zc:__cplusplus`, `/Zc:inline` and `/Zc:preprocessor`.
Its include paths identify MSVC 14.44 under VS 2022; that is saved-build evidence,
not a claim that Chris's VS 2026 IDE uses that toolset for every current build.
The standalone flag comparison does not reproduce UBT's entire PCH/optimisation
environment.

The component imports `optimised`, constructs the sound before setting `fs` to
44100, and explicitly calls `prepare()` before buffer processing. The new core's
buffer methods also call `prepare()`. Rate-before-construction and duplicate
preparation warrant a separate lifecycle review; this turn does not modify the
host. The editor header supplies controls, not hidden DSP arithmetic overloads.

## Next integration step

Reconcile the useful 0.7.9 API changes in the experimental core individually,
retaining the old examples and sound models as unchanged compatibility tests.
Resolve the arithmetic/routing failures and the measured OSM faults before
promoting those changes. Then continue with the generic numeric types and unit
literals. Production and experimental headers are unchanged by this review.

[Retained evidence](../../../../../experiments/klang/core/game-header-review.json) records file identities, compiler
profiles, test outcomes, residuals and oscillator measurements. Full logs and
renders remain under `build/game-header/`. Reproduction commands are in the
[test README](../../../../tests/klang/README.md).
