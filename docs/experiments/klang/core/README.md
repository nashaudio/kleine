# Klang core replacement: active numeric and compatibility work

The generic float/double foundation, `param64`/`signal64`, Time and unit-literal
integration remain unfinished. This directory and its compatibility evidence
are retained for that work. Reports are in [docs](reviews/ARITHMETIC-REVIEW.md).

The local [klang.h](../../../../experiments/klang/core/klang.h) is an earlier experimental snapshot, not the working
production header. Its bytes have been preserved during cleanup. Start any next
numeric-refactor branch from the current production header after reviewing the
remaining compatibility issues; no automatic rebase was made here.

Completed OSM experiments have been archived in
[osm-improved.zip](../../../../experiments/klang/osm-improved.zip). The accepted result is documented
in [docs/klang](../../../klang/OSM-IMPROVED.md). The previous production
[baseline](../../../../experiments/klang/core/baselines/production-2026-09-17/klang.h) stays live for regression tests.

## Recommendations before refactoring

### Compatibility contract

- Keep public `signal` and `param` float-backed, with their current size,
  inheritance relationship, setter syntax and established numerical behaviour.
- Preserve the float path's arithmetic for existing expressions, including plain
  double literals. Current `signal * double` narrows the scalar before arithmetic;
  changing that to double arithmetic is a behavioural change even if it compiles.
- Preserve exactly-once signal evaluation, const reads of cached outputs, routing
  order and feedback semantics. Compilation alone cannot establish these.
- Treat source compatibility as required. Rebuilding binaries is expected;
  preserving the ABI of previously compiled plugins is a separate requirement.
  Do not claim aliases and old concrete types have identical C++ type identities.
- Keep the models unchanged during compatibility work. Report genuine host
  dependencies, obsolete source APIs and core defects separately. Fix suitable
  core compatibility gaps in this candidate rather than weakening checks.

### Numeric foundation

Prototype `Generic::signal<T>` and `Generic::param<T>` with public `signal`,
`param`, `signal64` and `param64`. Start by moving the float implementation while
retaining its behaviour, then add double specialisations. Choose aliases versus
thin public wrappers based on compatibility evidence, especially operator lookup
and forward declarations; do not force an alias-only design upfront.

The double path must preserve precision through constructors, arithmetic,
relative values, conversion results and generic input/output. In particular,
`Generic::Generator<SIGNAL>` currently hardcodes `set(param)` signatures:
`Generator<signal64>` would otherwise still narrow its parameters. Check these
contracts explicitly before describing a processor as double-precision.

Preserve ordinary float/scalar semantics. Define new float-wrapper/double-wrapper
expressions to widen deliberately, with explicit narrowing at new precision
boundaries. Avoid globally loosening implicit conversions, which risks worsening
the existing ambiguities. Scalar templates should not add virtual dispatch or
allocation. Generalising every DSP algorithm and host buffer to double is a
separate audit from introducing double-backed scalar types.

### Literals and Time

Integrate the tested `_Hz`/`_kHz` forms in `klang::literals`. Keep suffix import
explicit and preserve existing plain-number syntax. Other domain suffixes need
their matching type and conversion contract, including signed expressions such
as `-6_dB`; a literal's return type alone does not preserve units in arithmetic.

Time should use `param64`, including `_ms`, `_s` and fractional `_samples`, with
double arithmetic and conversion results throughout. Retain the discussed
distinction between `Time` (default milliseconds) and `Time::samples`:
`Time offset = 0.25_samples` converts at the current sample rate, whereas
`Time::samples offset = 0.25_samples` retains its sample basis. Keep exact sample
counters as 64-bit integers. The older [syntax probe](../time-units/README.md)
is still float-backed and is not the desired implementation.

The optional [timer scheduler](../scheduling/README.md) is a separate integration
step. This replacement should make precise Time values possible without silently
bundling lifecycle and scheduling changes into the numeric refactor.

## Initial compatibility evidence

Follow-up: [the example/branch review](reviews/EXAMPLE-REVIEW.md) establishes that all
53 examples match Klang main. Ten example failures are isolated to four free
arithmetic overloads commented out in the current header; their restoration
passes those ten cases in a scratch copy. Table and setter-conversion issues
remain separate. That review predates the candidate repair below.

MSVC 19.51.36257 and clang-cl 22.1.3, C++17, Windows x64, `/O2 /fp:precise`.
Each source is included unchanged in its own generated translation unit, with
explicit instantiation of selected top-level model constructors/destructors.
Separate units avoid intentional class-name collisions between examples.

| Collection | Files | MSVC baseline | MSVC copied candidate | Clang baseline |
| --- | ---: | ---: | ---: | ---: |
| examples | 53 | 40 | 40 | 40 |
| sounds | 7 | 1 | 1 | 1 |
| farnell/klang | 20 | 20 | 20 | 18 |
| **Total** | **80** | **61** | **61** | **59** |

The MSVC candidate and baseline have identical per-file pass/fail outcomes and
there are zero new failures. Header selection and unchanged source/header hashes
were verified. Clang was run against the baseline; the candidate is still the
same header bytes. [baseline.json](../../../../experiments/klang/core/baseline.json) retains compiler provenance,
source hashes, per-file outcomes and first diagnostics. Full compiler logs and
objects remain under `build/core-compatibility`.

The existing failures include output/rvalue arithmetic restrictions, old
`FUNCTION(TYPE)` macro usage, setter-interface mismatches, a missing `Phasor`
name and Unreal `FMath` dependencies in some sound headers. The latter are host
dependencies, not evidence that a standalone scalar implementation is broken.
Do not add Unreal names to Klang merely to manufacture a green test result.

These are compile checks, not linking or audio acceptance. Selected root-model
instantiation exercises their constructors and virtual implementations, but does
not instantiate every possible template argument or run all control paths.

## Reproduce

From an x64 Visual Studio developer command prompt, at the repository root:

```text
python experiments/klang/core/check_compatibility.py
python experiments/klang/core/check_compatibility.py --header baseline --compiler clang-cl.exe --output build/core-compatibility/clang
```

Supply the full path to `clang-cl.exe` if it is not on PATH. The default run tests
both headers with MSVC, using four compiler processes. Sources are discovered
from all three collections; a new sound or Farnell file needs an explicit root
type entry rather than being silently skipped. `examples` uses the file's stem
as its root model type. `examples/` is also an include directory for Train's
existing `Reverb.k` dependency.

The default exit gate rejects new pass-to-fail regressions, wrong header selection
or source changes during the run. Existing baseline failures remain visible in
the report. Add `--require-all` for an all-files-pass gate; it currently fails.
Keep baseline failure triage explicit as the candidate improves.

## Arithmetic restoration and regression tests

The [arithmetic and game-sound review](reviews/ARITHMETIC-REVIEW.md) records the first
candidate change and [retained results](../../../../experiments/klang/core/arithmetic-review.json). Merely restoring
the four upstream overloads breaks Harrier's `abs(...) * 2`; the revised overloads
accept scalar temporaries while leaving lvalue arithmetic and float-convertible
function wrappers on their established paths.

| Collection | MSVC baseline → candidate | Clang baseline → candidate |
| --- | ---: | ---: |
| examples | 40 → 50 / 53 | 40 → 50 / 53 |
| sounds, without host helpers | 1 → 2 / 7 | 1 → 2 / 7 |
| farnell/klang | 20 → 20 / 20 | 18 → 18 / 20 |
| **Total** | **61 → 72 / 80** | **59 → 70 / 80** |

No pass-to-fail regressions. Kleine also builds, links and runs `--help` with the
candidate on MSVC. [Language tests](../../../tests/klang/README.md) check values,
types, evaluation counts, routing order, cached reads and setter dispatch; they
also isolate known compilation failures. Sound fixtures compare the local,
candidate and pinned upstream headers at 44.1/48 kHz. The three sound fixtures
that run on candidate MSVC are sample-identical to their available references.

Chris identifies the game host as UE 5.8.1, VS 2026 with Unreal Build Tool's
settings. These checks use VS 2026 MSVC 19.51 and Clang 22.1.3 in standalone mode;
they do not reproduce an actual UE build or claim to know its full command line.

## Replacement sequence and acceptance

The [supplied UE 0.7.9 review](reviews/GAME-HEADER-REVIEW.md) adds a third reference:
all nine sound roots run on standalone MSVC with explicit host dependencies.
Its core API changes resolve several earlier blockers, but six examples that
the candidate accepts fail with 0.7.9, and Clang exposes further ambiguities.
The completed OSM repair is now in the working header; see the
[promotion and listening acceptance](../../../klang/OSM-IMPROVED.md).
Historical OSM prototypes and benchmarks are in [osm-improved.zip](../../../../experiments/klang/osm-improved.zip).
Reconcile the remaining API changes selectively before the numeric refactor.

1. Establish and retain the baseline above; classify existing failures and host
   dependencies. Add focused tests for the existing ambiguity cases.
2. Refactor the float foundation, checking unchanged callers before adding new
   functionality. Build and link Kleine and its supporting headers as well.
3. Add the double path and test precision beyond float's exact sample-count range,
   fractional samples, relative setters and mixed-precision arithmetic.
4. Integrate domain literals and double-backed Time; test units, conversions,
   signed literals, overload selection and cross-translation-unit linkage.
5. Compare deterministic float renders at 44.1/48 kHz, check evaluation counts,
   object layout and allocation, and compare representative float-path CPU cost.
   Separate numerical/audio changes from compiler acceptance.
6. Review the candidate diff and evidence for promotion. Replace the core when
   the compatibility, behaviour and precision gates pass; migrate examples to
   improved literal syntax afterwards so they remain independent old-code tests.
