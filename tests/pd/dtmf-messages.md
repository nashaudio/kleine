# DTMF decoder message tests

16 September 2026. PD-Vanilla 0.55.2, MSVC C++17 `/O2 /fp:precise`, 48 and
44.1 kHz, 64-sample PD blocks. [Results](dtmf-message-results.json) retain commands,
source/executable/input hashes, input recipes and all event frames.

## Source topology

| Source | Klang component | Behaviour |
| --- | --- | --- |
| `decode-tone.pd` | `DTMFTones::Detector` | `bp~ frequency 24 -> env~ -> / 80 -> int -> == 1`; publishes on every envelope update. |
| `sand.pd` | `DTMFTones::Sand` | Both inputs trigger AND; select key or -1, then suppress repetitions with `change`. Initial change state is 0. |
| `phone-effects.pd` / `dt-call-recogniser.pd` decoder | `DTMFTones::DigitDecoder` | Seven detectors, ten Sand instances, shared change, then nonnegative output (`moses 0`). |
| `dtmfdec.pd` | `DTMFTones::Decoder` | Eight detectors, sixteen ordinary AND/select pairs; hot/cold inlet distinction, with repeated messages while a key remains held. |

Each decoder's `messages` contains every output from its latest sample evaluation.
The digit bank emits integers 0-9; the standalone decoder emits keypad characters.
`PhoneEffects` consumes the digit bank's messages immediately after decoding.
The envelope primitive publishes at the following-block boundary; callers no
longer defer the messages for another sample or keep just one pending key.

Independent detector outlets arrive in reverse creation order in these PD graphs.
The Klang loops preserve that order and each outlet's connection order, including
the swapped AND inputs for key 4 and standalone bottom-row order `0, *, #, D`.
These are message connections, not an added block-timing workaround.

Detector output holds the current threshold decision. The chain
`in >> filter >> envelope >> out` evaluates the envelope and supplies its latest
result for the threshold calculation. `signal active` holds the decision between
updates; callers inspect `detector.envelope.updated` directly. There is no copied
update flag, separate envelope `level`, or explicit `envelope.process()` call.
The envelope keeps its pending analysis private, then publishes `out` and
`updated` together at PD's following-block boundary. The
[env output checks](env-output-results.json) compare raw output and event frames
directly. Neither comparisons nor decoder timestamps apply a sample offset.

## Validation

All **56 shared-input audio cases pass**, 28 at each rate:

- All sixteen keys, hold/release, repeat after silence and release of either tone.
- Direct row/column switches, overlapping rows/columns/both, isolated row/column
  and silence.
- All eight detector state traces agree in value and publication sample.
- Both decoder message streams agree in value, count, order and publication
  sample, including overlaps and the standalone decoder's held-key repetitions.

Each case lasts two seconds and includes a silent tail. Both implementations get
the same mono float32 samples: continuous-phase cosines, peak 0.25 per component,
switched on a shared 64-sample grid. No gain fitting, resampling or alignment.
PD's block-boundary events are converted to the nearest block index; truncating
the float timer had incorrectly labelled some 44.1 kHz events one block early.
The standalone patch's known symbol-to-float display warnings do not affect its
unconverted output messages.

The isolated [Sand fixture](sand-messages.pd) sends fourteen messages covering
both input orders, repeats and releases. Both PD and the actual Klang Sand emit
`-1, 1, -1, 1, -1, 1, -1, 1`.

The previous combined decoder rejected multiple rows/columns and suppressed
held-key repetitions. Those behaviours were adaptations, now replaced with the
two source graphs. Ordinary sound acceptance remains separate from this correction.

## Model regression

[18 paired renders](dtmf-model-results.json) cover the dialler, unfiltered study,
retriggers, detector diagnostics, phone effects and call recogniser (including
automatic number matches), at both rates. **Every existing performance is sample
identical to the executable saved before this revision.** These recipes use
separated keypresses; the new overlap/repetition semantics are verified above.
At 48 kHz the dialler, study, retrigger and detector diagnostics also match PD
sample for sample. Existing 44.1 kHz scheduling differences remain recorded.

The dialler now uses [pd::del](del.md) for its fixed 200 ms release. There is no
model-local release timestamp. Other phone/alarm timers and the reconstructed
number matcher remain separate follow-ups; no 64-sample adapter was added.
Fresh audio is in `build/del-review/models`, leaving open/retained listening files
alone. Performance records distinguish process CPU/RSS and elapsed time from
Kleine's processing wall time; no optimisation threshold is imposed.

The `env.out` timing repair repeats these checks in
`build/env-publication/models`. Its [18 before/after renders](env-model-results.json)
retain identical samples; all 56 decoder cases match PD at their actual message
frames. Detector's simplified body and `signal active` are unchanged. The earlier
one-sample comparison projection and caller deferrals have been removed.

## Reproduce

From an MSVC developer shell, after creating `build/dtmf-messages`:

```text
cl /nologo /std:c++17 /EHsc /O2 /fp:precise /DNOMINMAX /Iinclude tests/pd/dtmf-messages.cpp /Febuild/dtmf-messages/decoder.exe /Fobuild/dtmf-messages/decoder.obj
python tools/check_dtmf_messages.py
python tools/render_artificial.py --cases dtmf dtmf-study dtmf-retrigger dtmf-detector dtmf-decoder phone-effects phone-match call-recogniser call-match --rate 48000 --review --output-root build/env-publication/models
```

Repeat the render at 44100. With the saved pre-revision executable, reproduce the
before/after check using `python tools/review_dtmf_topology.py --baseline build/env-publication/before/kleine.exe --renders build/env-publication/models --output tests/pd/env-model-results.json`.
Generated wrappers/audio stay under `build/`; original PD patches are unchanged.
