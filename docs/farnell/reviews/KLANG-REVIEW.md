# Klang review backlog

Last consolidated: 15 September 2026, after completing the Idiophonics trial (29-33). This records language friction, DSP gaps, reusable candidates and integration work for review. It does not authorise changes to core `include/klang.h`.

Context: [AGENTS.md](../../../AGENTS.md) prioritises readable Klang models, `param`/`relative` setters, immediate sample/buffer processing, faithful audible results before optimisation, and separate prototypes for core-language changes. Work on [pd.h](../../../include/klang/pd.h) is authorised directly. [Coverage](COVERAGE.md) identifies model dependencies; [primitive usage](PD-PRIMITIVES.md) identifies common building blocks. Existing [trial notes](../klang/README.md), [primitive results](../../tests/pd/README.md) and [level findings](../audio/comparisons/reference-levels.md) provide the initial evidence.

## Topology and primitive interface update — 16 September 2026

[Review and changes](KLANG-TOPOLOGY-REVIEW.md) apply the Pedestrians decisions to
all implemented Farnell models. Ambiguity remains the language priority. Inline
`float(object(...))` and some arithmetic/routing expressions still require an
explicit `signal(...)` conversion; no core-language workaround was promoted.

K-007 now includes the shared `pd::line`: two setter arguments select audio ramps,
three select intervalled control ramps. Both requested two-breakpoint syntaxes
are supported. Control/audio pairs will be reviewed individually, favouring one
class instead of a blanket namespace or tilde-suffix rule. See
[isolated evidence](../../tests/pd/line.md).

K-009 remains open: Creaking changes a metro's period synchronously from its bang
before the next deadline is scheduled. The current polling metro lacks that
outlet-feedback contract and tempo units. Full delay/timer/random interfaces
also remain work; existing helpers are explicitly labelled. K-008's prior
sample-identical routing evidence describes the retained earlier implementation;
pure delays now omit implicit block routes and require listening review.

`pd::vline` now replaces Gesture throughout the models. Its complete ramp/message
contract has 50 isolated fixtures, with explicit host-clock synchronisation in
the test driver. The pure models retain sample timing; see [the port notes](../../tests/pd/vline.md).

## Priorities and effort

![Complete](../../../farnell/status/complete.svg) · ![Current](../../../farnell/status/current.svg) · ![Problem](../../../farnell/status/problem.svg) · ![Not started](../../../farnell/status/not-started.svg) · ![—](../../../farnell/status/none.svg)

**Review items resolved:** ![6.7% (1/15) complete; 9 current, 3 problematic, 2 not started](../../../farnell/status/progress-1-9-3-2.svg)

Three confirmed issues are red; existing work awaiting consolidation/review is amber; unstarted investigations are white. K-005 is closed within its retained fixture scope; broader lifecycle and existing-model integration remain under K-006/K-011. K-014 remains amber because downstream review is pending despite the completed project build fix. The [Artificial Sounds evidence](../audio/artificial-sounds.md) adds block-timing, delay and detector findings to K-006–K-009, and a reproduced stream-extraction collision in K-015.

P0 means a current blocker or serious correctness failure; none is established here. P1 means address before the next affected models or before teaching the affected syntax. P2 means planned consolidation, broader validation or usability. P3 means later optimisation/polish. Estimates are active engineering effort for one contributor, including focused verification, not elapsed commitments. Historical PD research and integration review can add time.

**Chris's current priority is ambiguity resolution.** Investigate K-002 first, alongside the related routing/stream collision in K-015 and literal/conversion cases in K-003. The model style review reproduced K-002 again and retained a minimal failing case plus an evaluated workaround below. Core fixes still require separate prototypes and review.

| ID | Priority | State | Review item | Estimated effort |
| --- | --- | --- | --- | --- |
| K-001 | P1 | ![Problem](../../../farnell/status/problem.svg) Confirmed source restriction; workaround in models | Accept Klang values in `Controls::set` | 0.5–1 day |
| K-002 | P1 — user priority | ![Problem](../../../farnell/status/problem.svg) Minimal reproducer retained; worked around | Ambiguous routing of the `Output&` returned by inline object calls | 1–3 days |
| K-003 | P1 | ![Current](../../../farnell/status/current.svg) Typed literal prototype tested; promotion and examples review pending | Literal and conversion ergonomics (`0`, `0.f`, helper types, mixed expressions) | 0.5–1 day to catalogue; 1–3 days per coherent fix |
| K-004 | P2 | ![Current](../../../farnell/status/current.svg) Confirmed API constraint | Setter dispatch accepts only `param`/`relative` overrides | 0.5 day for guidance/diagnostics; 2–4 days if redesign needed |
| K-005 | P1 | ![Complete](../../../farnell/status/complete.svg) Source correction and isolated parity validated | PD table/gain resonator; both outlets and FM, Q0/Q80 | Complete for retained trial scope |
| K-006 | P1 | ![Not started](../../../farnell/status/not-started.svg) Source-level concern; targeted test pending | Sample-rate changes and cached coefficients/state | 1–2 days for contract and affected primitives; 1–3 more for models |
| K-007 | P1 when needed | ![Current](../../../farnell/status/current.svg) line/vline ports tested; host/model integration remains | Reusable ramps, delayed segments and event timing | 2–4 days |
| K-008 | P1 when needed | ![Current](../../../farnell/status/current.svg) Bell/police helpers tested; general port pending | PD-compatible delay reads, interpolation and feedback ordering | 2–5 days |
| K-009 | P1 for faithful models | ![Current](../../../farnell/status/current.svg) Optional timer prototypes measured; integration pending | Explicit event/block/summing contract in the renderer and ports | 1–3 days |
| K-010 | P2 | ![Current](../../../farnell/status/current.svg) Existing helper; integration candidate | Review `Additive` and partial/envelope abstractions | 1–2 days |
| K-011 | P2 | ![Current](../../../farnell/status/current.svg) Primitive sequence fixed; integration review open | Noise seeding, reset and compatibility for existing game models | 0.5–2 days |
| K-012 | P2 | ![Current](../../../farnell/status/current.svg) Scripts exist; CI registration absent | Lightweight syntax and audio regression entry points | 1–2 days |
| K-013 | P3 | ![Not started](../../../farnell/status/not-started.svg) Measurements available; optimisation deferred | Separate DSP cost from host/file/startup overhead | 1–2 days for a focused benchmark; optimisation estimated separately |
| K-014 | P2 | ![Current](../../../farnell/status/current.svg) Downstream review pending; project build fix complete | Windows `min`/`max` macro collision for downstream header consumers | 0.5 day |
| K-015 | P1 | ![Problem](../../../farnell/status/problem.svg) Reproduced on MSVC; explicit-call workaround | Klang routing captures standard stream extraction | 1–2 days |

## Language and learner-facing issues

<a id="k-001"></a>

### K-001 — Klang values in `Controls::set`

[Controls::set in klang.h](../../../include/klang.h) applies `std::is_arithmetic` to every argument before converting it to `float`. `param`, `signal` and unit helpers are class types, so an otherwise sensible `controls.set(value, dialHz, smoothBusy)` is rejected. [PhoneTones::set](../../../farnell/klang/Artificial%20Sounds/Phone%20Tones/phonetones.k) consequently calls `controls.set(float(value), float(dialHz), float(smoothBusy))`; [TelephoneBell::ring](../../../farnell/klang/Idiophonics/Telephone%20Bell/telephonebell.k) also converts stored control values.

Desired result: accept intended Klang scalar values without casts while retaining useful diagnostics for unsupported arguments. Prototype an explicit accepted-type/conversion policy; do not simply accept any user-defined type that happens to convert. Check control values, `param`, `Frequency`, scalar literals, unsupported inputs and overload selection. These casts currently belong to the workaround, not the preferred teaching idiom.

<a id="k-002"></a>

### K-002 — Inline call result routing

The [trial compatibility notes](../klang/README.md#compatibility-notes) record an MSVC ambiguity when routing the `Output&` returned by `osc(frequency)` directly with `>>`. [Primitive::process](../../../tests/pd/primitive.h) materialises the carrier as a `signal` before routing it. [Generic::Generator::operator(), Generic::Output and the free routing operators](../../../include/klang.h) are the relevant overload sets.

The retrospective style review reproduced MSVC C2593 for Boing's `phase(...) >> clamped`. [inline-route.cpp](../../../experiments/klang/signal-flow/inline-route.cpp) now isolates `source(2) >> destination` without PD dependencies. The competing candidates are the `Generic::Output<signal>::operator>>` member and the mutable/const free routing templates. Its default build checks that `signal(source(2)) >> destination` evaluates each object once and produces the expected sample; `REPRODUCE_AMBIGUITY` selects the failing expression. See the [commands and investigation scope](../../experiments/klang/signal-flow/README.md). Boing uses this inline conversion to avoid an unnecessary named signal.

Desired result: idiomatic `osc(frequency) >> out` and modifier chains compile without an intermediate solely for overload resolution. Use the retained reproducer to verify processing occurs exactly once, in the intended order, for const/non-const sources and chained expressions; a compilation-only fix could accidentally change the sound. Include K-015's standard-stream case when constraining routing candidates. This is a leading user-requested language priority, not a reason to add permanent boilerplate to models.

<a id="k-015"></a>

### K-015 — Standard stream extraction and routing overloads

While adding TSV events in [render.h](../../../farnell/render.h), ordinary `std::istringstream` extraction inside the Klang namespace context selected a Klang free `operator>>` template. The resulting MSVC C2678 diagnostic came from its routing implementation, with `std::string` as a destination. The encountered expression was `fields >> item.frame >> item.message`, where `fields` is an `std::istringstream`, `frame` is `uint64_t`, and `message` is `std::string`. The successful workaround explicitly calls `fields.operator>>(item.frame)` and `std::operator>>(fields, item.message)`; numeric values also use the member extraction. The Release and Debug builds and all scripted renders pass with that workaround on MSVC 19.51.

Desired result: ordinary stream syntax continues to work when Klang routing is in scope. Reproduce the overload selection in a small separate prototype, then constrain the routing templates to intended audio source/destination types without accepting unrelated standard-library expressions. Verify string and numeric extraction as well as generator/modifier chains, lvalue/temporary outputs and exactly-once audio processing. This is a core-language review item, not a change to the stream API or a learner-facing idiom to recommend.

<a id="k-003"></a>

### K-003 — Literals and conversions

Chris reports ambiguous or missing conversions that require explicit casts or `0.f` instead of `0`. Current [PhoneTones](../../../farnell/klang/Artificial%20Sounds/Phone%20Tones/phonetones.k), [TelephoneBell](../../../farnell/klang/Idiophonics/Telephone%20Bell/telephonebell.k), and [motor models](../../../sounds/Motors.h) contain mixed scalar/helper expressions and explicit conversions suitable for collecting cases. Their presence alone does not prove each cast is necessary. `std::clamp`/`std::max` template deduction, unit conversion, conditional expressions, Klang routing, and setter dispatch must be distinguished.

Record the smallest failing expression, intended type/units, actual compiler diagnostic, successful workaround, and header/compiler revision. Compare `0`, `0.f`, `0.0`, `param(0)`, `signal(0)`, and relevant unit helpers. Aim for ordinary numeric literals and unambiguous unit-aware expressions without broad conversions that weaken type checking. The original family was user-reported; the typed-literal study below now adds concrete cases without claiming to resolve the whole family.

**16 September — typed literals (P1).** The separate
[prototype and evidence](../../experiments/klang/literals/README.md) adds optional
`_Hz`/`_kHz` suffixes returning existing `Frequency` values. `1_Hz`, `1.0_Hz` and
`1._Hz` have identical types; `1.f_Hz` is an invalid suffix. MSVC and Clang C++17
checks cover overload selection, standard algorithms, normal param dispatch,
unchanged plain literals and sample-identical oscillator audio at both rates.
Constant-return assembly has no added instructions. Kleine's unchanged source
compiles on MSVC with the suffixes imported; Clang reports the same four existing
application errors with and without the header. No core changes are promoted.

This resolves selected mixed-type deduction cases when the programmer chooses
typed literals; K-001/K-002 and unit-preserving arithmetic remain separate work.
The probe also retains `source(440) * 0.2f` failing on both compilers because
`Output::operator*` takes a non-const reference; `0.2f * source(440_Hz)` works.
Review the remaining unit suffixes, signed literals, constexpr construction and
the [Time proposal](../../experiments/klang/time-units/README.md), then review all
53 example `.k` files for clearer typed values and inference improvements before
extending the migration to models. Preserve units, defaults and audio throughout.
This examples review is explicitly requested follow-up, not yet performed.

**Replacement-core preparation:** Chris has now requested a complete experimental
copy of `klang.h` for shared generic float/double types, `signal64`, `param64`,
double-backed Time and unit literals, with early promotion as the objective.
The [candidate and compatibility runner](../../experiments/klang/core/README.md)
start from an exact copy and compile all 80 source files in examples, sounds and
farnell/klang without edits, in separate translation units with root-model
instantiation. The baseline is 61/80 on MSVC and 59/80 on Clang; the identical
candidate reproduces MSVC's results with no regressions. Existing operator/API
failures and Unreal host dependencies must be distinguished from refactor
regressions. This is baseline preparation; generic numeric types and production
core replacement have not yet been implemented. The later examples migration
remains separate from these unchanged-source compatibility tests.

The [plugins/main example review](../../experiments/klang/core/reviews/EXAMPLE-REVIEW.md)
refines the scalar-rvalue finding: upstream includes four free
`Output<SIGNAL>& op float` overloads that are commented out in the Kleine copy.
Restoring only those declarations in a scratch header makes ten failing examples
compile unchanged on MSVC. All 53 local examples match Klang main; plugins does
not supply a replacement example set that repairs the current failures. Table
macro/initializer compatibility and evaluated Envelope-to-param conversion
account for the other three failing examples and remain isolated follow-ups.

**Arithmetic restoration and language regressions:** the experimental core now
restores scalar-temporary arithmetic with constraints preserving the game fork's
Function/float behaviour. Simply uncommenting upstream overloads regresses
Harrier's `abs(...) * 2` on both compilers. The constrained version improves the
unchanged-source suite to 72/80 MSVC and 70/80 Clang with zero new failures;
Kleine also builds/links on MSVC. [Language regression tests](../../tests/klang/README.md)
cover evaluation count/order, const cached reads, relative setters, types and
known ambiguities. [Sound-header comparisons](../../experiments/klang/core/reviews/ARITHMETIC-REVIEW.md)
show sample-identical Harrier against production and Bicycle/Rain against
available upstream references at both rates. Remaining minimal cases include
scalar-left Function arithmetic (Mini/Helicopter), Bank-to-mono reduction
(Motors), and compiler-dependent Control routing (Train). UE 5.8.1/VS 2026 is
the reported host; these are standalone checks, not an actual Unreal build.
Production `klang.h` is unchanged; generic numeric types remain pending.

**Supplied 0.7.9 follow-up:** the [actual game-header review](../../experiments/klang/core/reviews/GAME-HEADER-REVIEW.md)
now confirms source-level Function, Bank and Control changes that let all nine
sound roots run on standalone MSVC with explicit host dependencies. Six examples
accepted by the candidate fail on 0.7.9, and its arithmetic contract fails on
Clang. Reconcile those fixes selectively; the supplied core is not yet a
whole-collection replacement. A separate mono-to-multichannel constructor probe
also reproduces `{2,0,0}` rather than broadcasting 2 in all three headers.
These are confirmed K-003 integration issues, P1 before promotion.

**OSM correctness, P1 before core promotion:** [independent tests](../../klang/OSM-IMPROVED.md#archived-material)
verify 0.7.9's phase-scale correction, but find a near-DC default Square,
phase/tick argument-order transition spikes, and inconsistent Saw breakpoint
and slope widths. Diagnostic subclasses isolate these without changing either
core. Allow 1-2 days for focused repairs and boundary/FM tests, followed by
affected-model listening. The Farnell PD matches use independent `pd::osc` and
`pd::phasor`, so they do not validate or invalidate OSM. Detailed evidence stays
with the core experiment rather than changing the PD model acceptance records.

**OSM repair implemented:** the [experimental core repair](../../klang/OSM-IMPROVED.md#archived-material)
now passes 3,840 independent waveform fixtures plus lifecycle/FM/PWM checks on
MSVC and Clang, Release and Debug. It corrects phase scaling, duty defaults,
transition averaging, zero/negative frequency and startup, including the fast
Sine's initial 1000 Hz silence. All 80 source files retain their previous
compile outcomes; Kleine builds/links. Exact audio changes and the measured
CPU/memory cost are recorded. Production/UE promotion and affected-sound
listening remain review steps; PD primitive acceptance is unchanged.

**OSM CPU follow-up, P1 before promotion:** the [V2 source review](../../klang/OSM-IMPROVED.md#archived-material)
traces the principal faults to Klang's port, not the original state machine.
A small float correction removes the ordinary audio-frequency faults at much
lower cost than the general repair: repeated fixed-frequency microbenchmarks
are approximately unchanged to 6% slower than old Klang. This diagnostic is
not a complete replacement; preserve the full lifecycle/edge contracts while
restoring the inexpensive common path. CPU validation and the previously
identified API/ambiguity integration remain P1 core work.

**V2-informed fast repair implemented:** [current implementation and evidence](../../klang/OSM-IMPROVED.md#archived-material)
replace the experimental general integrator with a float state machine and
stable corner calculations. Positive-frequency quality/lifecycle tests pass
9,216 fixtures on MSVC/Clang, Release/Debug, with worst interval error 2.82e-7.
Repeated CPU tests are largely comparable or faster than production; MSVC's
unchanged-setter cases retain a small measured cost, up to about 7%. A separate
signed-frequency prototype also passes; multi-cycle intervals remain in the
general reference. Neither is silently added as a default fallback. Chris's
acceptance target is perceptually good approximation with excellent performance,
not mathematical perfection. Review of extant models is the next stage;
production/UE headers and prior PD sound acceptance are unchanged.

**Signed FM is required, now integrated in the experimental default:**
[current OSM status](../../klang/OSM-IMPROVED.md#archived-material) supersedes the earlier
opt-in description. Ordinary Saw/Triangle/Square/Pulse now support both signs
and FM through zero for `abs(Hz) < fs`; 18,432 fixtures pass on both compilers,
Release/Debug. CPU overhead is measured and retained, including Clang's loss of
some positive-only specialisation. The general integrator remains on the
reference bench, with no automatic fallback. **Open work:** OSM-01, rendering
FM at/above one cycle per sample; OSM-02, fractional-breakpoint repair for the
extreme LF/tiny-duty limit. Neither current limitation has been accepted as
permanent or dismissed as inaudible. Extant-code review remains pending, and
production/UE headers are unchanged.

**Positive-path and Debug follow-up:** the [current CPU review](../../klang/OSM-IMPROVED.md#archived-material)
adds direct backwards OSM states, caches direction/setup, and removes avoidable
wrapper calls from the sample path. Signed correctness passes MSVC/Clang in
Release and Debug. Explicit changes improve Debug throughput on both compilers,
and Clang's concrete Release workload improves over the signed wrapper. Some
MSVC and virtual-call Release cases regress; preserving positive-only throughput
remains **OSM-04, open before promotion**, not a solved zero-overhead claim.
Debug performance is now a recorded design requirement alongside Release.

**17 September working-production promotion (supersedes the pending OSM statuses
above):** Chris authorised the combined reflection OSM, including multi-cycle
and fractional-duty fixes, after the FM/PWM profile. It is now in `include/klang.h`;
the previous header is archived byte-for-byte. [Promotion evidence and model
comparison](../../klang/OSM-IMPROVED.md) record MSVC/Studio Clang
14.0.6 Release/Debug checks, no new compile/runtime regressions, and examples
improving from 40/53 to 50/53 compiling. Existing language/host gaps remain.
OSM-01 and OSM-02 are implemented; OSM-04 remains an optimisation opportunity:
isolated Release modulation can cost up to 2.32x the reflection baseline even
though SynTHX stays close or faster. The generic numeric/64-bit/literal refactor
and external UE/Studio source trees remain unchanged. An MSVC `/fp:fast`
constant-folding/narrowing discrepancy reproduces with both old and new headers;
the precise language suite passes. Four initial example crashes were caused by
missing debug-buffer session setup in the test host, now repaired and rerun.

<a id="k-004"></a>

### K-004 — Setter dispatch and diagnostics

[Generic::Generator and Modifier](../../../include/klang.h) dispatch through a fixed family of virtual `set(param, ...)` / `set(..., relative)` signatures. Chris's instruction and [AGENTS.md](../../../AGENTS.md) therefore require only these argument types in overrides; a custom `set(int)` or `set(bool)` can hide rather than override the intended path. Current PD/model setters follow that rule, while ordinary methods such as `dial(int)` and `ring(bool)` need not be setters.

Keep this distinction in teaching examples. Prefer `override` where a matching virtual signature exists. Investigate clearer compile-time diagnostics or an explicit extension mechanism before redesigning dispatch. Verify direct `.set(...)` and inline `object(...)` behaviour agree. A redesign is optional review work, not a prerequisite for the next sound.

The Boing trial reproduced another dispatch trap: a two-argument `set(param hz, param q = 80)` does not override the one-argument virtual setter used by `free(hz)`. The modal bank consequently kept its default frequency. [Boing::Free](../../../farnell/klang/Idiophonics/Boing/boing.k) now explicitly overrides both arities; its moving-frequency diagnostic matches PD. This is a confirmed language/API constraint, addressed in model code; consider guidance or diagnostics (P1, about half a day), not an unreviewed core change.

## DSP, lifecycle and host contracts

<a id="k-005"></a>

### K-005 — `pd::vcf` validation

The old `vcf` used a polynomial approximation and unused cached coefficients. The [Idiophonics trial](../audio/idiophonics.md) replaced these with PD 0.55-2's actual table-based complex resonator, gain and state update. [vcf.pd](../../../tests/pd/vcf.pd) compares both real/imaginary outlets, modulation through negative and high frequencies, and Q=0/80 at both rates. Nonzero residuals are below -128 dB; zero Q is sample-identical. Full Boing mode-bank comparisons also pass.

Existing `lpf`/`bpf` aliases remain for source compatibility, but describe real/imaginary quadrature outputs. Harrier uses this primitive; its smoke probe now compiles after materialising the turbine `Output` as a `signal` (K-002). Selected Harrier settings remain finite but exceed unity substantially; no old-timbre equivalence or level-safety claim is made. Broader Q/reset/rate and game integration checks remain under K-006/K-011. No core-language change was required.

<a id="k-006"></a>

### K-006 — Sample rate, preparation and reset

[pd::bpf](../../../include/klang/pd.h) caches coefficients when frequency/Q change, so a changed `klang::fs` with unchanged parameters warrants a targeted test. `hip` also calculates its coefficient in `set`; `lop` recalculates when set. [TelephoneBell::Casing](../../../farnell/klang/Idiophonics/Telephone%20Bell/telephonebell.k) allocates lengths and sets frequencies during construction. The model documentation consequently requires construction after the sample rate is set and limits established fidelity to 48 kHz/block 64.

The Artificial Sounds models now pass 39 fixtures at each of 48 and 44.1 kHz, constructing fresh instances after setting the rate. That establishes two fixed-rate configurations, not a live rate-change contract. The police delay's 165 ms conversion exposed a half-sample rounding difference at 44.1 kHz: retaining PD's `float(fs) / 1000 * milliseconds` operation order fixed the impulse comparison.

Define whether runtime rate changes are supported or require reconstruction. If supported, refresh affected coefficients and buffer sizes through the host lifecycle, without allocations in the audio callback. Keep Chris's rule: do not manually call `prepare()` from constructors or `set()`; the processor calls it at each buffer. Check reset/retrigger separately from rate changes, including delay contents and oscillator phase. This is a concrete audit target, not a claim that the current fixed-rate trials fail.

<a id="k-007"></a>

### K-007 — Ramp and event helpers

[TelephoneBell::Decay](../../../farnell/klang/Idiophonics/Telephone%20Bell/telephonebell.k) is a finite linear decay with a tested first-sample convention. [vline-decay.pd](../../../tests/pd/vline-decay.pd) tests that restricted pattern. The new `pd::line` implements `line~` for a fixed 64-sample block, including immediate values, retargeting and stop; [line.pd](../../../tests/pd/line.pd) matches sample-for-sample at both rates. It serves DTMF and the alarm models. A requested 1 ms DTMF fade lasts one block, so replacing it with a sample-duration ramp changes the reference.

**16 September update:** Gesture has been removed. [pd::vline](../../tests/pd/vline.md)
ports PD 0.55-2's complete float/list, one-shot duration/delay inlets, queue suffix
replacement, coincident jump/ramp ordering, negative-time handling and stop.
Bouncing, Rolling, Creaking, Boing and the early bell studies use the primitive.
The 50 isolated cases are sample-identical with the reference driver's explicit
PD clock anchoring. Pure sample timing uses a sample-count clock; a ten-second
zero-height/retrigger check protects Bouncing's coincident jump/attack ordering.

`messageTime(ms)` supplies fractional host message timestamps; `sync(ms)` anchors
PD's audio clock explicitly. The driver preserves the float division in PD's
scheduler tick, which matters for exact-boundary jumps. No block adapter was
added to the model files. Reblocking and DSP pause/resume remain host contracts.

The queue allocates on messages, as PD does. Bounded/allocation-free storage is a
performance candidate (P2, 1–2 days), not a reason to restrict the current message
contract. TelephoneBell's separate finite Decay helper remains to be migrated.
The migration also exposed a tiny negative envelope sample in legacy Bouncing:
its noninteger power now returns zero for negative bases, matching PD's `pow~`
domain guard instead of propagating NaN into the oscillator.

**DTMF control-delay update:** `pd::del` now supplies reusable bang, hot/cold float,
stop and tempo-unit behaviour; [52 control cases](../../tests/pd/del.md) pass at both
rates. PD's pending sample-unit tempo-change quirk is reproduced and documented.
The dialler uses `del{200}` with no model-local timestamp. Shared-clock ordering,
synchronous feedback and block delivery remain explicit host concerns.
The explicit Sand abstraction and separate decoder graphs pass
[56 message cases](../../tests/pd/dtmf-messages.md); 18 existing audio recipes retain
identical samples. Migrating other model-local control timers is remaining P2
work, to be done during their individual reviews (roughly half a day per model).

Use the next model's requirements to select the smallest reusable object. Inspect [Klang Envelope/Ramp](../../../include/klang.h) before adding a duplicate. Keep the helper nested until there is a second independent use. Test event boundaries and re-triggering at 48/44.1 kHz and across buffers; preserve PD quirks only where the reference requires them.

<a id="k-008"></a>

### K-008 — Delay semantics

**16 September 2026 — feedforward/feedback tap indexing.** Reviewing
`Police::Environment::Delay` against `klang::Delay` identified avoidable buffer
duplication and a timing convention to resolve. In both fixed and resizable Klang
delays, `tap(int N)` reads `(position - 1) - N`, relative to the last written
sample. With one write per sample, writing before reading gives an N-sample delay;
reading before writing gives N + 1 samples. Exact N-sample feedback therefore
currently requires `tap(N - 1)`. This is a source-level finding, separate from
PD's 64-sample scheduling; an isolated impulse test remains pending.

**Chris's proposal:** investigate using `>>` versus `<<` to identify feedforward
and feedback context, so the model can express its intended delay as `tap(N)`.
The routing entry points are distinct (`>>` calls `input(source)`, while `<<` has
its own overload), but currently converge on the same input/write operation.
Operator direction is a candidate API convention, not an established detector of
execution order. Resolve how context is known on the first feedback read, before
the first `<<`, and how mixed routing and multiple taps behave. Prototype under
`experiments/klang` before proposing a core change; retain existing model behaviour
until the contract is agreed. Priority P1 before replacing affected delay helpers;
initial contract/probe estimate 0.5–1 day within K-008's broader work.

Check impulse positions for both processing orders, startup, wraparound, integer
and fractional taps, minimum/zero delay, multiple reads per write, and existing
feedforward/feedback examples. Merely shifting tap indexing globally would shorten
the existing write-before-read delays by one sample.

The following comparison evidence predates the topology review, which removed
model-local block routing delays; see [the current review](KLANG-TOPOLOGY-REVIEW.md).

[TelephoneBell::Delay and Casing](../../../farnell/klang/Idiophonics/Telephone%20Bell/telephonebell.k) explicitly separate read/write and retain measured 64/128-sample routing delays. This is a useful prototype, not a complete `delread~`/`vd~`/`delwrite~` implementation. [Klang Delay](../../../include/klang.h) already supplies delay facilities; inspect its interpolation and update order before introducing another general object.

[Police::Environment](../../../farnell/klang/Artificial%20Sounds/Police/police.k) adds a second use: three fixed feedback taps, each with one extra 64-sample routing block. Its isolated impulse fixture is sample-identical at both rates. The model keeps its delay nested; this corroborates the importance of scheduling without establishing a reusable general delay contract.

[SampleDelay and Creaking::Reflection](../../../farnell/klang/Idiophonics/Creaking/creaking.k) add fixed panel reflections with a 64-sample feedback return. Both the isolated dfbef and complete panel impulse responses are sample-identical at 48/44.1 kHz. A4's earlier bell additionally covers a 59 ms filtered feedback body. These are useful shared helpers with explicit routing lengths, not a general PD named-buffer implementation.

Needed evidence: integer/fractional delay, minimum read delay, allocation size, interpolation kernel, write/read ordering, feedback, block boundaries and rate changes. Retain the bell fixture while generalising. Delay timing is part of the timbre in feedback models, so replacing it with an apparently equivalent tap is a correctness change.

<a id="k-009"></a>

### K-009 — Events, blocks and immediate processing

The [renderer](../../../farnell/render.h), [Python wrapper generator](../../../tools/render_farnell.py), and [model notes](../klang/README.md) already encode pulse-event rounding and the bell's PD scheduling. Klang processes immediately; PD schedules a graph. Fan-out from one stored sample, summing multiple connections, message ordering and feedback therefore need explicit translations rather than a presumed graph runtime.

[render_artificial.py](../../../tools/render_artificial.py) now supplies shared TSV events on a 64-sample grid. PD message timestamps sit a quarter-sample inside the intended block to avoid floating-point boundary ambiguity. Metro start fires immediately; stopping holds the current gate, rather than muting. Ringback's nominal whole-second events also require block rounding at 44.1 kHz. `pd::env` publishes its new control value after its source block, and the phone sequencer consumes a detected digit on the next block.

The decoder trial also exposed a processing contract worth documenting: discarding `in >> detector` does not evaluate that route. [DTMFTones::Decoder](../../../farnell/klang/Artificial%20Sounds/DTMF%20Tones/dtmftones.k) now completes the route with `>> out` before reading each detector's state, replacing its earlier unused signal temporary; it restores the pass-through output afterwards. This is an immediate/deferred expression boundary, not a newly established compiler defect. Consider teaching guidance or diagnostics for discarded routes; preserve normal lazy routing semantics.

The [Pedestrians block API paper](PD-BLOCK.md) now compares developer-owned/inherited clocks and automatic `block()` events, including a self-disabling default and variable sizes. Its [separate C++17 probes](../../experiments/klang/pd-block/README.md) preserve audio under different host partitions and demonstrate that a buffer-only override misses nested signal evaluation. An inherited explicit action works on the current core; preserving `process()` with automatic events everywhere needs a common evaluation hook, or a different sample-method spelling. No core change or production API promotion is implied.

Idiophonics adds sub-sample event evidence: PD parses delay milliseconds as float. A quarter-sample guard avoids earlier-block dispatch at 44.1 kHz; the renderer now reproduces that float-rounded fractional timestamp in finite gestures. Boing's phase and frequency diagnostics distinguish control-block mistakes from DSP errors. Creaking retains documented small pulse/control-timing approximations in the two-rate gesture trials, with exact panel/material checks isolating it from the delay network.

Document a small host contract for event timestamps, control updates, `prepare`, fixed-rate construction, tails and reset. When shared by a second model, factor event utilities out of the render case switches. Verify the pulse and bell fixtures remain stable. This is not a proposal to build a complete PD interpreter or force a general scheduler into every model.

The [retrospective style comparison](../../../farnell/audio/comparisons/klang-style-review.json) extended the original chapter 25/29 runs to 44.1 kHz and found existing reference mismatches for `pulse` and the final `bell` (raw PD-relative residuals about +0.37 and +2.89 dB). The saved pre-refactor executable reproduces both; `pulse` is sample-identical before/after and the bell differs only at rounding level. Their original retained parity scope is 48 kHz. Investigate event/block alignment and reference setup before extending that scope (P1 when extending these models to 44.1 kHz, about 0.5–1 day); this does not implicate the already passing two-rate earlier bell studies.

## Reuse, verification and performance

The Pedestrians review now implements Chris's pure polling form using [pd::metro](../../tests/pd/metro.md), `set(param on)` and inline `osc(2500)`, with the patch's constants hardcoded. Sixteen event-count fixtures pass; 48 kHz model output remains sample-identical. PD block delivery is explicitly deferred: 44.1 kHz tests check bounded edge shifts and carrier identity instead of raw parity. This is current K-009 evidence; it does not promote any `pd::block` / `pd::sound` proposal. The arithmetic oscillator expression compiles without a K-002 workaround, while the separate direct-routing ambiguity remains open.

**16 September — optional audio-thread timers.** The
[scheduling prototypes](../../experiments/klang/scheduling/README.md) implement
`every(100, &Alarm::tick)` in `prepare()`, with default milliseconds, tagged
sample intervals, idempotence, first-registration precedence, fractional
deadlines, interval changes, cancellation and bounded storage. Fixed versus
startup-reserved storage and branch versus function-pointer buffer dispatch
have Release/Debug checks and CPU microbenchmarks. See the
[measurements](../../experiments/klang/scheduling/measurements.md) and
[hashed evidence](../../../experiments/klang/scheduling/results.json).

The recommendation is a branch per buffer and optional startup-reserved timer
storage: 24 bytes of unused adapter overhead and a 992-byte bank for eight timers
on this MSVC build. No allocation occurs during registration/rendering/updates.
Performance when unused is close to the original oscillator workload, with
measurable code-layout differences in the cheapest workload; this is not a
zero-cost guarantee. Sparse coefficient updates avoid per-sample work without
manual counter logic. Dense timers and small buffers have measurable overhead.

An explicit 64-sample control-window example reproduces cached PD alarm01 audio
at both rates; the sample-timed form reproduces Kleine. This supports a separable
PD adapter, not a complete shared PD scheduler. Buffer-only scheduling still
does not automatically cover nested sample evaluation or custom buffer
overrides. Parent-owned registration of child handlers is implemented and tested.
Startup reservation, common evaluation, synth/note lifecycle and copy/rebind
semantics remain promotion decisions. Core Klang and production models are
unchanged. Review priority P1; the original integration estimate remains 1–3 days,
with additional cross-platform benchmarking required before claiming portability.

<a id="k-010"></a>

### K-010 — Additive and partial-group candidates

[include/klang/utils.h](../../../include/klang/utils.h) contains `Additive`, partial collections and dB helpers. [TelephoneBell::Bell::Group](../../../farnell/klang/Idiophonics/Telephone%20Bell/telephonebell.k) contains tuned PD oscillators and a shared envelope. They overlap conceptually but differ in phase, frequency, amplitude and envelope requirements.

Audit helper conversions, partial count/assignment, units and initialisation; compare the minimal APIs needed by chapter 17 and the bell. Promote a reusable object only when it improves both models and retains readable signal flow. The [block API experiments](../../experiments/klang/pd-block/README.md) are a separate timing study, not an Additive prototype. The earlier PD reference experiment is no longer authoritative; do not recreate an obsolete `PdReference.h` merely to populate the backlog.

<a id="k-011"></a>

### K-011 — Noise seeds and existing-model compatibility

[pd::noise](../../../include/klang/pd.h) now uses unsigned wrapping arithmetic, outputs before advancing, and supports explicit seeds. [Retained tests](../../../tests/pd/results.json) establish seeded parity. The default construction seed remains shared state, so instance order influences the resulting sequence. [Model compatibility notes](../klang/README.md#compatibility-notes) record that existing users such as [Harrier](../../../sounds/Harrier.h) receive a changed sequence.

Document explicit seeds for reference fixtures and an intentional seed policy for multiple game actors. Add reset/reconstruction and repeated-instance cases if required. Compare [ToyBoatEngine/FourStrokeEngine](../../../sounds/Motors.h), [Helicopter](../../../sounds/Helicopter.h), and Harrier before claiming parity with their Farnell references; current adaptations are useful starting points, not already measured equivalents.

<a id="k-012"></a>

### K-012 — Focused regression entry points

[CMakeLists.txt](../../../CMakeLists.txt) does not register CTest cases. The meaningful existing checks live in [check_pd_primitives.py](../../../tools/check_pd_primitives.py), [render_farnell.py](../../../tools/render_farnell.py), and [check_reference_levels.py](../../../tools/check_reference_levels.py). Existing [Klang tests](../../../include/klang/tests.h) are not currently integrated into this project's CTest path.

The extended trial adds [render_artificial.py](../../../tools/render_artificial.py): 39 paired fixtures at each of two rates, with a residual/level failure gate, finite/non-silent checks, and explicit reconstructed references where upstream external objects are missing. Primitive checks now cover 58 cases. Idiophonics adds 33 paired cases per rate, including material/modal diagnostics and documented contact-model envelope/spectral gates; rapid controls and Harrier have a standalone smoke probe. [package_artificial.py](../../../tools/package_artificial.py) validates retained listening WAVs and records source/audio hashes. Release and Debug builds pass; CI registration and cross-compiler coverage remain open.

Chris's creaking review exposed a **comparison-design error**: the initial held-force fixture reproduced the patch but missed the website's accelerating/decelerating performance. [Contact timing and first-contact analysis](../audio/idiophonics.md#creaking-correction-after-listening-review) support the same material model with different force gestures. The listening fixture and default renderer now use a shared moving-force contour; the original control test remains separately reproducible. Matching a reference implementation does not establish a convincing performance or game-ready realism. Keep behavioural listening recipes alongside technical controls for future practicals (P1, incorporated in this trial).

Add a convenient, optional entry point for the retained checks and small syntax regressions when fixing K-001–K-004. Separate checks requiring PD/Python audio dependencies from a normal compiler build. Avoid tests that merely mirror implementation; retain cases for actual overload failures, processing order, state transitions and audio residuals. Confirm compiler coverage before claiming the MSVC workarounds are portable.

<a id="k-013"></a>

### K-013 — DSP cost measurements and later optimisation

[AGENTS.md](../../../AGENTS.md) explicitly puts faithful working sound before optimisation. Existing [audio manifests](../audio/README.md) and render tools provide process-level timing/memory context. These include startup and output overhead and are not pure per-node DSP timings.

Use a warmed renderer with identical sample rate, voice count, duration, controls and file-I/O policy. Report CPU seconds per rendered audio second and memory separately for process and incremental voices. Optimise hotspots only when measured: coefficient updates, delay allocation, oscillator tables and per-sample virtual dispatch are candidates, not established bottlenecks. Do not impose a performance gate on the next reference model.

<a id="k-014"></a>

### K-014 — Windows header macros

Including the engine before [pd.h](../../../include/klang/pd.h) previously exposed a Windows `min`/`max` macro collision. [CMakeLists.txt](../../../CMakeLists.txt) now defines `NOMINMAX` for Windows, and the project builds with that fix. Review whether public header usage needs documentation or defensive syntax so consumers outside Kleine do not rediscover it. Preserve the existing build fix; no additional platform change is required for the current trials.

## Consolidation workflow

During implementation, capture new friction in the relevant model's notes or a small tracked note beside its prototype under `experiments/klang`. Include a stable K-ID where one exists, source location, minimal expression/patch, compiler or PD revision, observation, workaround, and effect on the model. Keep generated logs, repro binaries and temporary audio under `build/`.

Update this file when requested, when completing a practical series, or when a finding blocks the next agreed model. Merge repeated cases, distinguish confirmed defects from hypotheses and deliberate design constraints, revise effort estimates after a reproducer exists, and link the actual prototype and validation evidence. Keep resolved entries briefly with their resolution rather than losing the reason a workaround existed. Core-language integration still follows the review boundary in AGENTS.md; `pd.h` can be developed directly.
