# Shared PD line / line~ interface

The overloads in [pd.h](../../../include/klang/pd.h) select audio ramps with
`set(target, durationMs)` and control ramps with `set(target, durationMs, grainMs)`.
The latter holds its output between emissions. `updated` reports an emission even
if the value did not change; `updates` preserves its count.

Two-breakpoint notation is also supported:

```cpp
line = { {0, 1}, {3000, 0}, 20 };
line.set({0, 1}, {3000, 0}, 20);
```

Points use `{timeMs, value}`, with the first time zero and a nonnegative finite
last time. Both forms expand to the explicit initial jump and subsequent ramp;
they preserve the two control emissions on the trigger sample. Omitting grain
selects the audio form. Invalid times throw before changing the envelope. This
describes one segment, not an arbitrary breakpoint queue. Bouncing uses the first
form for its height gesture. The point order follows Klang Envelope's x/y order;
time is milliseconds here, consistent with PD and the other line setters.

Control behaviour follows PD 0.55-2 `src/x_time.c`: destination/duration/grain,
nonpositive grain fallback to 20 ms, retargeting from the interpolated current
value, stop, silent `set` (Klang `reset`), one-shot duration inlet and persistent
grain inlet. `legacy` selects the pre-0.48 stop state. Source hash is recorded in
[sources.json](../../../tests/pd/sources.json); original copyright and BSD notice remain in pd.h.

Run `python tools/check_pd_line.py`. Its ten cases at each of 48/44.1 kHz compare
with an isolated [PD reference](../../../tests/pd/line-control.pd). PD delivers control messages
before each audio block, so analysis projects the last C++ sample of each block
across that block. This is not processing inside the port. Initial events share
time zero; later PD messages occur a quarter-sample after C++ events to avoid
float-to-block ambiguity. The resulting bound is 0.0001 for these slopes, while
emission counts must match exactly. See [results](../../../tests/pd/line-results.json).

The existing `check_pd_primitives.py` audio-ramp fixture remains sample-identical
at both rates. [line-modes.cpp](../../../tests/pd/line-modes.cpp) checks arity dispatch, held control
output and switching back to an audio ramp at a block boundary.

## Limits

Audio mode retains the established 64-sample grid contract. This is separate from
the sample-polled control clock and remains part of the upcoming block review.
When multiple control emissions fall within one sample, polling exposes the last
value plus count. There is no synchronous per-emission callback/shared PD message
scheduler. These limits are explicit; the class is not a complete PD runtime.

Two arguments explicitly switch to audio mode; three explicitly select control
mode. A single target retains the selected mode and consumes a pending
`duration(ms)`, or jumps if none was supplied. `reset(value)` is the silent control
message, distinct from Klang's active `set(target)` configuration method.
