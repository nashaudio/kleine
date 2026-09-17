# PD block timing in Klang: a Pedestrians design study

**16 September update:** the [topology review](KLANG-TOPOLOGY-REVIEW.md) extends the pure-model approach across the implemented series. Model-local block rounding, lookahead and routing padding have been removed. Any required replacement must be explicit and separable, following the planned per-sound review.

Design paper, 15 September 2026. Related review items: [K-007](KLANG-REVIEW.md#k-007) and [K-009](KLANG-REVIEW.md#k-009).

**Implementation update:** Chris selected the pure polling form below, with `set(param on)` and `metro = on`. It now compiles in [Pedestrians](../../../farnell/klang/Artificial%20Sounds/Pedestrians/pedestrians.k), including the inline oscillator expression and the patch's hardcoded values. [pd::metro](../../tests/pd/metro.md) owns the clock and supports `operator=(param)`. Sixteen event-count fixtures pass; PD block delivery remains deliberately separate. The block alternatives and earlier manual implementation below remain design history.

This paper compares the code a sound author would write, starting with the desired Klang presentation and progressively accommodating PD timing. The pure `pd::metro` form is now implemented. Examples using `Metro`, `pd::block` or `pd::sound` remain **proposed APIs**. Small [C++17 probes](../../../experiments/klang/pd-block/prototypes.h) implement those alternatives under `block_trial`; they retain the earlier callback presentation.

## 1. Start with the sound we want to teach

The [PD patch](../../../farnell/pd/PEDESTRIANS/pedestrian-beep.pd), associated with Designing Sound figure 24.4, contains a 100 ms metro, a counter modulo two, a 2500 Hz oscillator and an output multiplier of 0.2. There is no visible block object. Starting the metro emits its first count immediately, initially leaving the gate at zero; stopping holds the last gate value. A stopped metro is therefore not necessarily silence. Those behaviours belong to the model even if we remove PD's timing quantisation.

Chris's revised ideal makes the two rates clear through ordinary control flow, without explaining any DSP block:

```cpp
// Alternating pedestrian-crossing beeps, starting with a silent half-cycle.
struct Pedestrians : Sound {
    pd::osc osc;
    pd::metro metro;
    signal gate = 0;
    int count = 0;

    void set(param on) override {
        metro = on;
    }
    void process() override {
        if (metro(100))
            gate = count++ % 2;
        osc(2500) * gate * 0.2f >> out;
    }
};
```

This is the preferred teaching target. `metro(100)` asks whether a 100 ms metronome ticks on this sample; the oscillator runs on every sample, including the silent half-cycle. The counter belongs to the model, matching the patch's separate counter and modulo objects. It must be declared and incremented: `count % 2` alone would leave the gate unchanged. Post-increment makes the first tick select zero. Neither a callback, a `prepare()` override nor an intermediate oscillator value belongs in this ideal merely to accommodate today's implementation.

The metro starts disabled. Assigning a nonzero `param` arms a first tick for the next sample evaluation; assigning zero cancels it. Each `metro(100)` call advances one sample and returns a bang count, used as a condition here, even while timekeeping continues disabled. An unchanged interval does not restart the timer. The counter survives stop/restart, and stopping holds the gate. Fractional deadlines belong inside the metro. The initial tick is returned on the first evaluation, not after waiting 100 ms, so the initially silent half-cycle lasts 100 ms.

This polling interface applies the start event when `process()` next runs; PD sends its first bang synchronously in the start message. That distinction is harmless for the intended single start/stop control between audio samples when only the subsequent audio is observed. Multiple start/stop messages before evaluation, immediate inspection of the gate, and sub-sample periods need an explicit contract before claiming a general PD-compatible primitive. A boolean reports at most one event per call; it cannot silently stand in for draining several events in a block.

For Pedestrians, a metro could also own its timing quantisation internally and preserve this same model source. That possibility should be tried before requiring a model-level `block()` handler. Shared scheduling between several objects remains a separate question. The inline oscillator expression now compiles in this arithmetic context; [K-002](KLANG-REVIEW.md#k-002) still records other inline-output routing friction. The implementation update above links current validation.

### Callback baseline used by the existing probes

The following is the earlier executable design baseline. It remains here so that the block variants and their recorded measurements can be traced to the code actually tested:

```cpp
// Alternating pedestrian-crossing beeps, starting with a silent half-cycle.
struct Pedestrians : Sound {
    pd::osc tone;
    Metro metro{100};
    signal gate = 0;

    void start(bool on = true) {
        metro.start(on, [&](int count) { gate = count % 2; });
    }
    void prepare() override { tone.set(2500); }
    void process() override {
        metro([&](int count) { gate = count % 2; });
        signal wave = tone;
        wave * gate * 0.2f >> out;
    }
};
```

Here `Metro` is a proposed ordinary sample-timed helper. It owns elapsed time, fractional deadlines and the counter, supplies the immediate first event in `start`, and subsequently invokes the supplied action when due. A named `tick(int count)` could replace the repeated lambda body; it is a style choice, not a requirement of blocking. `pd::osc` remains useful independently of the scheduling question. Materialising `wave` follows the project's existing workaround for inline output conversions.

The examples use the patch defaults to keep the comparison about syntax. The current model's frequency, interval and gain setters would remain available in an implementation. Its stop/restart behaviour is retained in the probes. `Metro` is deliberately labelled proposed: changing `Sound` alone cannot make the current manual metronome disappear.

## 2. What the previous implementation added

The previous Pedestrians source implemented its own metronome. A sample-timed version using the same state would contain:

```cpp
if (running && frame >= nextTick) {
    gate = (count++ % 2) != 0;
    nextTick += double(float(interval)) * float(fs) / 1000;
}
```

PD compatibility changes the condition to:

```cpp
if (running && frame / 64 >= int(nextTick / 64)) {
```

The precise block-specific cost is **two divisions by 64, a conversion, and the explanatory comment**. `frame`, `nextTick`, `count`, `running`, the time conversion and `++frame` are mostly the cost of implementing a metro manually. Moving all those fields into `pd::block` would give the block object responsibilities belonging to the metronome.

| Visible machinery | Natural owner |
| --- | --- |
| Period, running state, fractional next deadline, restart policy | `pd::metro` (the earlier `Metro` probe also packages a counter) |
| Position within an internal block, block length, enabled state | `pd::block` |
| Calling a model's block handler at the correct point | `pd::sound` / common evaluation entry point |
| Counter, modulo two, held gate, oscillator and gain | `Pedestrians` in the revised ideal |

The purpose is to recover that separation, not merely shorten an `if` expression.

### Advancing a metro once is not advancing it for one block

The current condition applies a known periodic event at the **start of the block containing its deadline**. At 44.1 kHz, the first 100 ms deadline is sample 4410, inside `[4352, 4416)`, so the reference gate changes at 4352. A block callback that merely asks `now >= nextTick` would wait until 4416 and be late. At 48 kHz, 100 ms is exactly 4800 samples, also a block boundary, which conceals this mistake.

The proposed helper processes a time span:

```cpp
metro([&](int count) { gate = count % 2; });         // one sample
metro(blocks, [&](int count) { gate = count % 2; }); // one internal block
```

The second form reads the block's length and drains events in its half-open logical interval `[begin, end)`. It does **not** advance the block clock. Keeping fractional deadlines avoids cumulative drift; retaining every event avoids losing counts when a period is shorter than one block. For this metro-to-held-gate patch, the last event in the interval determines the gate for the block.

This look-ahead applies to already scheduled model events. It cannot predict a live external control message that has not arrived. External control acceptance and timestamp policy remain separate host responsibilities. `Metro(blocks, ...)` is not a complete PD message scheduler.

## 3. Candidate author-facing forms

### A. The developer adds a block member

This is the most local extension of the callback baseline:

```cpp
// Pedestrian beeps with explicit ownership of a PD timing clock.
struct Pedestrians : Sound {
    pd::block blocks; // proposed default: 64 samples
    pd::osc tone;
    Metro metro{100};
    signal gate = 0;

    void start(bool on = true) {
        metro.start(on, [&](int count) { gate = count % 2; });
    }
    void prepare() override { tone.set(2500); }
    void process() override {
        if (blocks())
            metro(blocks, [&](int count) { gate = count % 2; });
        signal wave = tone;
        wave * gate * 0.2f >> out;
    }
};
```

`blocks()` consumes one sample position and returns whether it begins a block. It is called exactly once per sample, regardless of whether the metro is running. Placing it behind `running && ...` would stop the block clock too, shifting the grid when the metro restarts.

This form works inside an ordinary nested generator evaluation because it lives in the model's existing `process()`. It needs no core change. Its cost to the author is one member declaration, an `if`, and the `blocks` argument. A second occurrence of `blocks()` in the same sample would accidentally advance twice, so this is deliberately an advancing call, not a state query.

Equivalent action forms are possible:

```cpp
blocks([&] { update(); });                  // immediate captured lambda
blocks(&Pedestrians::update, this);         // immediate member callback
if (blocks()) update();                    // direct control flow
```

All three mean “run this action at the next due boundary”; none registers a persistent callback. `update()` contains `metro(blocks, ...)`. The first keeps the call short; the second avoids a lambda but repeats the type name; the third is especially clear for a single statement. The probes exercise all three forms.

### B. The parent supplies the machinery, and the author chooses when to use it

Supply `blocks` through inheritance and keep the explicit call:

```cpp
// A PD-oriented sound with optional block machinery available to its author.
struct sound : Sound {
    pd::block blocks;
};
```

The model becomes `struct Pedestrians : pd::sound`, removes the member declaration, and keeps either `if (blocks()) ...` or `blocks([&] { update(); });` in `process()`.

This satisfies the “gubbins without another declaration” objective. An unused clock does no scheduling work, although its storage exists. It also preserves `process()` and works for nested objects on the current header. The parent does not secretly advance this clock: combining automatic advancement with the author's advancing call would be a bug.

It is the **smallest immediately usable form on the current Klang evaluation paths**. It is still explicit about when the author wants block work, and does not yet implement “override `block()` to enable”.

### C. The parent supplies an automatically invoked event

Move the timing call out of the audio body altogether:

```cpp
// Pedestrian beeps whose control state advances at PD block boundaries.
struct Pedestrians : pd::sound {
    pd::osc tone;
    Metro metro{100};
    signal gate = 0;

    void start(bool on = true) {
        metro.start(on, [&](int count) { gate = count % 2; });
    }
    void prepare() override { tone.set(2500); }
    event block() override {
        metro(blocks, [&](int count) { gate = count % 2; });
    }
    void process() override {
        signal wave = tone;
        wave * gate * 0.2f >> out;
    }
};
```

Compared with the callback baseline, the visible additions are the `pd::sound` base, a `block()` event and one `blocks` argument. Compared with the revised conditional ideal, this version also retains the callback metro API and oscillator conversion workaround. The author does not declare a clock, remember to advance it, or write a numerical block size. `event` is already Klang's alias for `void`; it communicates the same sort of role as `on` and `off` in notes.

Two defaults produce this **same model source**:

```cpp
// Optional event: the first call retires the default handler.
virtual event block() { blocks.disable(); }

// Always-supported event: call even when the model does nothing here.
virtual event block() {}
```

The first mirrors [NoteBase::off](../../../include/klang.h), whose default changes the note's stage and whose override supplies specialised behaviour. Start with a clock armed for 64 samples. Invoke the virtual handler at the first actual audio boundary, after construction and `prepare()`. If a derived override exists, it runs; otherwise the default disables subsequent block dispatch. There is no constructor-time virtual probe, vtable inspection or override-address comparison.

An intentionally empty override keeps block dispatch enabled; omitting the override disables it after its first call. Calling `pd::sound::block()` from an override deliberately disables it too. This convention must be documented, just as calling a default note `off()` has a lifecycle effect.

The always-empty default is simpler to explain and useful if most `pd::sound` models really use blocks. It performs repeated boundary checks/calls for models that do not. Their cost might be small; it has **not** been benchmarked here, and devirtualisation should not be promised for every host. Neither policy eliminates the per-object storage. The self-disabling policy is a runtime opt-out, not a guarantee that all overhead compiles away.

## 4. Default size and configuration

**Use 64 as the configured default, with the optional default handler retiring itself.** Zero cannot be the initial “disabled until overridden” value if overriding `block()` is supposed to be the only opt-in: a dispatcher that never invokes the handler cannot discover the override through its behaviour. A separate first-call discovery mechanism would still be needed and adds no benefit over starting armed.

Give the inherited clock a small explicit control surface:

```cpp
// Only models choosing a non-default size need this constructor.
Pedestrians() { blocks.set(128); }

// Configuration by an owner is equally possible.
sound.blocks.set(128);
sound.blocks.set(0);     // disable block callbacks; continue audio samples
sound.blocks.set(64);    // re-arm at the next sample
```

`set(param)` follows the project's setter convention. The value represents an integer sample count; invalid fractions and negative values should be rejected. The probe supports nonnegative integral counts, including one; this clock alone is not a claim to implement PD reblocking, overlap or resampling.

Zero means **no block callbacks**, not “call once per sample”, “inherit the device buffer size” or “make the model sample-accurate”. For the event-based Pedestrians source above, disabling block callbacks holds the current gate while the oscillator continues. An unquantised version advances its metro in `process()`, or uses size one with a handler. This distinction prevents a surprising silent conversion between timing models.

Configuration is independent of the user-facing sound controls. Block size should not normally appear beside frequency and loudness in a game or teaching UI. A constructor or owner configuration is enough. In the probe, changing size explicitly restarts its local phase at the next processed sample. Production code should apply such configuration on the audio thread or while stopped; it is not an unsynchronised public control for a second thread.

| Alternative spelling | Assessment |
| --- | --- |
| `pd::block blocks{128};` | Good for independent member ownership; redundant when supplied by the base. |
| `blocks.set(128)` | Recommended runtime configuration; the default needs no user code. |
| `pd::sound<128>` | Reasonable optional fixed-size facade, but should not make every default model spell `<>`. An alias can name the default specialisation. |
| `constant blocksize = 128;` in the derived class | Attractive visually, but merely hides a base data member; it does not change what a non-templated base reads. Do not present it as working configuration. |
| `block(128)` | Conflicts conceptually with the event name and risks overload hiding. Keep the event and configuration names separate. |
| `0` means automatic/inherit | Avoid: it competes with the requested disabled meaning. |

The distinction between `pd::block` (a type), `blocks` (inherited state) and `block()` (an event) is intentional. A data member and a handler called `block` would collide in the same class scope.

## 5. The automatic event must reach every evaluation path

Klang's [Effect::process(buffer)](../../../include/klang.h) currently calls `prepare()`, then calls the model's virtual `process()` for each sample. [Kleine's Processor](../../../include/klang/engine.h) reaches a `Sound` through that buffer overload. A prototype can insert the block event there without modifying Klang:

```cpp
void process(klang::buffer audio) override {
    prepare();
    while (!audio.finished()) {
        input(audio);
        if (blocks()) block();
        this->process();
        audio++ = out;
        debug.buffer++;
    }
}
```

This preserves the separation between host preparation, internal block updates and sample DSP. It does not require collecting 64 input samples before producing output, so a callback clock itself adds no buffering latency. It also does not create any of PD's graph-routing delays.

However, [Generic::Output and Modifier](../../../include/klang.h) can evaluate an object through a conversion, routing operator or arithmetic operator. Those paths call `process()` directly. They do not visit `process(buffer)`:

```cpp
signal wave = nestedPedestrians; // buffer-only block event would be skipped
```

The probes reproduce that failure. Overriding only the conversion operator would still leave other routes uncovered. An `input()` hook alone cannot intercept a source that is evaluated without new input. Neither a virtual base class nor a smaller host buffer repairs this dispatch gap.

There are three viable choices:

| Choice | Pedestrians' visible cost | Integration implication |
| --- | --- | --- |
| Inherited clock used in `process()` (B) | One explicit action/condition | Works across existing nested and buffer routes. |
| Base owns `process()`; author overrides `sample()` | Change `process()` to `sample()` in C | Works across current routes without core edits; introduces a different model-writing convention. |
| Common evaluation hook in Klang | Preserve C exactly | Best long-term presentation; requires a separate core proposal covering every evaluation path. |

The second can be implemented with ordinary virtual dispatch:

```cpp
// Inside the alternative pd::sound base.
virtual void sample() { out = in; }
void process() final {
    if (blocks()) block();
    sample();
}
```

This is tested, including nested evaluation. `prepare()` remains the owner's buffer responsibility for nested objects; the base does not magically know the surrounding host buffer boundaries.

For the third, a prospective core evaluation entry could perform a pre-sample hook and then `process()`. The buffer loop, non-const conversions, routing and arithmetic must all use that entry exactly once. Stored-output reads must continue to avoid processing. Audit inline calls and fan-out as well. This paper does not claim that adding one virtual method alone supplies those call sites, nor that direct external calls to `process()` would automatically acquire the new wrapper behaviour.

## 6. C++17 can shorten the machinery without obscuring the action

Immediate lambda/member invocation needs neither a stored `std::function` nor heap allocation:

```cpp
template<class F, class... Args>
void operator()(F&& action, Args&&... args) {
    if ((*this)())
        std::invoke(std::forward<F>(action), std::forward<Args>(args)...);
}
```

The compiler sees the concrete callable type. This supports `blocks([&] { update(); });` and `blocks(&Pedestrians::update, this)` with the same implementation. The lambda is invoked during that call, so it does not leave a captured `this` pointer stored in an object that could later move or be copied. This takes its cue from Klang's callable objects and expression syntax, while keeping time advancement explicit.

Several tempting forms deserve restraint:

- **`if (blocks)`** looks like a read-only enabled-state query. Making its boolean conversion advance time makes an innocent second query change the sound. Prefer `blocks()` for advancing and a separate non-advancing state query.
- **`blocks >> action`** resembles Klang signal routing. Here it would dispatch an event, with no signal result; existing broad routing overloads already cause [K-015](KLANG-REVIEW.md#k-015). A call is shorter to explain and less likely to collide.
- **`blocks = [&] { ... };`** could mean registration or immediate execution. Registration introduces callable ownership and lifetime questions that the immediate form avoids.
- **An inferred capturing-lambda data member** cannot simply use `auto`/class-template deduction as a non-static member declaration in C++17. A named closure type, type erasure or an owner-taking template would add more machinery than an immediate lambda. Do not borrow C++20 lambda-in-`decltype` examples and label them C++17.
- **`pd::sound<Pedestrians>`** could use compile-time knowledge of the derived type to select optional work. That repeats the model name and complicates access/overload handling. Ordinary virtual overriding plus a self-disabling default already expresses the requested policy. Standard C++17 offers no reflection by which a non-templated base can automatically name its most-derived class. Virtual dispatch itself is sufficient; inspect neither vtables nor cast virtual member pointers to guess overrides. See the [C++ virtual-function rules](https://eel.is/c++draft/class.virtual).
- **An implicit thread-local “current block”** could let `metro(...)` mean one sample in `process()` and a whole block in `block()`. It would remove one argument, but make the same expression advance different amounts depending on ambient context. Nested clocks, different rates and re-entrant processing would need scoped restoration. Prefer the small explicit `metro(blocks, ...)` distinction initially.

These choices keep the most useful Klang characteristic: a short expression should still communicate when computation happens. They do not require a generic event bus, graph runtime or registration framework.

## 7. Timing contract and limits

The recommended clock has a local origin at its first processed sample. At sizes 64 and 128, callbacks occur at `0, 64, 128, ...` and `0, 128, 256, ...`. The phase persists across host buffers and through metro stop/restart. A 100-sample callback followed by 28 samples must produce the same audio as one 128-sample callback under the same timestamped controls. `prepare()` can still occur a different number of times; buffer-dependent side effects in `prepare()` would invalidate that stronger audio equivalence.

The block handler runs before its first audio sample. A detector reporting a result after a block may require an end-of-block publication or a pending-value commit at the next boundary. Pedestrians does not need that second phase, so this probe does not introduce an `endBlock()` API pre-emptively.

The library should not impose one global clock on every model. A nested submodel may share its parent's time origin, or deliberately have its own. Independently created clocks align only when their origins and advancement agree. Parent/child ownership, inserting a new object mid-stream and paused child evaluation need an explicit later contract; duplicating a helper does not reproduce PD's shared canvas schedule.

PD defaults to 64-sample DSP blocks and permits local changes with `block~`. Its message cascades and audio blocks have scheduling rules distinct from simple per-sample C++ evaluation. Reblocking can affect feedback, and some signal connections impose ordering-dependent delays. **This proposed `pd::block` is a clock/action helper, not a complete port of `[block~]`.** It does not provide overlap, resampling, canvas graph ordering, named signal buses or their storage. [PD's theory of operation](https://msp.ucsd.edu/Pd_documentation/2.theory.of.operation.htm) supplies that context; the project baseline remains the installed PD 0.55.2.

Changing this clock to 128 also does not automatically reconfigure the existing `pd::osc`, `pd::line` and `pd::env` internals, which currently contain their own tested sizes. Passing a block length into those algorithms, where appropriate, is separate work. Here the variable-size probe establishes callback scheduling, not whole-model equivalence to arbitrary reblocked PD patches.

## 8. Decision and evidence

**Pedestrians' preferred author-facing target is the conditional `pd::metro` ideal in section 1.** First explore whether its timing requirements can stay inside the metro. For models that need an explicit block event, the preferred candidate remains C: `pd::sound` supplies the state and an optional `block()` event. Keep a standalone `pd::block` available for ordinary `Sound`/component authors and multiple independent clocks. Default the configured size to 64; let the default handler disable itself after the first runtime call. Reserve zero for explicitly disabling events, while audio processing continues. Let authors override `block()` without a separate enable flag, clock declaration or constructor.

On the current header, **B is the safe immediate implementation if we insist on retaining `process()` syntax everywhere**. The automatic buffer-only prototype is suitable for exploring Pedestrians, but must not be promoted as a universal nested-sound facility. The working `sample()` alternative makes the remaining design choice concrete: accept one renamed method in PD-oriented models, or add a reviewed common evaluation hook to preserve existing Klang syntax. A prototype does not justify imposing an unreliable automatic API on the collection.

| Form | Extra clock declaration | Extra work in sample body | Works nested on current Klang? |
| --- | --- | --- | --- |
| Revised conditional ideal | None | Ordinary metro condition | Proposed; no buffer-only dependency in its presentation |
| Earlier sample-timed callback baseline | None | Ordinary metro call | Yes; tested |
| A: developer-owned member | `pd::block blocks;` | Condition or immediate action | Yes |
| B: inherited member | None | Condition or immediate action | Yes |
| C: automatic buffer event | None | None; new `block()` handler | No; demonstrated failure |
| C with `sample()` dispatch | None | None; rename sample method | Yes |
| C with a future common evaluation hook | None | None; keep `process()` | Requires core integration |

The [probe source](../../../experiments/klang/pd-block/probe.cpp) checks the self-disabling/empty default policies, sizes 0/1/64/128, a runtime size change, persistent phase across unequal host buffers, callback/lambda forms, exact-boundary deadlines and multiple metro events within a block. It compares member, inherited, automatic-buffer and automatic-sample presentations against the current validated Pedestrians model at 48 and 44.1 kHz, with continuous and stop/restart recipes. Nested evaluation is exercised separately.

Measured results and reproduction commands are in the [probe README](../../experiments/klang/pd-block/README.md). The 72 prototype comparisons and four cached PD comparisons are sample-identical. The earlier sample-timed callback baseline (`IdealPedestrians` in the probe) is also identical at 48 kHz for the default period, but gives a raw relative residual of **−18.43 dB at 44.1 kHz**. That large change in a null comparison reflects shifted gate edges; it is not a measured perceptual penalty. These checks establish the explored syntax and timing, not a perceptual benefit, CPU improvement, portable optimiser result or complete PD block implementation. They do not validate the revised boolean-returning `pd::metro` API. The sample-timed baseline remains an explicit counterexample for later listening.

The next experiment should implement the conditional metro presentation and compare its start/stop and timing behaviour, then decide whether Pedestrians needs any visible block handling. For models needing automatic block events, the desired **sample method spelling and common evaluation entry** remain open decisions. The combined phone model supplies a useful second case for pending control publication. Bell/police routing-delay adaptations remain a separate question about effective acoustic delays and feedback.
