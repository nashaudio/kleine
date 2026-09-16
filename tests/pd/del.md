# PD del/delay port

`pd::del` ports the one-shot control clock from PD 0.55-2 `x_time.c`, using the
clock-unit rules in `m_sched.c`. Revisions and hashes are in [sources.json](sources.json);
the [PD BSD notice](../../licenses/Pure-Data-BSD.txt) applies. `klang::Delay` is an
audio buffer and does not supply this message-clock behaviour.

```cpp
pd::del del{200};
del.bang();          // Event: start/restart the stored 200 ms delay.
del = 50;            // Hot float: store 50 and restart.
del.set(100);        // Cold float: change the next duration, leave this deadline.
del.stop();          // Cancel the pending bang.
// Once per sample, including when idle:
if (del()) envelope.set(0, 1);
```

The constructor also accepts `(duration, amount, unit)`; `tempo(amount, unit)`
changes units. Milliseconds, seconds, minutes, samples and reciprocal forms follow
PD's name parsing and float conversion. Negative durations become zero;
nonpositive tempo becomes 1. Invalid units return `false` and select 1 ms.
There is only one pending bang; retriggering replaces it.

## Validation

[52 cases](del-results.json), 26 each at 48 and 44.1 kHz, compare the actual class
with the isolated [PD fixture](del.pd): construction, idle, hot/cold floats, bang,
retrigger, stop/restart, zero/negative/fractional durations, all unit families,
invalid/nonpositive tempo, and tempo changes while pending. Bang counts agree;
deadlines agree within the sample-boundary precision of PD's float `timer` outlet.
The driver allows one later sample when that float rounds a double deadline onto
an integral sample. These are control messages, so audio level/residual metrics
do not apply. No block projection is used in the class.

PD 0.55-2 has a source quirk: when the old unit is samples, the remaining-time
division in `clock_setunit` is negative, so changing units leaves the pending
deadline in place. Both sample-to-time and sample-scale changes reproduce this in
the executable fixture. The port retains that behaviour; it is not an intended
tempo improvement.

The object owns a sample clock. Poll once per sample at a fixed sample rate;
control calls refer to the next poll's sample time. A zero delay is observed at
the next poll. Shared-clock ordering, recursive outlet feedback and PD's delivery
of clocks before a 64-sample audio block remain explicit host-level concerns.
This is a reusable port of the object's message operations, not a global PD
scheduler or a claim of arbitrary-host equivalence.

## Reproduce

From an MSVC developer shell, after creating `build/del-review`:

```text
cl /nologo /std:c++17 /EHsc /O2 /fp:precise /DNOMINMAX /Iinclude tests/pd/del.cpp /Febuild/del-review/del.exe /Fobuild/del-review/del.obj
python tools/check_pd_del.py
```

Generated patches/logs stay under `build/del-review`; retained JSON includes
commands, creation arguments, events, source/executable hashes and timing errors.
