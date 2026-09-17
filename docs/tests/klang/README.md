# Klang language regression tests

These tests compare the archived previous production core with the working
`include/klang.h`; existing model sources remain unchanged. See the
[OSM promotion report](../../klang/OSM-IMPROVED.md). Generated wrappers,
executables, raw float renders and logs go under `build/`.

## Run

### Render, compare and inspect examples/sounds

One command from an x64 VS developer prompt creates raw renders, original-level
32-bit float WAVs, RMS/peak PNGs, sonogram PNGs, and HTML/JSON reports:

```powershell
python tests/klang/check_sounds.py --examples --fast --baseline-header build/ue-integration/baseline/klang.h --candidate-header include/klang.h --output build/model-review/msvc --inspect
```

Choose the baseline explicitly: this example uses the corrected OSM header before
the UE additions. It is also retained as `include/klang.h` inside
`osm-improved.zip`. Without `--baseline-header`, the runner uses the older,
faulty OSM baseline in `experiments/klang/core/baselines/production-2026-09-17/`.
Before future header changes, copy the accepted header to a separate baseline
directory under `build/` and pass that file. The report records both hashes.

Open `build/model-review/msvc/inspection/index.html`. Its collapsible model
sections contain players/downloads, levels, residuals and plots. Every channel
is checked, without alignment or gain correction. Exact sample agreement is
the default gate; `--allow-output-changes` keeps changes visible while permitting
intentional DSP changes. Neither numerical differences nor agreement constitute
a listening decision. Existing compile failures are listed with compiler logs;
new compile regressions and invalid/nondeterministic renders fail the runner.

Reuse saved renders without compiling or running the models again:

```powershell
python tests/klang/inspect_sounds.py build/model-review/msvc/results.json
python tests/klang/inspect_sounds.py build/model-review/msvc/results.json --source examples/Modulation/Flanger.k --all-plots --output build/model-review/flanger
```

The packager recomputes equivalence from the raw `.f32` files, verifies their
recorded hashes, exports float WAVs, and writes `report.json` plus `index.html`.
Identical pairs retain WAV links but omit redundant plots unless `--all-plots`.
Plots use 10 ms level bins, Hann FFT 16384 / hop 1024, and shared frequency/dB
axes within each comparison. `--fft 2048 --hop 256` gives finer time detail.
`--atol` / `--rtol` affect the packaged numerical classification only; defaults
are zero and the original build gate is preserved in the report. Packaging
returns failure for corrupt/missing/invalid audio; changed audio is a report
status, not a packaging failure. Failed builds remain in the report even when
there is no audio to compare. Do not reuse one output directory for two runs
concurrently. Reports do not remove older assets; the current index/JSON list
only the current selection.

Compilation/rendering needs only Python's standard library. Plotting additionally
uses `tools/requirements.txt` (already installed locally under
`build/trials/python`); its Matplotlib cache stays under `build/matplotlib`.
`python tests/klang/test_inspect_sounds.py` checks stereo differences, silence,
length/channel mismatches, explicit tolerances and partial level bins.

For Studio Clang 14.0.6, use `--compiler build/toolchains/klang14/klang.exe
--driver gnu` with `F:\klang\build\klang` on PATH. Use `--debug` for Debug;
MSVC uses `/MT` Release or `/MTd` Debug, with `/fp:fast` when `--fast` is supplied.
`--source` can also limit compilation to selected source files, saving time.

### Other contracts

`python tests/klang/check_debug_guard.py` checks `HAS_KLANG_DEBUG` defaults and
explicit overrides in Release and Debug, independently of `KLANG_DEBUG`.
Use `--compiler build/toolchains/klang14/klang.exe --driver gnu` for Studio Clang.
See [debug capture policy](../../experiments/klang/debug-guard/README.md).

From an x64 VS developer command prompt, at the repository root:

```text
python tests/klang/check.py
python tests/klang/check.py --compiler clang-cl.exe --output build/language-regression/clang
python experiments/klang/core/check_compatibility.py
python tests/klang/check_sounds.py --upstream-repo F:/klang/git
python tests/klang/check_sounds.py --upstream-repo F:/klang/git --compiler clang-cl.exe --output build/sound-headers/clang
```

Supply a full compiler path if needed. Python's standard library is sufficient.
The scripts record compiler versions, flags, selected-header include traces,
header/source hashes, results and commands. Upstream sound checks read pinned
Git blobs from the specified repository; they do not change its checkout or
fetch anything. Omit `--upstream-repo` to compare only local headers.

## Language contracts

[arithmetic.cpp](../../../tests/klang/arithmetic.cpp) tests public behaviour rather than private
implementation details:

- Float-backed `signal`/`param` layout and inheritance; arithmetic result types.
- All four arithmetic operations with mutable/const scalar, signal and parameter
  lvalues. Candidate tests add integer/float/double temporaries, both direct
  objects and inline calls returning `Output&`, plus stereo outputs.
- Noncommutative operand order and exactly-once evaluation, including two
  separate generators and source → modifier → output chains.
- Deferred inline setter calls, virtual absolute/relative dispatch, cached const
  reads and feedback input that does not advance processing.
- Existing narrowing of double constants before float signal arithmetic.
- Scalar routing to `signal` for all built-in arithmetic types, including
  `char`, unsigned integers and `long double`, with explicit float narrowing at
  the signal boundary; Klang `constant` has its own exact route.
- Basic phase wrapping and relative-phase behaviour, plus the independent
  integer-phase `optimised::Phasor`; the retained baseline cases record the old
  negative-frequency, multi-cycle and ignored-offset defects.
- Constrained Control assignment and routing, including clamping,
  `controls[i] = value`, mutable/const evaluation, mono/stereo destinations,
  and distinct `ControlMap` binding versus mapped-value assignment.
- Callable lookup-table construction without the former `FUNCTION` macro, plus
  exact flowed-versus-coupled RMS follower equivalence over 48,000 samples.

For the MSVC warning policy, compile [warnings.cpp](../../../tests/klang/warnings.cpp) with warnings
4587, 4263, 4264 and 4996 enabled and `/WX`. The default `KLANG_STRICT=0` build
passes; adding `/DKLANG_STRICT=1` deliberately exposes the warnings and fails
under `/WX`. See the [final UE details review](../../experiments/klang/ue-details/README.md).
- Function-wrapper arithmetic used by the game sounds, including preserving its
  float/double result type instead of silently selecting a new overload.

The runner also builds/runs the existing typed-literal probe: exact literal
types, overload selection and 184,200 sample-identical oscillator frames for
plain, Hz and kHz expressions across 44.1/48 kHz.

[ambiguities.cpp](../../../tests/klang/ambiguities.cpp) retains minimal compile probes:

| Expression family | Previous production | Working header |
| --- | --- | --- |
| `source(440) * .5f` | Fails | Compiles |
| `source(440) >> modifier` | Fails | Still fails |
| `set(envelope, 10)` taking `param` | Fails | Compiles; processes mutable source once |
| `1 - abs(signal(...))` | Fails | Compiles through stateless `abs` |
| Bank output routed into mono `signal` | Fails | Compiles; sums all lanes |
| `double/char/unsigned >> signal` | Compiler-dependent ambiguity | Compiles; narrows explicitly |
| `Control >> Modifier` with an integer conversion, as in Train | MSVC fails; Clang compiles | Compiles on both |

Known-failure probes must produce the expected diagnostic category and reference
the probe file. A compiler failure for an unrelated reason is not a pass. An
unexpected success also requests review of the expectation. `PASS` on these
probes means the known restriction was reproduced, not that the expression now
works. Baseline/candidate positive contracts run separately so header breakage
cannot turn the whole suite green through compilation failures.

## Sound fixtures

[check_sounds.py](../../../tests/klang/check_sounds.py) first compiles all seven unchanged sound
headers without helpers. Its separately labelled `host-fixture` mode adds only
the dependencies declared in [sound-host.h](../../../tests/klang/sound-host.h):

- `FMath::Tanh(float)` stands in as `std::tanh(float)`.
- Headers without native `Phasor` receive the definition commented out in
  `sounds/Bicycle.h`; the working header now supplies its own.
- Upstream headers receive `Sound`/stereo `Sound` aliases to `Effect` and `byte`.
- Train explicitly includes `examples/Delay/Reverb2.k`, because its own include
  names `Reverb.k` but its field names `Reverb2`. The actual game Reverb.k has
  since been confirmed identical to this Reverb2 source apart from line endings.

This is not UE emulation or a check of Unreal Build Tool's settings. Model code
is never rewritten to overcome compilation failures. In particular, no DSP or
signal-flow workaround is hidden in these host definitions.

[sound-render.h](../../../tests/klang/sound-render.h) defines six-second control recipes at 44.1 and
48 kHz, with changes at 0/2/4 seconds, `prepare()` every 64 samples, seed 12345,
fresh processes and full float output without normalisation. The period is a
fixed test-host buffer cadence, not a change to the models. Generator-only
components have no `prepare()` call. These fixtures exercise both steady-state
and changing controls; they do not claim exhaustive host/lifecycle coverage.

For successful builds the runner records sample counts, finite values, peak,
RMS, mean, samples beyond ±1, and render hashes. Candidate renders repeat in
fresh processes and must match. Candidate/baseline samples must agree wherever
both run. Upstream comparisons report residuals without assuming that every
branch difference is a defect. Missing/nonfinite/nondeterministic candidate
renders and compile regressions fail the gate; existing compile blockers and
levels above unity remain visible in the report. No test listens to or accepts
the sound on the user's behalf.

## First retained result

16 September 2026, VS 2026 MSVC 19.51.36257 and Clang 22.1.3, Windows x64,
C++17 `/O2 /fp:precise`:

- 147 baseline and 227 candidate runtime language checks pass on each compiler,
  plus compile-time assertions, compile probes and literal checks.
- All 80 model files checked unchanged: MSVC improves 61 → 72; Clang 59 → 70;
  zero new failures.
- Harrier candidate/baseline and Bicycle/Rain candidate/upstream renders are
  sample-identical at both rates wherever the relevant header compiles.
- Train's reconstructed fixture runs only on candidate Clang; it is finite and
  repeatable, but has no compiling reference for an equivalence claim.

See the [full review](../../experiments/klang/core/reviews/ARITHMETIC-REVIEW.md) and
[retained evidence](../../../experiments/klang/core/arithmetic-review.json).

Next additions should follow actual changes: isolate table initialisation,
resolve the remaining routing/conversion cases with evaluation-count tests,
then add mixed precision, precision-boundary and unit-conversion contracts for
the planned generic numeric core. Full UE builds, multitranslation-unit linkage,
other operating systems and comprehensive host-buffer lifecycle tests remain
separate coverage gaps.

## Supplied game header and oscillator review

The runners accept an optional read-only supplied header:

```text
python tests/klang/check.py --game-header F:/Future/Future.58/Plugins/Klang/Source/Public/klang.h --output build/game-header/language-msvc
python tests/klang/check_sounds.py --game-header F:/Future/Future.58/Plugins/Klang/Source/Public/klang.h --output build/game-header/msvc
python tests/klang/check_sounds.py --game-header F:/Future/Future.58/Plugins/Klang/Source/Public/klang.h --ue-language-flags --output build/game-header/ue-language-flags
python experiments/klang/core/check_compatibility.py --header reference --reference-header F:/Future/Future.58/Plugins/Klang/Source/Public/klang.h --output build/game-header/all-models-msvc
python tests/klang/check_oscillators.py --game-header F:/Future/Future.58/Plugins/Klang/Source/Public/klang.h
```

Add `--compiler clang-cl.exe` and a distinct output directory for Clang.
The game header supplies native Phasor and changes Sound's inheritance; the
fixture uses those interfaces directly. `--ue-language-flags` reproduces the
language/conformance subset of the saved build response, not a full UE build.

The supplied 0.7.9 passes 231 MSVC runtime language checks. Its complete
arithmetic contract fails to compile on Clang, honestly leaving that run red.
It fixes the Function-left-scalar, Bank-mono and Control-routing probes;
inline routing and Envelope-to-param are still diagnosed failures.
[channel-broadcast.cpp](../../../tests/klang/channel-broadcast.cpp) verifies that the working header
broadcasts a mono value 2 as `{2,2,2}`. The retained baseline and supplied 0.7.9
header still produce the historical `{2,0,0}` failure. The arithmetic contracts
also distinguish explicit channel `sum()`, `average()`/`mono()`, and Bank's
summed mono routing, including exactly-once processing.

[check_oscillators.py](../../../tests/klang/check_oscillators.py) and [oscillators.cpp](../../../tests/klang/oscillators.cpp)
measure the old/new native oscillators independently of PD. They report
waveform/phase diagnostics rather than claim an all-oscillator passing gate.
Snapshots before tick and a consistent Saw breakpoint isolate faults without
editing the reference headers. See the [OSM findings](../../klang/OSM-IMPROVED.md#archived-material),
[game-core review](../../experiments/klang/core/reviews/GAME-HEADER-REVIEW.md) and
[retained evidence](../../../experiments/klang/core/game-header-review.json).

## Production OSM regression tests

`python tests/klang/check_osm.py --fast` now defaults to the working
`include/klang.h` and the full `combined` contract, including negative FM,
multi-cycle intervals and fractional duty. Add `--debug` for Debug or
`--compiler build/toolchains/klang14/klang.exe --driver gnu` for the existing
Studio Clang launcher. Use separate `--output` directories per configuration.
The precise independent oracle remains in `oscillator-reference.h`.

`check_oscillators.py` retains the old/new oscillator metrics comparison.
`check_sounds.py --examples --fast --allow-output-changes` retains the unchanged
model comparisons. Existing compilation failures are reported explicitly.

Superseded OSM variants, benchmark generators and reports are in
[osm-improved.zip](../../../experiments/klang/osm-improved.zip), preserving their original paths.
See the [accepted result](../../klang/OSM-IMPROVED.md) for performance,
listening acceptance and archive contents. Restore historical tools in a
separate checkout when repeating those old experiments.
