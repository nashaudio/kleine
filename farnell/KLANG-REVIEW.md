# Klang review backlog

Last consolidated: 15 September 2026, after completing the Artificial Sounds extended trial (24–28). This records language friction, DSP gaps, reusable candidates and integration work for review. It does not authorise changes to core `include/klang.h`.

Context: [AGENTS.md](../AGENTS.md) prioritises readable Klang models, `param`/`relative` setters, immediate sample/buffer processing, faithful audible results before optimisation, and separate prototypes for core-language changes. Work on [pd.h](../include/klang/pd.h) is authorised directly. [Coverage](COVERAGE.md) identifies model dependencies; [primitive usage](PD-PRIMITIVES.md) identifies common building blocks. Existing [trial notes](klang/README.md), [primitive results](../tests/pd/README.md) and [level findings](audio/comparisons/reference-levels.md) provide the initial evidence.

## Priorities and effort

![Complete](status/complete.svg) · ![Current](status/current.svg) · ![Problem](status/problem.svg) · ![Not started](status/not-started.svg) · ![—](status/none.svg)

**Review items resolved:** ![0% (0/15) complete; 9 current, 3 problematic, 3 not started](status/progress-0-9-3-3.svg)

Three confirmed issues are red; existing work awaiting consolidation/review is amber; unstarted investigations are white. None of the 15 review items is closed. K-014 remains amber because downstream review is pending despite the completed project build fix. The [Artificial Sounds evidence](audio/artificial-sounds.md) adds block-timing, delay and detector findings to K-006–K-009, and a reproduced stream-extraction collision in K-015.

P0 means a current blocker or serious correctness failure; none is established here. P1 means address before the next affected models or before teaching the affected syntax. P2 means planned consolidation, broader validation or usability. P3 means later optimisation/polish. Estimates are active engineering effort for one contributor, including focused verification, not elapsed commitments. Historical PD research and integration review can add time.

| ID | Priority | State | Review item | Estimated effort |
| --- | --- | --- | --- | --- |
| K-001 | P1 | ![Problem](status/problem.svg) Confirmed source restriction; workaround in models | Accept Klang values in `Controls::set` | 0.5–1 day |
| K-002 | P1 | ![Problem](status/problem.svg) Encountered in trial; worked around | Ambiguous routing of the `Output&` returned by inline object calls | 1–3 days |
| K-003 | P1 | ![Not started](status/not-started.svg) User-reported family; isolate examples | Literal and conversion ergonomics (`0`, `0.f`, helper types, mixed expressions) | 0.5–1 day to catalogue; 1–3 days per coherent fix |
| K-004 | P2 | ![Current](status/current.svg) Confirmed API constraint | Setter dispatch accepts only `param`/`relative` overrides | 0.5 day for guidance/diagnostics; 2–4 days if redesign needed |
| K-005 | P1 | ![Current](status/current.svg) Implementation exists; parity unverified | Audit `pd::vcf` before wind/rain and resonant models depend on it | 1–3 days |
| K-006 | P1 | ![Not started](status/not-started.svg) Source-level concern; targeted test pending | Sample-rate changes and cached coefficients/state | 1–2 days for contract and affected primitives; 1–3 more for models |
| K-007 | P1 when needed | ![Current](status/current.svg) Block-64 line port tested; queued ramps pending | Reusable ramps, delayed segments and event timing | 2–4 days |
| K-008 | P1 when needed | ![Current](status/current.svg) Bell/police helpers tested; general port pending | PD-compatible delay reads, interpolation and feedback ordering | 2–5 days |
| K-009 | P1 for faithful models | ![Current](status/current.svg) Model-specific handling exists | Explicit event/block/summing contract in the renderer and ports | 1–3 days |
| K-010 | P2 | ![Current](status/current.svg) Existing helper; integration candidate | Review `Additive` and partial/envelope abstractions | 1–2 days |
| K-011 | P2 | ![Current](status/current.svg) Primitive sequence fixed; integration review open | Noise seeding, reset and compatibility for existing game models | 0.5–2 days |
| K-012 | P2 | ![Current](status/current.svg) Scripts exist; CI registration absent | Lightweight syntax and audio regression entry points | 1–2 days |
| K-013 | P3 | ![Not started](status/not-started.svg) Measurements available; optimisation deferred | Separate DSP cost from host/file/startup overhead | 1–2 days for a focused benchmark; optimisation estimated separately |
| K-014 | P2 | ![Current](status/current.svg) Downstream review pending; project build fix complete | Windows `min`/`max` macro collision for downstream header consumers | 0.5 day |
| K-015 | P1 | ![Problem](status/problem.svg) Reproduced on MSVC; explicit-call workaround | Klang routing captures standard stream extraction | 1–2 days |

## Language and learner-facing issues

<a id="k-001"></a>

### K-001 — Klang values in `Controls::set`

[Controls::set in klang.h](../include/klang.h) applies `std::is_arithmetic` to every argument before converting it to `float`. `param`, `signal` and unit helpers are class types, so an otherwise sensible `controls.set(value, dialHz, smoothBusy)` is rejected. [PhoneTones::set](klang/Artificial%20Sounds/Phone%20Tones/phonetones.k) consequently calls `controls.set(float(value), float(dialHz), float(smoothBusy))`; [TelephoneBell::ring](klang/Idiophonics/Telephone%20Bell/telephonebell.k) also converts stored control values.

Desired result: accept intended Klang scalar values without casts while retaining useful diagnostics for unsupported arguments. Prototype an explicit accepted-type/conversion policy; do not simply accept any user-defined type that happens to convert. Check control values, `param`, `Frequency`, scalar literals, unsupported inputs and overload selection. These casts currently belong to the workaround, not the preferred teaching idiom.

<a id="k-002"></a>

### K-002 — Inline call result routing

The [trial compatibility notes](klang/README.md#compatibility-notes) record an MSVC ambiguity when routing the `Output&` returned by `osc(frequency)` directly with `>>`. [Primitive::process](../tests/pd/primitive.h) materialises the carrier as a `signal` before routing it. [Generic::Generator::operator(), Generic::Output and the free routing operators](../include/klang.h) are the relevant overload sets.

Desired result: idiomatic `osc(frequency) >> out` and modifier chains compile without an intermediate solely for overload resolution. First retain a minimal reproducer on the current header/compiler. Then verify processing occurs exactly once, in the intended order, for const/non-const sources and chained expressions; a compilation-only fix could accidentally change the sound. Do not assume every `Output` conversion is broken: the current materialised FM path passes its audio fixture.

<a id="k-015"></a>

### K-015 — Standard stream extraction and routing overloads

While adding TSV events in [render.h](render.h), ordinary `std::istringstream` extraction inside the Klang namespace context selected a Klang free `operator>>` template. The resulting MSVC C2678 diagnostic came from its routing implementation, with `std::string` as a destination. The encountered expression was `fields >> item.frame >> item.message`, where `fields` is an `std::istringstream`, `frame` is `uint64_t`, and `message` is `std::string`. The successful workaround explicitly calls `fields.operator>>(item.frame)` and `std::operator>>(fields, item.message)`; numeric values also use the member extraction. The Release and Debug builds and all scripted renders pass with that workaround on MSVC 19.51.

Desired result: ordinary stream syntax continues to work when Klang routing is in scope. Reproduce the overload selection in a small separate prototype, then constrain the routing templates to intended audio source/destination types without accepting unrelated standard-library expressions. Verify string and numeric extraction as well as generator/modifier chains, lvalue/temporary outputs and exactly-once audio processing. This is a core-language review item, not a change to the stream API or a learner-facing idiom to recommend.

<a id="k-003"></a>

### K-003 — Literals and conversions

Chris reports ambiguous or missing conversions that require explicit casts or `0.f` instead of `0`. Current [PhoneTones](klang/Artificial%20Sounds/Phone%20Tones/phonetones.k), [TelephoneBell](klang/Idiophonics/Telephone%20Bell/telephonebell.k), and [motor models](../sounds/Motors.h) contain mixed scalar/helper expressions and explicit conversions suitable for collecting cases. Their presence alone does not prove each cast is necessary. `std::clamp`/`std::max` template deduction, unit conversion, conditional expressions, Klang routing, and setter dispatch must be distinguished.

Record the smallest failing expression, intended type/units, actual compiler diagnostic, successful workaround, and header/compiler revision. Compare `0`, `0.f`, `0.0`, `param(0)`, `signal(0)`, and relevant unit helpers. Aim for ordinary numeric literals and unambiguous unit-aware expressions without broad conversions that weaken type checking. This item is deliberately marked reported rather than a newly reproduced bug; K-001 and K-002 are the concrete initial cases.

<a id="k-004"></a>

### K-004 — Setter dispatch and diagnostics

[Generic::Generator and Modifier](../include/klang.h) dispatch through a fixed family of virtual `set(param, ...)` / `set(..., relative)` signatures. Chris's instruction and [AGENTS.md](../AGENTS.md) therefore require only these argument types in overrides; a custom `set(int)` or `set(bool)` can hide rather than override the intended path. Current PD/model setters follow that rule, while ordinary methods such as `dial(int)` and `ring(bool)` need not be setters.

Keep this distinction in teaching examples. Prefer `override` where a matching virtual signature exists. Investigate clearer compile-time diagnostics or an explicit extension mechanism before redesigning dispatch. Verify direct `.set(...)` and inline `object(...)` behaviour agree. A redesign is optional review work, not a prerequisite for the next sound.

## DSP, lifecycle and host contracts

<a id="k-005"></a>

### K-005 — `pd::vcf` validation

[pd.h](../include/klang/pd.h) has a `vcf` implementation with two state/output components, coefficient caching and approximations. Unlike `osc`, `hip`, `noise`, `lop`, and `bpf`, it has no retained isolated comparison in [tests/pd](../tests/pd/README.md). Its `lpf`/`bpf` member names need checking against PD's real/imaginary outlet semantics; presence of code is not established parity.

Before using it as a reference, add impulse and noise fixtures, both outlets, frequency modulation, low/zero/high Q, near-Nyquist frequency and parameter changes. Compare the actual PD source revision, gain and denormal treatment. Remove unused coefficient calculations only after the correct algorithm is established. Wind, rain and other resonant models make this an early dependency.

<a id="k-006"></a>

### K-006 — Sample rate, preparation and reset

[pd::bpf and pd::vcf](../include/klang/pd.h) cache coefficients when frequency/Q change, so a changed `klang::fs` with unchanged parameters warrants a targeted test. `hip` also calculates its coefficient in `set`; `lop` recalculates when set. [TelephoneBell::Casing](klang/Idiophonics/Telephone%20Bell/telephonebell.k) allocates lengths and sets frequencies during construction. The model documentation consequently requires construction after the sample rate is set and limits established fidelity to 48 kHz/block 64.

The Artificial Sounds models now pass 39 fixtures at each of 48 and 44.1 kHz, constructing fresh instances after setting the rate. That establishes two fixed-rate configurations, not a live rate-change contract. The police delay's 165 ms conversion exposed a half-sample rounding difference at 44.1 kHz: retaining PD's `float(fs) / 1000 * milliseconds` operation order fixed the impulse comparison.

Define whether runtime rate changes are supported or require reconstruction. If supported, refresh affected coefficients and buffer sizes through the host lifecycle, without allocations in the audio callback. Keep Chris's rule: do not manually call `prepare()` from constructors or `set()`; the processor calls it at each buffer. Check reset/retrigger separately from rate changes, including delay contents and oscillator phase. This is a concrete audit target, not a claim that the current fixed-rate trials fail.

<a id="k-007"></a>

### K-007 — Ramp and event helpers

[TelephoneBell::Decay](klang/Idiophonics/Telephone%20Bell/telephonebell.k) is a finite linear decay with a tested first-sample convention. [vline-decay.pd](../tests/pd/vline-decay.pd) tests that restricted pattern. The new `pd::line` implements `line~` for a fixed 64-sample block, including immediate values, retargeting and stop; [line.pd](../tests/pd/line.pd) matches sample-for-sample at both rates. It serves DTMF and the alarm models. A requested 1 ms DTMF fade lasts one block, so replacing it with a sample-duration ramp changes the reference.

Arbitrary `vline~` queues, delayed ramps, cancellation, overlapping messages, configurable block sizes and message-rate `line` remain separate work.

Use the next model's requirements to select the smallest reusable object. Inspect [Klang Envelope/Ramp](../include/klang.h) before adding a duplicate. Keep the helper nested until there is a second independent use. Test event boundaries and re-triggering at 48/44.1 kHz and across buffers; preserve PD quirks only where the reference requires them.

<a id="k-008"></a>

### K-008 — Delay semantics

[TelephoneBell::Delay and Casing](klang/Idiophonics/Telephone%20Bell/telephonebell.k) explicitly separate read/write and retain measured 64/128-sample routing delays. This is a useful prototype, not a complete `delread~`/`vd~`/`delwrite~` implementation. [Klang Delay](../include/klang.h) already supplies delay facilities; inspect its interpolation and update order before introducing another general object.

[Police::Environment](klang/Artificial%20Sounds/Police/police.k) adds a second use: three fixed feedback taps, each with one extra 64-sample routing block. Its isolated impulse fixture is sample-identical at both rates. The model keeps its delay nested; this corroborates the importance of scheduling without establishing a reusable general delay contract.

Needed evidence: integer/fractional delay, minimum read delay, allocation size, interpolation kernel, write/read ordering, feedback, block boundaries and rate changes. Retain the bell fixture while generalising. Delay timing is part of the timbre in feedback models, so replacing it with an apparently equivalent tap is a correctness change.

<a id="k-009"></a>

### K-009 — Events, blocks and immediate processing

The [renderer](render.h), [Python wrapper generator](../tools/render_farnell.py), and [model notes](klang/README.md) already encode pulse-event rounding and the bell's PD scheduling. Klang processes immediately; PD schedules a graph. Fan-out from one stored sample, summing multiple connections, message ordering and feedback therefore need explicit translations rather than a presumed graph runtime.

[render_artificial.py](../tools/render_artificial.py) now supplies shared TSV events on a 64-sample grid. PD message timestamps sit a quarter-sample inside the intended block to avoid floating-point boundary ambiguity. Metro start fires immediately; stopping holds the current gate, rather than muting. Ringback's nominal whole-second events also require block rounding at 44.1 kHz. `pd::env` publishes its new control value after its source block, and the phone sequencer consumes a detected digit on the next block.

The decoder trial also exposed a processing contract worth documenting: discarding `in >> detector` does not evaluate that route. [DTMFTones::Decoder](klang/Artificial%20Sounds/DTMF%20Tones/dtmftones.k) materialises it as a `signal` to process each detector once before reading its state. This is an immediate/deferred expression boundary, not a newly established compiler defect. Consider teaching guidance or diagnostics for discarded routes; preserve normal lazy routing semantics.

The [Pedestrians block API paper](PD-BLOCK.md) now compares developer-owned/inherited clocks and automatic `block()` events, including a self-disabling default and variable sizes. Its [separate C++17 probes](../experiments/klang/pd-block/README.md) preserve audio under different host partitions and demonstrate that a buffer-only override misses nested signal evaluation. An inherited explicit action works on the current core; preserving `process()` with automatic events everywhere needs a common evaluation hook, or a different sample-method spelling. No core change or production API promotion is implied.

Document a small host contract for event timestamps, control updates, `prepare`, fixed-rate construction, tails and reset. When shared by a second model, factor event utilities out of the render case switches. Verify the pulse and bell fixtures remain stable. This is not a proposal to build a complete PD interpreter or force a general scheduler into every model.

## Reuse, verification and performance

<a id="k-010"></a>

### K-010 — Additive and partial-group candidates

[include/klang/utils.h](../include/klang/utils.h) contains `Additive`, partial collections and dB helpers. [TelephoneBell::Bell::Group](klang/Idiophonics/Telephone%20Bell/telephonebell.k) contains tuned PD oscillators and a shared envelope. They overlap conceptually but differ in phase, frequency, amplitude and envelope requirements.

Audit helper conversions, partial count/assignment, units and initialisation; compare the minimal APIs needed by chapter 17 and the bell. Promote a reusable object only when it improves both models and retains readable signal flow. The [block API experiments](../experiments/klang/pd-block/README.md) are a separate timing study, not an Additive prototype. The earlier PD reference experiment is no longer authoritative; do not recreate an obsolete `PdReference.h` merely to populate the backlog.

<a id="k-011"></a>

### K-011 — Noise seeds and existing-model compatibility

[pd::noise](../include/klang/pd.h) now uses unsigned wrapping arithmetic, outputs before advancing, and supports explicit seeds. [Retained tests](../tests/pd/results.json) establish seeded parity. The default construction seed remains shared state, so instance order influences the resulting sequence. [Model compatibility notes](klang/README.md#compatibility-notes) record that existing users such as [Harrier](../sounds/Harrier.h) receive a changed sequence.

Document explicit seeds for reference fixtures and an intentional seed policy for multiple game actors. Add reset/reconstruction and repeated-instance cases if required. Compare [ToyBoatEngine/FourStrokeEngine](../sounds/Motors.h), [Helicopter](../sounds/Helicopter.h), and Harrier before claiming parity with their Farnell references; current adaptations are useful starting points, not already measured equivalents.

<a id="k-012"></a>

### K-012 — Focused regression entry points

[CMakeLists.txt](../CMakeLists.txt) does not register CTest cases. The meaningful existing checks live in [check_pd_primitives.py](../tools/check_pd_primitives.py), [render_farnell.py](../tools/render_farnell.py), and [check_reference_levels.py](../tools/check_reference_levels.py). Existing [Klang tests](../include/klang/tests.h) are not currently integrated into this project's CTest path.

The extended trial adds [render_artificial.py](../tools/render_artificial.py): 39 paired fixtures at each of two rates, with a residual/level failure gate, finite/non-silent checks, and explicit reconstructed references where upstream external objects are missing. Primitive checks now cover 40 cases. [package_artificial.py](../tools/package_artificial.py) validates retained listening WAVs and records source/audio hashes. Release and Debug builds pass; CI registration and cross-compiler coverage remain open.

Add a convenient, optional entry point for the retained checks and small syntax regressions when fixing K-001–K-004. Separate checks requiring PD/Python audio dependencies from a normal compiler build. Avoid tests that merely mirror implementation; retain cases for actual overload failures, processing order, state transitions and audio residuals. Confirm compiler coverage before claiming the MSVC workarounds are portable.

<a id="k-013"></a>

### K-013 — DSP cost measurements and later optimisation

[AGENTS.md](../AGENTS.md) explicitly puts faithful working sound before optimisation. Existing [audio manifests](audio/README.md) and render tools provide process-level timing/memory context. These include startup and output overhead and are not pure per-node DSP timings.

Use a warmed renderer with identical sample rate, voice count, duration, controls and file-I/O policy. Report CPU seconds per rendered audio second and memory separately for process and incremental voices. Optimise hotspots only when measured: coefficient updates, delay allocation, oscillator tables and per-sample virtual dispatch are candidates, not established bottlenecks. Do not impose a performance gate on the next reference model.

<a id="k-014"></a>

### K-014 — Windows header macros

Including the engine before [pd.h](../include/klang/pd.h) previously exposed a Windows `min`/`max` macro collision. [CMakeLists.txt](../CMakeLists.txt) now defines `NOMINMAX` for Windows, and the project builds with that fix. Review whether public header usage needs documentation or defensive syntax so consumers outside Kleine do not rediscover it. Preserve the existing build fix; no additional platform change is required for the current trials.

## Consolidation workflow

During implementation, capture new friction in the relevant model's notes or a small tracked note beside its prototype under `experiments/klang`. Include a stable K-ID where one exists, source location, minimal expression/patch, compiler or PD revision, observation, workaround, and effect on the model. Keep generated logs, repro binaries and temporary audio under `build/`.

Update this file when requested, when completing a practical series, or when a finding blocks the next agreed model. Merge repeated cases, distinguish confirmed defects from hypotheses and deliberate design constraints, revise effort estimates after a reproducer exists, and link the actual prototype and validation evidence. Keep resolved entries briefly with their resolution rather than losing the reason a workaround existed. Core-language integration still follows the review boundary in AGENTS.md; `pd.h` can be developed directly.
