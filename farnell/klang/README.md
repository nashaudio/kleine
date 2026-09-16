# Klang models from Designing Sound

The implemented Artificial Sounds (24–28) and Idiophonics (29–33) models have
undergone a [PD topology review](../KLANG-TOPOLOGY-REVIEW.md). The current code
favours the pure patch form. Removed implicit block timing is awaiting Chris's
one-by-one sound review; the earlier retained WAVs and parity results describe
the previous implementation, not blanket acceptance of the revised code.

The [Artificial Sounds](../audio/artificial-sounds.md) and
[Idiophonics](../audio/idiophonics.md) guides retain source/variant and recording
evidence. [Current raw comparisons](../audio/comparisons/klang-topology-review.json)
record the effects of this pass. Original PD files and retained audio are unchanged.
Core `include/klang.h` is unchanged; the reusable primitives are in
[pd.h](../../include/klang/pd.h).

## Style and primitive interfaces

Match the specific PD source's objects, constants, controls and signal flow.
Inline single-use generators and short chains; keep reused values and names for
complex stages. Use `object.out` to reuse an evaluated sample. Keep statements on
separate lines and discuss genuinely borderline readability choices.

Use `set(param on)` / `metro = on` for start/stop. Cache simple parameters; inline
configuration where inexpensive. Preserve fixed patch values. Alternate implementations
live in separate development files under each model's `variants/` directory;
provenance and selection belong in the [comparison notes](VARIANTS.md) and renderer.
Keep selectors for distinct sounds and studies. PD block workarounds must be explicit and
separable from the pure model. See [PD-BLOCK.md](../PD-BLOCK.md).

Aim for complete PD primitive ports, documenting remaining limits. Review
tilde/non-tilde pairs individually, favouring polymorphism. `pd::line` now supports:

```cpp
line.set(target, duration);        // Audio ramp: line~.
line.set(target, duration, grain); // Control ramp: line; times in ms.
line = { {0, 1}, {3000, 0}, 20 }; // Equivalent start/end breakpoint notation.
```

`pd::vline` supplies the complete queued audio-ramp message interface and replaces
Gesture. Use `set(target, durationMs, delayMs)` for delayed segments; both time
inlets are consumed by the next target. [Port and timing notes](../../tests/pd/vline.md)
explain queue replacement and explicit host timestamps. Pure models remain
sample-timed; no new 64-sample adapter is embedded in their signal flow.

PD primitives retain Doxygen API descriptions and usage examples. Models use
minimal comments and self-documenting code: one short `//` description per object,
with at most one longer Doxygen overview before the main model.
[Line API tests](../../tests/pd/line.md) explain control emissions and
remaining scheduling limits. The earlier
[style-only comparison](../audio/comparisons/klang-style-review.json) remains
historical evidence; this deeper topology pass deliberately changes some signals.

## Current model controls

| Model | Control / source selection |
| --- | --- |
| Pedestrians | `set(on)`; fixed 2500 Hz, 100 ms metro, gain 0.2. |
| PhoneTones | Constructor `(Tone)` selects 350/450 Hz dial, smoothed busy, brighter ringback or pulse sounds; `dial(count)` for pulse mode. Listening choices recorded in the [review](../audio/25-phone-tones/README.md). |
| DTMFTones | `dial(key)`; `tones(highHz,lowHz)` and `gate(value)` expose the oscillator/envelope inlets. Fixed 200 ms release, gain 0.25 and highpass. |
| AlarmGenerator | Constructor study 1–7; `set(on)` for 1/5. Programme `set(durationMs,cycles,hz1,hz2,hz3,hz4,colour)` then `trigger()` for 6/7. |
| Police | `set(sweepHz)`; fixed 300 Hz base, 800 Hz depth and echo network. |
| TelephoneBell | `set(on)` or `strike()`; fixed 650/653 Hz, strength 1, base decay 2000 ms, metro 60 ms. Dry diagnostics live in the renderer. |
| Bouncing | `trigger()`; fixed fourth-power pitch, 3000 ms height decay and output 0.2. |
| Rolling | `trigger()`; `set(1)` selects struck-can source, `set(0)` rolling. |
| Creaking | `set(force)` in 0..1, slewed over 100 ms. Performances are supplied externally. |
| Boing | `set(frequencyHz)` or `set(frequencyHz, vibratoDepthHz)` and `trigger()`. Vibrato rate 5 Hz and gain 4 remain fixed. |

PhoneEffects and BellStudies preserve their distinct supplemental source patches;
their headers document controls. Fixed tuning/filter preparation remains in
`prepare()` where appropriate. Set `klang::fs` before constructing delay models;
the Processor prepares attached sounds before each buffer.

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
