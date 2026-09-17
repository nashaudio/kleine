# Alarm sounds review

Coverage is **8/9 complete (88.9%)**: seven website studies and two collection
supplements. All are implemented. Study 1's timing/listening review is accepted;
study 5 remains pending following its move to `pd::metro`.

## Fresh comparison

Current MSVC Release against PD 0.55.2, at 48 and 44.1 kHz, with 64-sample PD
blocks. [Evidence](../../../../farnell/audio/comparisons/alarm-review.json) includes recipes, source and
executable hashes, raw metrics and performance records. No gain fitting or
alignment; existing listening WAVs and model code are unchanged.

| Study | Sound | 48 kHz | 44.1 kHz |
| --- | --- | --- | --- |
| 01 | Single 800 Hz tone, metro gating | Identical samples | NIPD accepted; residual -23.16 dB |
| 02 | Single tone, signal-driven gating | Identical samples | Identical samples |
| 03 | Alternating two-tone alarm | Identical samples | Identical samples |
| 04 | Three tones, split-phase sequence | Identical samples | Identical samples |
| 05 | Three tones, message-driven sequence | Identical samples | Timing review; residual -34.32 dB |
| 06 | Four-phase, shaped-tone alarm | Identical samples | Identical samples |
| 07 | Programmed alarms and presets | Identical samples | Identical samples |
| alarm15 | Earlier four-phase prototype | Identical samples | Identical samples |
| alarm-bank | Earlier three-voice bank | Identical samples | Identical samples |

The separate 01/05 stop/restart fixtures also match exactly at 48 kHz. Their
44.1 kHz residuals are -24.26/-39.21 dB. These numerical differences do not
establish an audible defect or justify adding a block workaround by themselves.

## Alarm 01 — accepted

Source: [alarm01.pd](../../../../farnell/zip/p04/alarm01.pd). An 800 Hz oscillator at gain 0.2,
alternating 300 ms off and 300 ms on, starting silent. Stopping the metro holds
the current gate; it does not necessarily mute the tone.

Fresh four-second 48 kHz listening pair:

- [PD](../../../../build/alarm-review/48000/alarm01/pd.wav)
- [Kleine](../../../../build/alarm-review/48000/alarm01/kleine.wav)
- [Website recording](../../../../farnell/audio/27-alarms/alarms-online.wav) (the complete alarm compilation).

The 48 kHz PD/Kleine pair is sample-identical. Chris reviewed the fresh
[44.1 kHz PD](../../../../build/alarm-review/44100/alarm01/pd.wav) and
[Kleine](../../../../build/alarm-review/44100/alarm01/kleine.wav) pair and reports
**NIPD — no immediately perceptible difference**. This accepts the ordinary
Alarm 01 performance; no block workaround is warranted by this review.
It does not extend listening acceptance to the separate stop/restart fixture
or establish agreement with the website recording.

## Remaining work

- Continue with Alarm 02; keep website performances
  distinct from the scripted PD/Kleine recipes.
- Assess the 05 timing differences audibly before choosing any explicit,
  separable block workaround.
- The programmed alarm still uses a local reset timestamp. Replace it with the
  reusable `pd::del` during that model's topology review; numerical agreement
  alone does not finish that follow-up.
- Decide whether the two earlier supplemental studies need retaining after
  listening. Neither is excluded yet.

Reproduce, replacing 48000 with 44100 for the second rate:

```text
python tools/render_artificial.py --cases alarm01 alarm01-controls alarm02 alarm03 alarm04 alarm05 alarm05-controls alarm06 alarm07 alarm15 alarm-bank --rate 48000 --review --output-root build/alarm-review
```
