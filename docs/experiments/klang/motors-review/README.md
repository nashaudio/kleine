# Legacy FourStrokeEngine and Car review

17 September 2026. Listening found high-frequency crackling and visible
waveform discontinuities in Car and FourStrokeEngine. The change from Basic to
integer-phase Phasor made no perceptible difference and is not the cause.

The defect is model-local. FourStrokeEngine recalculates this delay scale on
every sample:

```cpp
const signal ms = fs / 250 * random(0.99, 1.0);
```

At 48 kHz the longest tap can jump by roughly 38 samples between adjacent
output samples. Klang's Delay interpolates each requested position correctly,
but discontinuous random read-position changes create broadband clicks.

The retained [probe](../../../../experiments/klang/motors-review/probe.cpp) consumes the identical RNG sequence while
holding the tap scale at `0.995`. In Car's high-speed section, sample jumps over
0.02 fall from 120 to the one deliberate section-boundary control change; jumps
over 0.05 fall from 23 to zero. The remaining large raw FourStroke transitions
occur at the explicit speed changes at 2 and 4 seconds.

Decision: retain both models as historical/reference source, mark them
known-broken and superseded by Mini, and exclude Car/FourStrokeEngine from the
production acceptance corpus. ToyBoatEngine remains active. A future repair
should choose jitter per engine event/cycle or use smoothly varying delay
modulation rather than audio-rate independent randomness.
