# DTMF Tones review

16 September 2026. Coverage records **6/6 active patch entries complete (100%)**,
plus one discarded alternative (`exc`). This includes the accepted sounds and
the separately validated decoder/Sand message contracts. The remaining 44.1 kHz
block-timing review is distinct from this stated completion scope.

## Fresh PD comparisons

Current Release executable against PD 0.55.2, 64-sample reference blocks, raw
float32 output with no alignment or level matching. Existing shared recipes cover
all sixteen keys, retriggers, isolated detector states and complete number matches.
The latest topology-revision audio/results are under
`build/env-publication/models/{48000,44100}/CASE/`.

| Case | 48 kHz | 44.1 kHz |
| --- | --- | --- |
| Main dialler / louder variant | Numerical parity | Residual -26.75 dB; level +0.02027 dB |
| Unfiltered oscillator study | Numerical parity | Numerical parity |
| Main dialler retriggers | Numerical parity | Residual -30.36 dB; level +0.01001 dB |
| Single-tone detector | Exact state agreement | Exact state agreement |
| Eight-detector state bitmask | Exact state agreement | State timing differences; residual -34.53 dB |
| Call recogniser / full-number recipe | Residual -3.54 / -3.64 dB | Residual +0.54 / -0.45 dB |

The detector bitmask is diagnostic data, not listening audio. It validates the
eight detector states rather than the full symbol/message semantics of every
decoder outlet. Numerical residuals are not evidence of audible failure.
The recogniser references reconstruct the missing list-library matchers.

## Decoder control tests

[56 shared-input cases and an isolated sand test](../../../tests/pd/dtmf-messages.md)
now pass at both rates. `DTMFTones::Sand` explicitly implements the abstraction;
`DigitDecoder` composes ten instances for the phone/call-recogniser patches.
`Decoder` separately implements standalone `dtmfdec.pd` with ordinary AND/select
pairs. Both preserve source inlet/connection order. All detector states and
output messages match PD, including overlaps, direct changes and the standalone
graph's repeated held-key messages. The previous unique-row/column policy is gone.

The dialler uses a reusable `pd::del{200}` with bang/retrigger, hot/cold floats,
stop and tempo units, covered by [52 isolated cases](../../../tests/pd/del.md).
Detector uses the current `pd::env::out` for its threshold calculation, with
`signal active` holding the decision and `envelope.updated` read directly.
[18 model regression renders](../../../tests/pd/dtmf-model-results.json) are
sample-identical to the pre-revision executable, including the accepted phone
and call-recogniser performances. No new block adapter was introduced.
The subsequent envelope-output timing repair also preserves all
[18 model renders](../../../tests/pd/env-model-results.json); those fresh files
are under `build/env-publication/models`. The envelope publishes `out` and
`updated` together at the original PD boundary, with a private pending result.
Detector's simplified code remains unchanged. Raw envelope comparisons and
decoder event timestamps now match without a one-sample adjustment.

## Listening results

**Standalone call recogniser accepted:** Chris reports that both current 48 kHz
`call-recogniser` and `call-match` PD/Kleine pairs sound the same. This covers the
personalised ringers and automatic number-triggered performance in those recipes.
The raw numerical residuals above remain diagnostic evidence, not an audible
failure or a reason to add block compensation. Acceptance uses the documented
reconstructed matcher reference; it does not claim an unchanged upstream matcher
or extend this listening result to 44.1 kHz.

**Main dialler accepted at 48 kHz:** Chris reports that the current PD and Kleine
main diallers sound the same; the online recording has more apparent gaps.
Envelope measurements explain the spacing: all three contain 16 tones of about
202 ms including their short ramps/tails. The scripted PD/Kleine sequence starts
one every 300 ms, leaving about 98 ms between tones. Online starts are irregular,
leaving gaps of 267–654 ms (typically 353 ms). This is a difference in supplied
keypress timing, not a missing release in either implementation. Measurements
use 1 ms RMS bins on the left/mono channel, with an activity threshold of 2% of
that recording's maximum bin RMS; timing precision is approximately 1 ms.

The reviewed pairs are in `build/artificial/48000/{dtmf,call-recogniser,call-match}/`.
The 44.1 kHz numerical tests remain available; a separate listening pass at that
rate has not been reported.

The main dialler is already based on the latest companion-site patch. The
`dtmf-bulk` alternative is **discarded**: Chris hears it as identical. It used gain
0.3 instead of 0.25 (about +1.58 dB) and now survives only as a historical test fixture;
`dtmf-study` is an earlier manually gated oscillator study, without the highpass
and with gain 0.125. Neither needs to displace the selected companion-site model
merely because it exists.

As in Phone Tones, perceptual agreement is sufficient for sound acceptance.
The main dialler's local release timer has been replaced. Other phone/alarm
timers, reconstructed number matchers and any audible block-timing need remain
separate reviews.

Reproduce for each rate:

```text
python tools/render_artificial.py --cases dtmf dtmf-study dtmf-retrigger dtmf-detector dtmf-decoder call-recogniser call-match --rate 48000 --review --output-root build/del-review/models
```

Replace `48000` with `44100` for the second run. Retained `/audio` WAVs have not
been overwritten; the fresh build outputs identify the current implementation.
