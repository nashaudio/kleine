# pd::vline

Port of PD 0.55-2's `vline_tilde_*` message, queue and sample-ramp algorithms in
[d_ctl.c](https://github.com/pure-data/pure-data/blob/0.55-2/src/d_ctl.c), with PD's
32-bit target sanitisation. Attribution and BSD terms remain in
[pd.h](../../include/klang/pd.h) and [the licence](../../licenses/Pure-Data-BSD.txt).
[Source hashes](sources.json) identify the inspected revision.

## Interface

```cpp
pd::vline envelope;
// PD message: 1 1 0, 0 200 1
envelope.set(1, 1);
envelope.set(0, 200, 1);
// Once per sample:
signal amplitude = envelope;
```

| PD input | Klang |
| --- | --- |
| Float target | `set(target)` |
| Target and ramp duration | `set(target, durationMs)` |
| Target, duration and initial delay | `set(target, durationMs, delayMs)` |
| Middle inlet | `duration(ms)` |
| Right inlet | `delay(ms)` |
| Stop | `stop()` |

Both cold inlets are cleared by each hot message and by `stop()`. A two-element
list preserves a previously supplied cold delay until that message consumes it.
Delay is relative to message time, not the preceding segment's end. There are no
PD creation arguments. Klang's virtual setter dispatch supports all three arities.

The queue has no fixed segment limit. A new segment discards the queued suffix
starting at or after its start time, except that an instantaneous jump followed
by a nonzero ramp at the same time is preserved. Negative durations become zero;
negative delays clear all segments and jump immediately. `stop()` freezes PD's
internal next-sample value, which can differ from the last emitted sample during
a ramp. Queue allocation/deallocation follows PD's message-driven behaviour;
this is not an allocation-free audio-thread design.

## Time and host scheduling

Ordinary models use the sample clock, starting at zero. `time()` reports the next
sample's time in milliseconds, derived from the sample count rather than repeated
addition. This prevents clock drift from changing coincident-message ordering in
Bouncing's repeating zero-height tail.

`messageTime(absoluteMs)` supplies a logical timestamp for subsequent messages;
the next sample evaluation clears that override. This preserves fractional times
from external schedulers without adding timing controls to a model's `set()`.

`sync(absoluteMs)` explicitly switches to a host-anchored audio clock. It sets the
next sample time while preserving active and queued ramps. That clock advances
using PD's per-sample arithmetic; call `sync` again at each host clock boundary.
The isolated driver reproduces PD's block anchoring separately from the primitive:
`m_sched.c` computes `64 / sampleRate` in **float** before converting to logical
clock units; `d_ctl.c` anchors a block one block-length before that logical time.
At a discontinuity precisely on a sample boundary, this rounding can decide which
sample contains the jump. No output shift, resampling or gain fitting is used.

The model files do not call `sync` or contain a new 64-sample workaround. PD canvas
scheduling, reblocking and DSP pause/resume remain host responsibilities; this
port supplies the complete documented ramp/message interface, not a PD runtime.
Set `klang::fs` before construction and keep the rate fixed during a run.

## Verification

Build from an x64 MSVC developer shell at the repository root:

```text
cl /nologo /std:c++17 /EHsc /O2 /fp:precise /DNOMINMAX /Iinclude tests/pd/vline.cpp /Febuild/vline-review/vline.exe /Fobuild/vline-review/vline.obj
python tools/check_pd_vline.py
```

Create `build/vline-review` first. The [PD fixture](vline.pd) and
[C++ driver](vline.cpp) cover floats/lists, cold-inlet consumption, ordered queues,
suffix replacement, coincident jumps/ramps, active interruption, stop/restart,
negative times, target sanitisation, fractional messages and multiple segments
within a sample. The driver also checks nonfinite targets and a ten-second
sample-timed zero-height tail followed by a new attack.

[Results](vline-results.json) record 25 cases at each of 48 and 44.1 kHz using
PD-Vanilla 0.55.2, block size 64, 8192 samples per case. Each record includes raw
peak/RMS, residuals, messages, timing, commands, CPU time, elapsed time and process
memory estimates. There is no randomness. Timings include process startup and
file I/O; they are not isolated DSP benchmarks. The numerical error limit is
2e-6; bit-identical results are a diagnostic for this explicit host-clock setup.
All 50 retained cases are sample-identical with that setup.

## Model migration

`Gesture` has been removed. Bouncing, Rolling (including the earlier uneven
study), Creaking, Boing and the early bell studies use `pd::vline` with the PD
patches' target/duration/delay messages. Fractional scheduler timestamps remain
explicit in Creaking, uneven and bell cadence code. The older TelephoneBell
`Decay` helper is separate and has not been changed by this migration.

Legacy Bouncing also now follows the `pow~` guard in
[d_arithmetic.c](https://github.com/pure-data/pure-data/blob/0.55-2/src/d_arithmetic.c):
a negative base with a fractional exponent produces zero. A tiny negative ramp
endpoint otherwise sent NaN into the oscillator through `std::pow`. The envelope
itself is not clipped; the source power-domain rule is applied at that operation.

After building Kleine, reproduce the model comparisons with:

```text
python tools/review_vline_models.py
python tools/review_vline_models.py --before build/vline-review/before/kleine.exe
```

The optional baseline is an executable saved **before** the port. The
[model report](vline-model-results.json) compares the established recipes against
PD and that baseline at both rates. Previous model-level timing/routing differences
remain marked for the agreed listening review. Scratch WAVs stay under `build/`;
retained listening files are untouched. AlarmGenerator::Bank's `sum` accumulator
was also replaced by `out`; its before/after samples are checked for equality.
