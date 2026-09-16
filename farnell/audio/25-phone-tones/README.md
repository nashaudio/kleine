# Phone Tones completion review

16 September 2026. Fresh comparisons against installed PD-Vanilla 0.55.2,
using the current Release executable at 48 and 44.1 kHz. Original patches and
retained listening WAVs are unchanged. Fresh audio is under `build/phone-review/`
and `build/artificial/`; [raw results](../comparisons/phone-tones-review.json)
include commands, source hashes, levels, residuals and process measurements.

## Results

| Sound | 48 kHz | 44.1 kHz |
| --- | --- | --- |
| Dial 350/440, dial 350/450, dial through line | Sample-identical | Sample-identical |
| Busy, unsmoothed and smoothed | Identical apart from negligible filter tails | Identical apart from negligible filter tails |
| Ringback, both handset paths | Identical apart from negligible filter tails | Transition differences; residual -40.04 / -41.43 dB |
| Pulse dialling, counts 1/5/7 | Identical apart from negligible filter tails | Substantial raw residual: +0.37 dB; RMS level differs by only -0.000022 dB |

Residual is the RMS of the difference relative to the PD signal; it is sensitive
to timing and is not a perceptual score. The insignificant tail errors are around
1e-19 in sample amplitude. The old numerical parity limits pass for 13/16 core
tone/rate cases. All eight core cases pass at 48 kHz.

The variant extraction separately passed all 16 before/after core tone renders.
Thus these differences are not regressions introduced by separating the files.

## Gaps to completion

1. **Sound choices are resolved.** Chris accepts all busy fixtures as perceptually
   equivalent; retain the latest website's smoothed version. Use 350/450 Hz dial.
   Retain the brighter ringback and discard the duller `ringback-bulk` model.
   The selected flows now live directly in `PhoneTones`; earlier versions are
   historical test fixtures only. Agreement between online, PD and Kleine is a
   sound-review pass, without requiring sample equality.
2. **Review ringback and pulse transitions at 44.1 kHz.** The pure ringback metro
   delivers sample-timed events; PD presents control updates at its DSP block
   boundary. Pulse's initial digit is block-aligned by the renderer, but the
   model's subsequent 100/40 ms contacts are sample-timed. At 48 kHz these periods
   are integral multiples of 64 samples; at 44.1 kHz they are not. This is a
   concrete timing discrepancy to isolate and audition before deciding whether
   to add an explicit, separable block adapter. Numerical mismatch alone is not a
   reason to add one if online, PD and Kleine sound equivalent. No adapter was added.
3. **Finish the pulse topology.** `pulseElapsed`/`pulseCount` still simulate the
   patch's `metro 100`, duration delay and `del 40`. Replace that local timer with
   reusable PD objects, including a complete delay primitive, then compare the
   result. Zero counts and digits retriggered before the previous sequence ends
   need dedicated control tests; the present 1/5/7 recipe does not cover these.
4. **Resolve the ringback toggle's public control.** The PD patch includes a
   separate audio mute toggle, held on by the reference wrapper. The current
   model exposes continuous automatic ringback without that toggle. Decide
   whether to expose it or document the intentionally narrower control scope.

The selection update passes a Release build and 16 before/after renders (all eight
current/historical comparison labels at both rates), with identical samples.
The main model's default dial is now 350/450 Hz. The renderer's old `dial` label
still identifies the historical 350/440 fixture; `dial-web` renders the selected
default. See [fixture mapping](../../klang/VARIANTS.md).

Dial/busy choices are complete; the remaining timing and topology items above are
separate from that acceptance. Runtime sample-rate changes and arbitrary device
block sizes were not tested.

## Pulse level investigation

Rechecked against the current build and the latest local companion-site patch.
The website `pulsedial.pd` and original collection patch render **sample-identically**
at 48 kHz with shared controls. Kleine has the same level and waveform, apart from
negligible filter-tail differences. The embedded website handset and collection's
separate `telephone-line.pd` therefore do not explain the recording's extra level.

Across 26 individually matched 100 ms periods, the online recording is **1.972965x
amplitude (+5.902 dB)**. After fitting that single gain, residuals are approximately
-54 dB. This comparison does not depend on Andy dialling the same number: pulse
count/spacing changes the performance, while the individual contact shape matches.
Older highpass behavior, tested using PD 0.55.2 compatibility mode, changes the
patch level by only **+0.103 dB**. It does not account for the recording's increase.

This rules out a Klang gain error or the inspected patch revisions as the cause.
A different playback/capture gain or stereo summing is plausible, but the original
capture path is unknown; post-processing is not established. No compensating gain
has been added. [Measurements and method](../comparisons/reference-levels.md) and
[raw fits](../comparisons/reference-levels.json) retain the evidence.

## Supplemental combined phone demos

**PhoneEffects sound review accepted, 16 September.** Chris compared the four
current build waveforms in Audition: their envelopes look identical, and simultaneous
playback reveals no apparent phasing, echoes or other audible difference. This
acceptance supersedes the earlier pending status based on numerical residuals.
No block workaround is justified by this listening result.

The separate `call-recogniser` model has not received this listening acceptance.
The eight supplement/rate numerical comparisons still exceed the previous raw
parity limits (-13.47 to -3.54 dB at 48 kHz; -10.08 to +0.54 dB at 44.1 kHz).
Those figures are diagnostic evidence, not audible-failure findings.

### Measured PhoneEffects timing

An additional trace compares the manual-ringer/keypad demo and the full-number
matching recipe. Positive offsets mean Klang acts later than PD's receiving DSP
block; these are sample positions, not wall-clock timings.

| Event | 48 kHz offset | 44.1 kHz offset |
| --- | --- | --- |
| Initial ringtone programmes | 0 | 0 |
| Delayed follow-up programmes | 0, except Harry: 32 samples (0.667 ms) | 33–46 samples (0.748–1.043 ms) |
| Alarm timebase resets/endings | 32–48 samples (0.667–1 ms) | 9–61 samples (0.204–1.383 ms) |
| Keypad release | 0 | 52 samples (1.179 ms) |
| Consumption of decoded digits | 0 | 0 or 64 samples (1.451 ms) |

All traced numeric digits and effective ringtone programmes match in count, value
and order. This includes 41 digits in the number-matching recipe. Tom's two
simultaneous follow-up messages are compared by their final effective programme;
the raw PD trace retains both messages. Instrumenting the PD patches did not
change their audio samples. The shifts are consistent with sample-timed control
versus PD block delivery; they do not establish the cause of every waveform
residual or imply an audible defect.

[Raw event traces](../comparisons/phone-timing.json) and
[diagnostic driver](../../../tests/pd/phone-timing.cpp) retain the evidence.
Run `python tools/check_phone_timing.py` after compiling that driver as
`build/phone-timing/trace.exe` and generating the existing phone-effects/phone-match
fixtures at both rates. The diagnostic changes neither model nor original patch.
Matching references reconstruct missing upstream list abstractions; acceptance
therefore applies to that documented reference scope.

## Reproduce

Run these commands for each rate (replace `48000` with `44100` for the second run):

```text
python tools/render_farnell.py --cases dial dial-web dial-line busy busy-archive ringback pulse --rate 48000 --output build/phone-review/48000 --results build/phone-review/48000/results.json
python tools/render_artificial.py --cases ringback-bulk phone-effects call-recogniser phone-match call-match --rate 48000 --review
```

Then collect the report:

```text
python tools/review_phone_tones.py
```

`--review` records failed numerical parity without accepting it. No normalization,
alignment or gain fitting is used. Source SHA-256 values and actual render recipes
are retained in the report. Process CPU/RSS numbers include startup and file I/O;
they are not isolated DSP performance benchmarks.
