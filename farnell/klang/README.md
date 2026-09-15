# Klang models from Designing Sound

**Artificial Sounds (chapters 24–28) is trial validated**, including documented bulk variants and reconstructed number matchers for the two demos with missing external dependencies. See the [series guide](../audio/artificial-sounds.md) for all implementations, controls, paired audio, comparison limits and reproduction commands. Models include `Pedestrians`, `PhoneTones`, `DTMFTones`, `AlarmGenerator`, `Police`, and the supplemental `PhoneEffects`.

Design study: [PD block timing and the Pedestrians API](../PD-BLOCK.md) compares ways to hide compatibility scheduling from model authors, with separate C++17 probes. It proposes APIs for review; the validated models continue to use their existing implementations.

[phonetones.k](Artificial%20Sounds/Phone%20Tones/phonetones.k) implements chapter 25, figures 25.2–25.6. [telephonebell.k](Idiophonics/Telephone%20Bell/telephonebell.k) implements chapter 29, figures 29.7–29.15. Both use `klang::optimised` and the PD primitives in [pd.h](../../include/klang/pd.h). Models are grouped by book section and practical. Core `include/klang.h` is unchanged.

Start with the [audio bundle](../audio/README.md): website excerpts and paired PD/Kleine renders at 48 kHz. Attribution is to Andy Farnell, *Designing Sound* (MIT Press, 2010); project licensing remains as described in the [root README](../../README.md#licensing-and-attribution).

## Phone tones

`farnell::PhoneTones` is a mono `Sound`. Select a mode with `set(mode, dialHz = 440, smoothBusy = true)` before processing. `dial(digit)` triggers pulse dialling; zero means ten pulses. `PhoneTones::Line` is its nested handset/line filter with distinct normal and pulse-dial settings.

| Mode / CLI name | Default signal and behaviour | PD reference |
| --- | --- | --- |
| `Dial` / `dial` | 350 + 440 Hz, each at 0.125 | `PHONETONES/dialtone1.pd` |
| `Dial` / `dial-web` | 350 + 450 Hz, each at 0.125 | Updated `reference/p02/dialtone1.pd` |
| `DialLine` / `dial-line` | 350 + 450 Hz through the clipped, filtered line; enabled continuously | `PHONETONES/dialtone2.pd` |
| `Busy` / `busy` | 480 + 620 Hz, each at 0.1; clipped 2 Hz cosine gate, smoothed at 100 Hz | Updated `reference/p02/busy-signal.pd` |
| `Busy` / `busy-archive` | Same carriers, unsmoothed gate | `PHONETONES/busy-signal.pd`; unused manual 600 Hz test tone is not triggered |
| `Ringback` / `ringback` | 480 + 440 Hz through the line; 3 s off, 3 s on | `PHONETONES/ringingtone.pd` |
| `Pulse` / `pulse` | 40 ms contacts every 100 ms, through the pulse line, output gain 0.8 | `PHONETONES/pulsedial.pd` + `telephone-line.pd` |

The Klang controls are `Tone` (0–4 in the enum order), `Dial frequency` (second oscillator, Hz; default 440), and `Smooth busy` (default on). The CLI selects 450 Hz for `dial-line`, following its source patch. These are reference sound generators, not a complete telephone state machine. Mode changes are not crossfaded.

## Telephone bell

`farnell::TelephoneBell` is a mono `Sound`. `ring(true/false)` runs/stops its alternating striker; `strike()` excites one bell and advances to the other. Stopping the striker preserves the ringing tail. Each bell has five groups of three inharmonic partials, grouped squared decay envelopes, and a shared short noise hammer. Both bells drive the clipped resonant casing network.

| Control | Default | Unit / range |
| --- | ---: | --- |
| Ring | 0 | Off/on |
| Fundamental | 650 | Hz, 100–1000 |
| Detune | 3 | Second bell offset in Hz, 0–30 |
| Strength | 1 | Partial amplitude multiplier, 0–2 |
| Decay | 2000 | Base envelope duration in ms, 10–4000; each group applies its own ratio |
| Strike interval | 60 | ms, 10–200 |

Fundamental, detune, strength and decay apply at the next strike. Like the patch, the hammer has fixed 10 ms decay and 0.1 gain; `Strength` controls the tonal partials. `casingMix = 0` exposes the dry bells/hammer for diagnostics; default 1 reproduces the patch.

The reference is `pd/BELL/striker.pd`, byte-identical to the website's `telephonebell.pd`, with the supplied `bellosc`, `bellenv`, `partialgroup` and `ratios` abstractions. `A4-bell-telephone.pd` is a different implementation. The casing retains measured PD scheduling at block size 64: 64 samples for the width delay at 48 kHz, 42 for length, 64 in the material feedback, and 128 around the enclosing send/receive route. Those delays affect phase and timbre. Construct the model after setting `klang::fs`; its reference fidelity is established at 48 kHz and block size 64, not every PD version/rate/block size.

## Reproduce the trials

Build Release as in the root README. Python 3.12 was used with the pinned analysis dependencies:

```text
python -m pip install --target build/trials/python -r tools/requirements.txt
python tools/render_farnell.py
python tools/package_farnell.py
```

The default executables are `build/x64-release/kleine.exe` and `C:/Program Files/Pd/bin/pd.exe`; override with `--kleine` and `--pd`. Use `--compatibility 0.43` to investigate the older routines inside the installed PD executable. The packager also accepts `--pd`. It first uses the verified recordings in `farnell/zip/p02` and `p06`, then the build cache, and downloads only if neither exists. Updated PD variants needed for the trial are already tracked in `reference/`.

To keep an experimental run out of the retained fixtures:

```text
python tools/render_farnell.py --cases bell --output build/my-trial --results build/my-trial/results.json
python tools/compare_audio.py build/my-trial/29-telephone-bell/bell-pd.wav build/my-trial/29-telephone-bell/bell-kleine.wav --output build/my-trial/bell.png
```

The renderer exposes the original patches' left DAC signal without altering their DSP, supplies GUI events, captures an array, writes float32 WAV with `soundfiler`, then exits. Temporary wrappers go in `build/trials/<case>/`. It checks file length, sample rate, channel count, finite samples and non-silence. Headless PD may emit a Windows registry warning; missing objects/connections are treated as failures.

Kleine's `--render MODEL OUTPUT.wav [seconds] [sample-rate] [gain]` processes through its existing `Processor` in blocks up to 64 samples, splitting at timed events. Gain defaults to 1 and does not include the live device callback's 0.25 output attenuation. Output is mono float32 WAV; the default duration is 10 s. No audio device opens.

The scripted cases render four seconds except `ringback` (12 s) and `bell` (10 s). Pulse digits 1, 5, 7 start at nominal 0.25, 0.75, 1.75 s; to reproduce PD `sig~`, starts are rounded down to a 64-sample boundary (11968, 35968, 83968 at 48 kHz). Bell ringing runs 0.5–2.5 s and 4–6 s, with 60 ms alternating strikes and the default controls. Noise starts at seed 404933 and advances continuously. All remaining time captures the tail.

Optional diagnostic cases are `bell-single` (strike at 0.5 s), `bell-single-dry`, `bell-dry`, and `bell-casing` (isolated casing impulse at 0.5 s). Their outputs belong under `build/`. These isolated comparisons established the partial envelopes, noise sequence and casing delays before testing the full model.

## Compatibility notes

`pd::osc`, `pd::hip` and `pd::noise` are in `include/klang/pd.h`, adapted from PD tag `0.55-2` with [BSD notices](../../licenses/Pure-Data-BSD.txt). The oscillator now uses PD's 2048-point cosine table and phase arithmetic. The existing `pd::noise` now outputs before advancing, wraps unsigned arithmetic, follows PD's default seed sequence and supports explicit seeding. This changes the random sample sequence of existing users such as Harrier, without changing the intended noise level/distribution.

Model-only line, bell, partial-group, decay and casing components are nested. All `set` arguments use `param`; buffer preparation is left to Kleine. `Controls::set` currently requires scalar C++ arguments internally, so the model converts its `param` arguments when storing controls.

The MSVC build also found an existing ambiguous `operator>>` when using the `Output&` returned by `osc(frequency)` directly. The FM test materialises that return as a `signal` before routing it. Neither workaround required a change to core Klang.

[Isolated primitive patches and tests](../../tests/pd/README.md) check oscillators (including phase, negative frequency and FM), highpass, seeded noise, lowpass, bandpass and the limited decay helper. Run `python tools/check_pd_primitives.py`. The [level investigation](../audio/comparisons/reference-levels.md) separates website recording gain from measured PD-version changes.

Further changes should retain the original PD files and document meaningful differences. Existing machine ports for subsequent calibration are `ToyBoatEngine` and `FourStrokeEngine` in [sounds/Motors.h](../../sounds/Motors.h).
