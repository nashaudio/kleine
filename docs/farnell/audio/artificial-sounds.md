# Artificial Sounds extended trial

> **Topology review, 16 September 2026:** retained audio, controls and validation claims below describe the pre-review implementation. Current pure-form changes and pending listening decisions are documented in [the topology review](../reviews/KLANG-TOPOLOGY-REVIEW.md). Original recordings remain unchanged.

Chapters 24–28 are implemented, including the bulk supplements: **37/37 inventory items** within the validation scope below. The extended trial has **39 paired fixtures at each of 48 and 44.1 kHz**, plus the existing chapter 25 comparisons. All pairs except the deliberately sample-timed Pedestrians cases at 44.1 kHz have raw residuals below −100 dB or sample-identical output, with level differences below 0.002 dB. Pedestrians now checks bounded gate-timing differences and identical overlapping carrier samples at that rate; PD block delivery is deferred. These are reproducible numerical comparisons, not a claim to have reproduced every author performance or an independent listening verdict.

## Listening files

All delivered bounces are mono float32 at 48 kHz, unity render gain. Website excerpts preserve the original stereo PCM16 samples exactly. No normalisation, resampling, fades or post-processing is applied. Performances and spacing differ between the website recording and the scripted bounces.

| Practical | Klang implementation | Website example | PD bounce | Kleine bounce |
| --- | --- | --- | --- | --- |
| 24 — Pedestrians | [pedestrians.k](../../../farnell/klang/Artificial%20Sounds/Pedestrians/pedestrians.k) | [WAV](../../../farnell/audio/24-pedestrians/pedestrians-online.wav) | [WAV](../../../farnell/audio/24-pedestrians/pedestrians-pd.wav) | [WAV](../../../farnell/audio/24-pedestrians/pedestrians-kleine.wav) |
| 25 — Phone tones | [phonetones.k](../../../farnell/klang/Artificial%20Sounds/Phone%20Tones/phonetones.k) | [WAV](../../../farnell/audio/25-phone-tones/phone-tones-online.wav) | [WAV](../../../farnell/audio/25-phone-tones/phone-tones-pd.wav) | [WAV](../../../farnell/audio/25-phone-tones/phone-tones-kleine.wav) |
| 26 — DTMF tones | [dtmftones.k](../../../farnell/klang/Artificial%20Sounds/DTMF%20Tones/dtmftones.k) | [WAV](../../../farnell/audio/26-dtmf-tones/dtmf-online.wav) | [WAV](../../../farnell/audio/26-dtmf-tones/dtmf-pd.wav) | [WAV](../../../farnell/audio/26-dtmf-tones/dtmf-kleine.wav) |
| 27 — Alarm generator, studies 1–7 | [alarmgenerator.k](../../../farnell/klang/Artificial%20Sounds/Alarm%20Generator/alarmgenerator.k) | [WAV](../../../farnell/audio/27-alarms/alarms-online.wav) | [WAV](../../../farnell/audio/27-alarms/alarms-pd.wav) | [WAV](../../../farnell/audio/27-alarms/alarms-kleine.wav) |
| 28 — Police, website wiring | [police.k](../../../farnell/klang/Artificial%20Sounds/Police/police.k) | [WAV](../../../farnell/audio/28-police/police-online.wav) | [WAV](../../../farnell/audio/28-police/police-pd.wav) | [WAV](../../../farnell/audio/28-police/police-kleine.wav) |
| 28 — Police, historical/bulk wiring | Same model, historical option | Same recording | [WAV](../../../farnell/audio/28-police/police-legacy-pd.wav) | [WAV](../../../farnell/audio/28-police/police-legacy-kleine.wav) |

The alarm montage concatenates the seven separately retained studies, with 0.5 s gaps. Each study is also available as `27-alarms/alarm01` through `alarm07`, with `-pd.wav` and `-kleine.wav` suffixes. The [final programmed alarm](../../../farnell/audio/27-alarms/alarm07-kleine.wav) runs all ten parameter lists, including the two-part wrong buzzer; its [website excerpt](../../../farnell/audio/27-alarms/alarm07-online.wav) covers the corresponding later passage. The native eight-control [earlier alarm bank](../../../farnell/audio/27-alarms/alarm-bank-kleine.wav) and [four-phase prototype](../../../farnell/audio/27-alarms/alarm15-kleine.wav) have separate bounces.

The bulk [remote ringback](../../../farnell/audio/25-phone-tones/ringback-bulk-kleine.wav), [combined phone demo](../../../farnell/audio/25-phone-tones/phone-effects-kleine.wav), [call recogniser/ringers](../../../farnell/audio/26-dtmf-tones/call-recogniser-kleine.wav), [earlier dialler](../../../farnell/audio/26-dtmf-tones/dtmf-bulk-kleine.wav) and [two-tone study](../../../farnell/audio/26-dtmf-tones/dtmf-study-kleine.wav) each have a matching `-pd.wav` file. Their references are distinct from the website's final patches.

## Evidence and findings

[Pedestrians listening review](24-pedestrians/README.md): gate phase explains the main online transition difference. An actual PD 0.42-5 executable matches the recording to approximately its PCM16 noise floor after phase adjustment, without fitted gain or EQ. Thirty-minute simulations also measure the slow phase drift. The review retains transition plots and separate phase-adjusted listening variants.

[Measurement tables](comparisons/artificial/tables.md), [raw runs and event recipes](../../../farnell/audio/comparisons/artificial/render-results.json), and the [manifest](../../../farnell/audio/artificial-manifest.json) retain source hashes, exact controls/times, audio hashes, CPU/memory observations and platform details. Plots include [pedestrians](../../../farnell/audio/comparisons/artificial/pedestrians.png), [DTMF](../../../farnell/audio/comparisons/artificial/dtmf.png), [all alarm studies](../../../farnell/audio/comparisons/artificial/alarms.png), [programmed alarms](../../../farnell/audio/comparisons/artificial/alarm07.png), and both police laws: [website](../../../farnell/audio/comparisons/artificial/police.png) / [historical](../../../farnell/audio/comparisons/artificial/police-legacy.png). FFT 16384/Hann/hop 1024 remains the default; pedestrians, DTMF and programmed alarms also have `-2048.png` transient views.

- **Message timing:** `line~` rounds its duration down to whole DSP blocks, with a minimum of one block. At 48 kHz, the requested 1 ms DTMF fade therefore lasts 64 samples. Shared scripts put controls on the same grid in both renderers. The PD wrapper schedules a quarter-sample inside the intended block to avoid float millisecond rounding moving an exact-boundary message one block early.
- **Restart and retrigger:** crossing/alarm stop–restart tests preserve the PD counter and its held gate. Stopping the metro can leave a tone on; it is not a mute command. DTMF includes rapid retriggers and repeated keys. The final alarms preserve oscillator phase between programmes.
- **Police reference discrepancy:** the website `logosc.pd` and `logosc_graph.pd` wire the shaped signal to the left `pow~` inlet; bulk `POLICE/logosc~.pd`, `police00.pd` and `police1.pd` wire the constant there. On modern Vanilla these are different waveforms. Both are implemented and compared explicitly. `police-legacy` selects the constant-base interpretation; it does not run an old PD executable. The book's prose on p. 360 describes a constant base, while its nominal sweep range fits the current website law better.
- **Recording evidence:** in the first ten seconds of `police.wav`, the website law has a median spectral-shape difference of about **0.78 dB**, compared with **4.73 dB** for the bulk law, after separately fitting spectral gain over active 200–6000 Hz bins. The fitted website gain is about **+3.97 dB**. This supports preferring the website law for this recording; performance/timing and recording gain remain separate uncertainties. It does not establish Andy's PD version or prove post-processing. The [analysis definition and values](../../../farnell/audio/comparisons/artificial/police-reference.json) are retained; fitted gain is never applied to the WAVs.
- **Echo routing:** the police environment's three feedback paths each include one additional 64-sample routing block. Its impulse matches exactly at both rates. Computing delay samples in PD's operation order also matters at the 165 ms half-sample rounding boundary at 44.1 kHz.
- **Decoder:** the default Hann envelope detector and all eight binary tone-detector states match the PD fixtures. Native keypad decoding accepts one active row/column; the combined demos use the digit subset wired in their sources. The `sand` abstraction belongs to this decoder family despite its ALARMS directory. The standalone `dtmfdec.pd` display routes letter/star/hash messages through a redundant float branch of `t f a`, producing PD console errors. These do not affect the captured detector states; the raw logs retain them, and the runner rejects unexpected PD errors. Klang exposes the decoded key directly instead of copying the display plumbing.
- **Missing external abstractions:** `phone-effects.pd` and `dt-call-recogniser.pd` require absent `list-emath`/`list-dotprod` objects. Render copies replace only their number-matcher canvases with [number-match.pd](../../../tests/pd/number-match.pd), a documented Vanilla reconstruction of sequential equality and the five-second inactivity timeout. Klang uses `DTMFTones::Number` directly. Tests cover all three expected numbers, a wrong number, timeout resets, keypad audio and all manual ringers. Validation of these two items is against that reconstructed reference, not an unmodified working original.

## API and validation scope

| Model / component | Controls |
| --- | --- |
| `Pedestrians` | `set(param on)` controls the metro through `metro = on`. The patch's values are hardcoded: 2500 Hz, 100 ms, 0.2. |
| `DTMFTones` | `dial(char)` accepts `123A456B789C*0#D`; `set(durationMs, gain, filter)` defaults to 200 ms, 0.25, highpass enabled. `tones(highHz, lowHz)` and `gate(value)` support the initial study. |
| `DTMFTones::Detector` / `Decoder` / `Number` | Default-window tone detection, keypad interpretation and fixed-length digit matching. These preserve the tested simple detector, not a general robust telephony decoder specification. |
| `AlarmGenerator(study)` | Studies 1–7. For 6/7: `set(durationMs, cycles, hz1, hz2, hz3, hz4, spectrum)`, then `trigger()`. `start(bool)` controls studies 1/5. Nested `Bank` implements the eight-control bulk prototype. |
| `Police` | `set(sweepHz, historical, ambience)` defaults to 0.1 Hz, website law, echoes enabled. `LogOsc`, `Horn`, and `Environment` are nested components. |
| `PhoneEffects(recogniserOnly)` | Keypad via `dialler.dial(char)`, manual ringer via `ring(0..2)`, and the documented number matching. False selects the full phone; true selects the standalone recogniser's differing ringer parameters. |

Construct models after setting the sample rate. Controls are delivered at 64-sample boundaries; the host calls `prepare()` at each buffer. The new `pd::line` is specifically the block-64 ramp, and `pd::env` implements the default 1024-sample Hann window with a 512-sample hop. Live sample-rate changes, arbitrary PD block sizes, generic message scheduling, GUI ports and a full external library are outside this trial. Display-only police patches are covered by their waveform outputs and external spectrum analysis; this is not an `rfft~` port. Scratch control-state WAVs and over-unity graph waveforms remain under `build/`, outside the listening bundle.

## Reproduce

Build Kleine Release as described in the [root README](../../../README.md), then run from the repository root:

```text
python tools/check_pd_primitives.py
python tools/render_artificial.py --retain
python tools/render_artificial.py --rate 44100
python tools/package_artificial.py
python tools/catalogue_farnell.py
```

Use `--cases` for a subset. Direct `kleine --render dtmf ...` and `--render alarm07 ...` supply small default event sequences; the full retained performances use the TSV script generated from the Python recipes. Each TSV row contains an absolute frame, action and numeric values; the frame must be a multiple of 64. The existing chapter 25/29 bounces remain reproducible with `tools/render_farnell.py` and `tools/package_farnell.py`.

Original code/audio attribution is Andy Farnell, *Designing Sound* (MIT Press, 2010). New work and recording provenance remain under the [project licensing position](../../../README.md#licensing-and-attribution); the source-code archive's permission is not assumed to license the recordings. PD-derived primitives retain [Pure Data's BSD notice](../../licenses/Pure-Data-BSD.txt). The private book and its extracts are not included.
