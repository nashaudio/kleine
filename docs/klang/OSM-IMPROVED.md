# Working OSM promotion and existing-model comparison

17 September 2026. **The combined reflection OSM is now the working
[`include/klang.h`](../../include/klang.h).** It includes signed frequencies,
multi-cycle intervals and fractional/tiny duty. The
[previous production header](../../experiments/klang/core/baselines/production-2026-09-17/klang.h) is retained
byte-for-byte. [WORKING-PRODUCTION.json](working-production.json) pins both hashes
and the generating fragment. External Studio and UE trees were not changed.

Chris authorised promotion unless the FM/PWM results were catastrophic.
Isolated modulation costs are material, but absolute costs and the previously
measured full SynTHX performance support using this as the corrected working
version. This is not a claim of unchanged throughput in every workload.

The promoted candidate also contains the earlier constrained scalar-arithmetic
restoration and shared Fast::Increment/Phase and Sine lifecycle repairs. It does
not include the planned generic numeric types, 64-bit types or unit literals.
Public oscillator syntax, subclassing and virtual overrides are preserved.
`experiments/klang/core/klang.h` remains an older experimental candidate; select
the working header through `include/`, not that historical directory.

## FM/PWM profile

Compared the **reflection baseline without either extension** with the combined
version. Forty workloads cover Saw, Triangle, Square and Pulse: fixed frequency,
unchanged setters, positive and bipolar FM, PWM, simultaneous FM/PWM, multi-cycle
frequency and tiny duty. Six ordinary modes are directly comparable. The old
control does not correctly implement every exceptional-mode request, so those
timings are not equal-output speed comparisons.

Intel Core i7-10700, Windows x64, 48 kHz, 2,000,000 samples per pass; three
alternating executions per variant, five timed passes per execution. Table
values are medians of execution medians, **nanoseconds per sample**. Benchmark
processes ran sequentially. The later parallel model compilations were not
performance measurements.

| Build | Fixed Saw: baseline -> combined | Saw FM+PWM: baseline -> combined | Largest ordinary-case increase |
| --- | ---: | ---: | ---: |
| MSVC Release | 2.73 -> 3.25 | 14.63 -> 20.59 | +114.2%, Pulse FM+PWM |
| MSVC Debug | 13.91 -> 14.59 | 97.31 -> 116.48 | +24.4%, Saw PWM |
| Studio Clang 14 Release | 2.63 -> 2.66 | 13.84 -> 21.53 | +132.4%, Square PWM |
| Studio Clang 14 Debug | 11.81 -> 12.47 | 72.46 -> 80.92 | +19.6%, Saw PWM |

MSVC 19.51.36257: `/O2 /fp:fast /MT` or `/Od /fp:fast /MTd`.
Studio Clang 14.0.6: native Windows GNU/libc++ DLL toolchain,
`-O2`/`-O0`, `-ffast-math -static`, C++17. Its bundled static libraries still
import Windows UCRT APIs; this is not an exact MSVC CRT match. The actual DLL
is `F:/klang/build/klang/klang.dll`; identity is retained in the reports.

All benchmark checksums are finite. Correctness is established separately:
[23,093,220 checks per build](OSM-IMPROVED.md#archived-material), all passing on these
four builds. The combined [SynTHX profile](OSM-IMPROVED.md#archived-material)
remains close to or faster than the reflection control; its controls update
per buffer, so it does not conceal the per-sample setter costs above.
`sizeof(Saw)` is 184 bytes versus the reflection control's 144 bytes.

[Full FM/PWM evidence](OSM-IMPROVED.md#archived-material) retains commands, assembly paths,
header/source hashes, every execution and all forty workload results.

## Unchanged examples and sounds

Compiled every example and sound header with the archived and working versions,
then rendered every successful model. No model source was modified.
All four builds use the fast-math settings above. Selected headers are checked
through compiler include/dependency output, and source hashes must remain stable.

| Build | Examples compiling, old -> new | Sound fixtures compiling, old -> new | New compile/runtime regressions |
| --- | ---: | ---: | ---: |
| MSVC Release | 40/53 -> 50/53 | 1/9 -> 3/9 | 0 |
| MSVC Debug | 40/53 -> 50/53 | 1/9 -> 3/9 | 0 |
| Clang 14 Release | 40/53 -> 50/53 | 1/9 -> 4/9 | 0 |
| Clang 14 Debug | 40/53 -> 50/53 | 1/9 -> 4/9 | 0 |

Raw sound-header compilation, before host helpers, improves from 1/7 to 2/7
on all builds. The nine runnable sound cases come from those seven files;
`Motors.h` contains three models. Host helpers are explicitly documented in
[sound-host.h](../../tests/klang/sound-host.h); these are standalone tests,
not claims that the whole UE host has been recreated.

Each render is six seconds at **44.1 and 48 kHz**, seed 12345, unnormalised.
Example synths receive MIDI 48 at time zero, MIDI 60 at two seconds, and note-off
at four seconds. Effects receive 220/997 Hz input for four seconds and a two-second
tail. Buffer size is at most 64, split at event boundaries. Sound controls follow
[sound-render.h](../../tests/klang/sound-render.h). Candidate renders repeat
in a fresh process and must be byte-identical. All successful renders have the
expected length, are finite, and every candidate repeat is identical.

Each build has **82 old/new paired files: 57 sample-identical, 25 changed**.
Two identical files are the silent default `Modulation/Operators.k` outputs;
they establish unchanged defaults, not audible validation of its presets.
Newly compiling models have no renderable old-header counterpart. These fixtures
do not cover every preset/control trajectory or replace listening acceptance.

### Listening acceptance

Chris reviewed all six exported 44.1 kHz old/new pairs (Flanger, Expression and
FM2, MSVC and Studio Clang 14 Release) and reported that they all sound the same.
**Accepted as NIPD: no immediately perceptible difference.** The numerical
differences below are accepted and do not require further investigation for
these reviewed fixtures. Improved accuracy/reduced phase drift is a plausible
explanation, not an established cause of every residual. The independent
oscillator correctness checks support retaining the repaired implementation.

[Listening record](osm-listening-review.json) retains the decision and reviewed
file hashes. Compilation gaps and performance optimisation remain separate work.

### Retained numerical differences and limits

- **MSVC Flanger:** at 44.1 kHz only three samples differ by more than 0.01;
  residual peak is 0.33406, RMS 0.000759. Around the largest difference at
  2.666712 seconds, the old samples include `-0.53805, -0.21849, -0.56271`,
  versus `-0.53813, -0.55255, -0.56267` with the repair: the isolated old
  excursion is gone. The model uses an OSM Triangle to modulate its delay.
  Clang Release residual peak is only 0.000320 at this rate.
- **MSVC Expression:** the three-Saw synth changes more broadly: residual
  peak 0.20209, RMS 0.01287 at 44.1 kHz. Whole-file RMS changes from 0.11724
  to 0.11688. Clang Release residual peak is 0.000094. Chris subsequently
  accepted both compiler pairs as perceptually equivalent, as recorded above.
- **FM2 and other Sine users:** shared phase-increment repairs also change
  these outputs. FM2's largest Clang Release residual is 0.006833 at 44.1 kHz,
  with almost unchanged level. Do not attribute all changes exclusively to OSM.
- Several models exceed unity without a host limiter. Harrier already peaks
  around 11.85 under the old header in this fixture and remains at that level.
  Full peak/RMS/DC/over-unity counts are retained; finite float output does not
  imply safe playback gain or a production mix.

Unnormalised 44.1 kHz float WAV pairs are exported under
`build/osm-promotion/listening/` for Flanger, Expression and FM2, in MSVC and
Clang Release. Each filename states the compiler and baseline/candidate; no gain
matching or alignment is applied. Paths and hashes are in the evidence file.

### Remaining compilation gaps

- `examples/DX7.k` and root `examples/FM.k`: existing function-macro syntax
  mismatch. `examples/Modulation/FM.k` does compile.
- `examples/Subtractive/Filter.k`: existing Envelope-to-param conversion gap.
- Helicopter and Mini: existing `1 - abs(...)` overload ambiguity.
- All three Motors fixtures: existing bank-output-to-mono routing failure.
- Train: existing Control-routing ambiguity on MSVC; now compiles/renders
  with Clang using the explicit host fixture.

These also fail with the old header; they are not introduced by promotion.
The [earlier arithmetic review](../experiments/klang/core/reviews/ARITHMETIC-REVIEW.md) records their background.
No baseline audio comparison is possible for these failed old-header builds.

## Test-host crash repair

The initial broad run caused Windows `case.exe` application-error dialogs.
The standalone example host omitted `Debug::Session`, so debug-writing examples
overran Klang's debug buffer. Compressor, Gain/RM, Gain/Tremolo and root PingPong
failed with **both** headers. This was a test-host error.

The host now creates a debug session for each block, matching the core's buffer
contract. Targeted reruns and all four complete reruns pass without those crashes.
Runners set a process-local Windows error mode inherited by child tests, so any
future access violation is captured as a failing exit code without a desktop
dialog. No global Windows error-reporting setting was changed. The original
failed batch is retained under `build/osm-promotion/models-msvc-release`; only
the corrected `models-v2-*` batches are acceptance evidence.

## Language checks and main build

The precise MSVC language suite passes, including 227 candidate runtime checks;
Clang 14 Release fast-math also passes. Known-failure ambiguity probes are still
reported explicitly, including channel broadcast; their expected-failure passes
do not mean those language issues are fixed.

MSVC `/fp:fast` exposes a rounding-sensitive assertion in the expanded arithmetic
suite. A [small standalone probe](../../tests/klang/fast-rounding.cpp) reproduces
the same behaviour with **both** headers: constant `signal(16777216.f) - 16777217.0`
folds to `-1`, while runtime wrapped and built-in float-narrowing expressions give
zero. Under `/fp:precise` all give zero. This is a compiler-setting limitation,
not a new OSM defect. The assertion remains intact; it has not been weakened to
make the fast suite green. [Probe evidence](OSM-IMPROVED.md#archived-material).

The main CMake Release Kleine build and `--help` pass with the promoted header,
also compiling the included Farnell collection. That existing CMake preset uses
its existing `/MD` configuration; the controlled profiles above use `/MT`.

## Archived reproduction and evidence

The historical commands below use scripts now stored in `osm-improved.zip`.
Restore the archive in a separate checkout before repeating them. For the live
production regression check, run `python tests/klang/check_osm.py --fast`.

From an x64 VS developer shell:

```text
python tests/klang/compare_osm_versions.py --combined --versions control combined --compare-header control build/osm-combined/candidate/headers/control/klang.h --compare-header combined include/klang.h --output build/osm-promotion/repeat-fm
python tests/klang/check_sounds.py --examples --fast --allow-output-changes --output build/osm-promotion/models-v2-msvc-release
python tests/klang/check_sounds.py --examples --fast --debug --allow-output-changes --output build/osm-promotion/models-v2-msvc-debug
```

Use the [Studio DLL launcher setup](OSM-IMPROVED.md#archived-material)
and add `--compiler build/toolchains/klang14/klang.exe --driver gnu` for Clang,
with distinct `clang14-release`/`clang14-debug` output directories. The default
comparison baseline is the archived header; the candidate is `include/klang.h`.
The profile runner defaults should be checked against the retained exact commands
for matching repetition counts. `--source examples/Compressor.k` limits a model
run when investigating a particular fixture.

`python tests/klang/report_osm_promotion.py` collects the four complete model
reports and exports the review WAVs. [Full model evidence](OSM-IMPROVED.md#archived-material)
contains all commands, compile errors, selected-header checks, input hashes,
render metrics, repeat checks and residuals. Generated executables, include
traces, assembly and audio remain under `build/osm-promotion/`.

## Archived material

[Download osm-improved.zip](../../experiments/klang/osm-improved.zip). It contains the complete
OSM investigation reports, measured JSON evidence, prototype fragments,
superseded benchmark/generator scripts, and matching header snapshots. Files
retain their original repository-relative paths. `ARCHIVE-MANIFEST.json` gives
SHA-256 hashes and identifies retired files versus retained dependency snapshots.
`ARCHIVE-README.md` explains restoration; extract separately to avoid overwriting
live work. Current regression tests, working and previous production headers,
and all unfinished experiments remain outside the archive as live files.

Start inside the archive with `experiments/klang/core/OSM-PROMOTION.md`,
`osm-combined-results.json`, `osm-fm-pwm-results.json`, and
`osm-promotion-model-results.json`. The former `OSM-STATUS.md` is historical.
The current [listening acceptance](osm-listening-review.json) and
[production manifest](working-production.json) remain alongside this report.
