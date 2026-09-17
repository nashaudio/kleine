# Optional audio-thread scheduling

Status: **working prototypes for review, 16 September 2026**. No changes to
`include/klang.h`, PD primitives, the engine or production models. The existing
Klang build therefore retains its behaviour, syntax and performance.

## Recommendation

Keep the requested `every(100, &Alarm::tick)` syntax, milliseconds by default,
and idempotent registration in `prepare()`. Use a branch once per buffer to
select ordinary sample processing or processing in spans between deadlines.
The function-pointer alternative has no consistent measured advantage.

Prefer optional timer storage reserved before audio starts if this moves into
every `Plugin`. The compact prototype adds **24 bytes** to an unused Sound;
an eight-timer bank is **992 bytes**, allocated once. A fixed inline bank needs
no allocation but adds **1,008 bytes to every instance**, even when unused.
Capacity is a template argument in both prototypes; eight is a trial default.
The existing Sound is already 622,736 bytes on this build, mostly unrelated
plugin storage. The timer cost should still be assessed in absolute bytes.

An opt-in base is usable now within the experiment. Promotion into Plugin needs
a host startup/reservation contract and a decision about automatic nested
evaluation; neither is silently changed here.

## Author-facing syntax

Inside `namespace scheduling_trial`, or with that experimental Sound explicitly
selected in the model's own namespace:

```cpp
// A held control value changes every 100 ms.
struct Alarm : Sound {
    pd::osc osc;
    signal gate = 0;

    void prepare() override {
        every(100, &Alarm::tick);
    }
    void tick() { gate = 1 - gate; }
    void process() override {
        osc(800) * gate * 0.2f >> out;
    }
};
```

The only change for explicit units is the first argument:

```cpp
every(milliseconds(100), &Alarm::tick);
every(samples(64), &Alarm::tick);
```

These are alternative declarations of the same timer, not two independent
registrations. A timer's identity is its target object and typed member pointer.
`samples(64)` is a tagged duration: it retains 64 samples if the sample rate
changes. It does not eagerly turn samples into a fixed millisecond duration.

The compact storage variant uses the same registration syntax. Reserve in its
constructor or through the host before audio starts:

```cpp
struct Alarm : CompactSound {
    Alarm() { reserveTimers(); }
    // Same prepare(), tick(), process() as above.
};
```

Reservation is idempotent. `every` never allocates; without reservation this
variant returns `Result::unavailable`. Failure is also retained in
`timerError()`. Exhaustion returns `Result::full`, without reallocating or
overwriting another handler. Production startup should check reservation and
registration results before beginning playback.

See [examples.h](../../../../experiments/klang/scheduling/examples.h) for sample timing, sparse filter configuration,
an explicit PD control window, and a parent scheduling a child's handler.

## Timing and update contract

- Handlers run on the calling audio thread, before input/sample processing at
  their delivery sample. There is no worker thread or wall-clock timer.
- First call is after one interval. `First::immediately` instead runs before the
  first rendered sample. A zero-length buffer prepares but delivers no callbacks.
- Deadlines carry across host buffers. Fractional deadlines are delivered at
  the first sample at or after the deadline. An integer-boundary correction of
  four double-precision ULPs removes arithmetic roundoff, not a fitted shift.
- Periods use an anchored occurrence count, not repeatedly rounded sample
  increments. A million 1 ms events at 44.1 kHz match an integer-rational oracle.
- If several handlers land on one sample, **first-registration order wins**,
  including different fractional deadlines that round to that sample. Repeating
  or reordering registrations in later `prepare()` calls does not reorder them.
- Same object, method, interval and unit: no-op; no reset or repeated conversion
  of milliseconds into samples. `First` only affects a new registration.
- Changed interval: keep the pending deadline, then use the new period. For
  example, a 10 ms timer changed to 7 ms at 11 ms fires at 20, 27, 34 ms.
- `Change::restart` on a changed interval instead starts that new interval from
  the update sample. Repeating an unchanged registration remains a no-op.
- A callback can change an existing timer or cancel one. Its own next deadline
  is already pending when it runs: `keepNext` preserves that deadline too.
  Cancelling a later coincident handler prevents that later call on this sample.
- `cancel(&Alarm::tick)` retains the slot and its precedence. Registering it
  again reactivates it after one interval. Conditional omission in `prepare()`
  does not cancel a timer. Cancelled identities continue to consume capacity.
- New registrations during a callback, and all timer mutation from `process()`,
  are rejected. The next audio span has already been chosen. Register/update in
  `prepare()`, between buffers, or update existing timers inside handlers.
- Finite intervals from one sample through 2^40 samples are supported. Zero,
  negative, sub-sample, non-finite intervals and null members are rejected.
  This bounds callbacks to at most one per timer per sample.
- Valid sample-rate changes at buffer boundaries preserve the remaining wall
  time for ms timers and remaining sample count for tagged sample timers. This
  does not promise that other Klang DSP objects support live rate changes.
  An invalid new rate leaves the queue unchanged, renders audio with callbacks
  suspended and sets `timingValid()` false; the host must resolve that condition.

All changes are audio-thread-owned. UI/MIDI threads must deliver changes through
the host's event mechanism. Targets must remain alive, callbacks must not throw
or block, and active models are not copyable/movable because registrations bind
their object identity. Free-function, const-member and captured-lambda overloads
are not part of this prototype; typed no-argument, void member handlers are.

## Dispatch and composition

[scheduler.h](../../../../experiments/klang/scheduling/scheduler.h) implements the same bounded queue behind two buffer
dispatch choices and two storage policies. With no registrations, the audio
loop has no timer polling. With timers, one linear scan of the small bank finds
the next deadline; normal samples run as a contiguous span. Deadlines are cached
as integer delivery frames so scanning does not repeat floating-point rounding.
It scans again at an event or buffer boundary, not per sample. Registration is
O(capacity), interval updates have bounded constant work after lookup, and no
sorting, locks, `std::function`, or heap operations occur while rendering.

This adapter overrides the normal mono/stereo `process(buffer)` path. A custom
override of that whole method, direct calls to a model's `process()`, or nested
signal evaluation can bypass it. The probe explicitly demonstrates the nested
limitation; it is not claimed as working automatic child scheduling.

A parent can instead share its clock explicitly:

```cpp
void prepare() override {
    every(100, child, &Child::tick);
}
void process() override {
    child >> out;
}
```

That form is implemented and tested. It gives related components deterministic
ordering on one clock and leaves ordinary child signal routing intact. Fully
automatic independent child timers would need a common evaluation hook in the
core; solving that by renaming `process()` would violate the requested syntax.
Synth/note lifecycle integration and copying/rebinding need a separate review
before core promotion.

## PD block experiment

`BlockAlarm` explicitly registers:

```cpp
every(samples(64), &BlockAlarm::block, First::immediately);
```

Its handler advances an experimental control clock through the half-open
interval `[blockStart, blockEnd)` before rendering that block's samples. All
messages in that window update the held gate. It uses the earlier
[experimental Metro](../../../../experiments/klang/pd-block/prototypes.h), not a new production PD primitive.

This reproduces the retained **PD 0.55.2 alarm01 WAV sample-for-sample at both
44.1 and 48 kHz**, even with irregular host buffer partitions. The sample-timed
alarm matches the retained Kleine rendering at both rates. A separate fast
clock check verifies that several control events within one block are retained.

Calling a single tick every 64 samples would not be equivalent. Full PD support
still needs chronological merging of separate control clocks within the window,
synchronous feedback, message timestamps for objects such as vline~, and
reblocking. This experiment establishes a useful explicit boundary mechanism;
it does not establish a complete PD scheduler or justify adding block timing
to accepted models.

## Validation and performance review

[probe.cpp](../../../../experiments/klang/scheduling/probe.cpp) passes **2,781 assertions in Release and Debug /RTC1**:
three rates; three dispatch/storage variants; irregular buffers; unchanged
untimed audio and preparation count; zero buffers; default/immediate first call;
idempotence; registration order; fractional deadlines; interval changes;
cancellation; capacity/error handling; live timer rate changes; multiple
inheritance and virtual member dispatch; mono/stereo routing; parent/child
registration; callback thread identity; allocation checks; one million ticks;
and explicit block delivery. Repeated trace comparisons contribute most of the
assertion count; this is not a claim of 2,781 distinct scenarios.

[report.py](../../../../experiments/klang/scheduling/report.py) verifies four audio comparisons against cached references,
checks PD patch hashes, and records source/executable hashes and measurements
in [results.json](../../../../experiments/klang/scheduling/results.json). C++ `new`/`new[]` interception reports **zero
allocations during first prepare and repeated rendering/interval changes** with
fixed storage or previously reserved storage. Reserving twice allocates once.

The [benchmark table](measurements.md) compares the original Klang loop, both
dispatchers, no timers, one/eight timers, repeated interval changes, and sparse
coefficient updates. Seven interleaved repeats cover buffers of 32, 64, 256 and
1,024 samples on an i7-10700, MSVC 19.51, `/O2 /fp:precise`, one voice at 48 kHz.
Each repeat renders 4,194,304 samples after a 262,144-sample warm-up per case;
the benchmark thread is pinned to logical CPU 2. Shorter unpinned exploratory
runs showed interference spikes, motivating these longer measurements.
The 1 ms timer workload is intentionally much denser than `every(100, ...)`.

Findings:

- In the oscillator workload the unused compact scheduler is close to the
  original loop. The very cheap recurrence workload exposes several-percent
  differences from generated code/layout too. **No universal zero-cost claim.**
- One 1 ms handler adds measurable dispatch cost in the oscillator workload;
  eight staggered handlers cost more, particularly with small host buffers where
  repeated registration also matters. Neither dispatcher consistently wins.
- Repeated interval changes add a small bounded cost; equality avoids timer
  reconstruction, allocations and sorting. Changes are not literally free.
- Moving an expensive, constant coefficient calculation to every 10 ms is over
  twice as fast in this probe and comparable to or faster than a hand-written
  sample counter. A changing coefficient would be an audible model choice, not
  a behaviour-preserving optimisation by default.

These are DSP microbenchmarks, not PD-versus-Klang CPU claims. Per-case timing
excludes construction, file I/O and process startup. Aggregate process CPU is
recorded separately. Fixed object/bank sizes are more informative here than
whole-process resident memory. Cross-compiler/platform benchmarking and a real
core-integration benchmark remain necessary before promoting an API.

## Reproduce

From an x64 Visual Studio developer command prompt at the repository root:

```text
mkdir build\scheduling
cl /nologo /std:c++17 /EHsc /O2 /fp:precise /DNOMINMAX /wd4244 /wd4305 /Iinclude experiments\klang\scheduling\probe.cpp /Fobuild\scheduling\probe.obj /Febuild\scheduling\probe.exe
cl /nologo /std:c++17 /EHsc /Od /RTC1 /fp:precise /DNOMINMAX /wd4244 /wd4305 /Iinclude experiments\klang\scheduling\probe.cpp /Fobuild\scheduling\probe-debug.obj /Febuild\scheduling\probe-debug.exe
cl /nologo /std:c++17 /EHsc /O2 /fp:precise /DNOMINMAX /wd4244 /wd4305 /Iinclude experiments\klang\scheduling\benchmark.cpp /Fobuild\scheduling\benchmark.obj /Febuild\scheduling\benchmark.exe
python experiments/klang/scheduling/report.py
```

The report requires the project's existing Python audio-analysis dependencies
and the cached alarm01 fixtures under `build/alarm-review`. If missing, generate
them with `tools/render_artificial.py --cases alarm01 --rate 44100 --review
--output-root build/alarm-review` and again with `--rate 48000` after building
Kleine Release. Scratch executables, CSV timings and raw audio stay in `build/`.
