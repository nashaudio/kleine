# UE 0.7.9 integration — Klang 0.7.10

17 September 2026. Integrated the reviewed additions from the supplied UE 0.7.9
header into the working header. The working version is now **0.7.10**. Klang's
corrected OSM and the reviewed compatibility choices supersede the supplied
header where documented below.

## Added

- `basic::Phasor`: the UE ramp based on Basic Saw; `optimised::Phasor` now uses
  the hardened integer phase while retaining the same aliased 0-to-1 contract.
- `interleaved::buffer<N>` and `Stereo::interleaved::buffer`, with interleaved
  processing in Stereo Effect and its current Sound subclass.
- Smaller corrections: inline `Controls::set` for multi-translation-unit
  linking; absolute-value buffer level measurement; DCF's `param` setter
  override; GraphPtr's helper renamed `init` to avoid the UE `check` macro;
  qualified event forwarding and the unsigned-long noise mask literal.

The buffer port corrects the UE frame-index mask and compound-assignment return
values. Copies borrow storage, matching the existing mono buffer convention,
and cannot free the original owner's allocation. Empty buffers are supported.
Size/offset values count frames, not individual channel samples.

The prototype and reproducible preparation script remain under
[experiments/klang/ue-integration](../experiments/klang/ue-integration/README.md).
The pre-addition header is retained as `include/klang.h` inside
[osm-improved.zip](../../experiments/klang/osm-improved.zip); the validation copy is under
`build/ue-integration/baseline/klang.h`.

## Validation

| Check | MSVC Release | MSVC Debug | Studio Clang 14 Release | Studio Clang 14 Debug |
| --- | ---: | ---: | ---: | ---: |
| Buffer/Phasor/setter/link contracts | 1,105,295 pass | 1,105,295 pass | 1,105,295 pass | 1,105,295 pass |
| Examples + sounds, paired renders | 106 identical | — | 108 identical | — |

Model comparisons use unchanged sources at both 44.1 and 48 kHz, six seconds,
fresh-process repeat checks, no alignment or level adjustment. No compilation
or rendering regressions were found. Existing compilation blockers remain:
50/53 examples compile on both compilers; 3/9 sound roots compile under MSVC
and 4/9 under Clang with the explicitly documented test-host dependencies.
These are standalone tests, not an Unreal build.

The retained OSM suite passed all **23,093,220 checks** under MSVC Release
`/fp:fast`. New contracts used MSVC `/MT` Release, `/MTd` Debug, and `/fp:fast`;
Studio's GNU-driver Clang used `-static -ffast-math`, with `-O2` / `-O0`.

After promotion, a mono Flanger and stereo PingPong rerun through the complete
render/package command produced four identical pairs. The standalone
Kleine/Farnell application compiled and linked in Debug with `/MTd /fp:fast /Z7`.
The existing CMake Debug directory encountered a PDB update error; the successful
build used independent outputs under `build/ue-integration/` and embedded debug
information, leaving that build directory's settings unchanged.

The generated inspection reports and WAVs were listening-time scratch outputs
and were removed after acceptance. Compact result manifests are retained at
their original repository-relative paths inside
[`klang-UE-0.7.9-integration.zip`](../../experiments/klang/klang-UE-0.7.9-integration.zip).

The reusable [test workflow](../tests/klang/README.md#render-compare-and-inspect-examplessounds)
exports float WAVs, 10 ms RMS/peak PNGs, Hann sonograms, and HTML/JSON reports.
Exact pairs skip redundant plots by default. Six independent report-policy
tests cover stereo differences, invalid audio, lengths, tolerances and level
bins. Both example plot types were visually inspected.

## Review decisions

1. **Sound's role — resolved and promoted:** retain `Sound : Effect` so models can receive
   modulation, sidechains and reference audio. The subsequent
   [HAS_KLANG_DEBUG integration](../experiments/klang/debug-guard/README.md)
   independently disables capture outside Studio by default, in either build
   configuration. It preserves the existing Studio debug implementation.
2. **Arithmetic and Function conversions — resolved and promoted:** the
   [function-flow review](../experiments/klang/function-flow/README.md)
   replaces the global math Function objects and macro/`std::klang` workaround
   with standard-backed stateless unary maths overloads plus exact
   `float(float)` signal flow. Mini's `FMath::Tanh` dependency is also removed.
   It resolves the Function-left ambiguity without adopting UE's broad
   overloads or removing `Function::operator float()`. Generic Function
   redesign remains deferred. Built-in arithmetic values now have one
   constrained route to `signal`, with explicit float narrowing at that
   boundary; this keeps Motors' original double-valued shaping expression
   valid on both MSVC and Clang without admitting arbitrary convertible objects.
3. **Channel and Bank reduction — resolved and promoted:** constructing
   `signals<N>` from one scalar or mono signal broadcasts to every channel.
   Channel reduction remains explicit: `sum()` adds all channels, while
   `average()` and `mono()` return their arithmetic mean. Routing a `Bank`
   directly to mono processes each item once and sums its parallel outputs;
   attenuation remains an explicit model choice. MSVC and Studio Clang pass
   the 250-check working-header language contract and the 1,105,295-check UE
   integration contract in Release and Debug. All three Motors roots compile,
   but Car/FourStrokeEngine are subsequently excluded from acceptance because
   of their model-local audio-rate delay jitter; ToyBoatEngine remains. The reconstructed Clang
   Train also compiles; its corrected mono-object broadcast feeds both stereo
   channels, so it deliberately differs from the old `{mono,0}` constructor
   defect and remains a listening-review item.
4. **Control routing — resolved and promoted:** scalar/signal assignment is
   constrained and clamped, so `controls[i] = value` is supported. Removing
   Control's unconstrained member routing leaves the existing global router as
   the single feed-forward path and preserves mutable processing, const cached
   reads and chain order. `ControlMap = Control` binds the map, while assigning
   scalar or signal values writes through an existing mapping.
   The 17 focused checks pass on MSVC and Clang, Train now compiles on both, and
   Compressor remains sample-identical. See the
   [Control routing review](../experiments/klang/control-routing/README.md).
5. **Basic and optimised Phase/Phasor — resolved and promoted:** `Phase` now
   wraps negative and multi-cycle radian or wavetable steps, and every Basic
   waveform applies relative phase through the same readable helper.
   `optimised::Phasor` uses the existing integer `Fast::Phase` and signed
   `Fast::Increment` rather than inheriting Basic's float path. Both remain
   deliberately aliased; Phasor is generally a control ramp, so band-limiting
   would change its intended signal. Focused results and benchmarks are in the
   [basic phase review](../experiments/klang/basic-phase/README.md).
6. **Other UE details — resolved and promoted:** MSVC suppressions 4587, 4263, 4264 and 4996
   are consolidated in one scoped block. `KLANG_STRICT` defaults to `0`; define
   it as `1` before inclusion to reactivate those warnings. The broken
   `FUNCTION` macro is removed: `Table` accepts value-returning and
   `Result`-writing callables directly, and DX7/FM now use ordinary lambdas on
   MSVC and Clang. The UE coupled RMS spelling is numerically identical across
   48,000 focused samples, so the clearer `(in * in) >> ar >> sqrt >> out`
   signal flow is retained.

The final warning, table and follower evidence is recorded in the
[UE details review](../experiments/klang/ue-details/README.md).

7. **Generator-to-param snapshots — resolved and promoted:** constrained
   `param` construction preserves Klang's existing mutable-processes/const-cached
   conversion contract. Generator and Modifier inline setters perfect-forward
   arguments, so `osc >> filter(env, 10) >> out` processes the live envelope
   exactly once. All 53 examples now compile on MSVC and Clang. See the
   [generator-param review](../experiments/klang/generator-param/README.md).

Interleaved storage currently uses the existing `signals<N>` layout (two or
more channels); mono uses `klang::buffer`. Synth's separate note renderer has
not acquired an interleaved rendering API.
