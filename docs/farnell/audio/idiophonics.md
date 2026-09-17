# Idiophonics — chapters 29–33

> **Topology review, 16 September 2026:** retained audio, controls and validation claims below describe the pre-review implementation. Current pure-form changes and pending listening decisions are documented in [the topology review](../reviews/KLANG-TOPOLOGY-REVIEW.md). Original recordings remain unchanged.

The remaining Idiophonics practicals and telephone-bell supplements are implemented and trial validated: **23 catalogue entries complete within their stated scope**. This extends the [original telephone-bell trial](README.md) with Bouncing, Rolling, Creaking, Boing and the nine earlier bell studies/abstractions.

The new trial has **33 paired cases at each of 48 and 44.1 kHz**, using PD-Vanilla 0.55.2, compatibility 0.55 and block 64. [Measurements](comparisons/idiophonics/tables.md), [full run records](../../../farnell/audio/comparisons/idiophonics/render-results.json), and the [manifest](../../../farnell/audio/idiophonics-manifest.json) retain controls, timing, levels, source hashes and machine/compiler information. Validation is numerical and visual. Chris found the initial creaking demonstration markedly different and less convincing than the website; the correction below addresses its movement. Listening approval of the revised performance and game-ready realism remain open.

## Listen and inspect

All PD/Kleine files below are mono float32 at their raw patch gain. Online files are complete, byte-identical copies of the verified local website recordings, retaining their original stereo PCM samples. No normalisation, fades or resampling are applied.

| Practical | Klang model | Website recording | PD / Kleine | Envelope and sonogram |
| --- | --- | --- | --- | --- |
| Bouncing | [bouncing.k](../../../farnell/klang/Idiophonics/Bouncing/bouncing.k) | [Online](../../../farnell/audio/30-bouncing/bouncing-online.wav) | [PD](../../../farnell/audio/30-bouncing/bouncing-pd.wav) / [Kleine](../../../farnell/audio/30-bouncing/bouncing-kleine.wav) | [Comparison](../../../farnell/audio/comparisons/idiophonics/bouncing.png) |
| Rolling can | [rolling.k](../../../farnell/klang/Idiophonics/Rolling/rolling.k) | [Online](../../../farnell/audio/31-rolling/rolling-online.wav) | [PD](../../../farnell/audio/31-rolling/rolling-pd.wav) / [Kleine](../../../farnell/audio/31-rolling/rolling-kleine.wav) | [Comparison](../../../farnell/audio/comparisons/idiophonics/rolling.png) |
| Struck can | `Rolling`, struck mode | Same practical | [PD](../../../farnell/audio/31-rolling/tincan-pd.wav) / [Kleine](../../../farnell/audio/31-rolling/tincan-kleine.wav) | [Comparison](../../../farnell/audio/comparisons/idiophonics/tincan.png) |
| Separate ground/can | `Rolling::Uneven` | Same practical | [PD](../../../farnell/audio/31-rolling/uneven2-pd.wav) / [Kleine](../../../farnell/audio/31-rolling/uneven2-kleine.wav) | [Comparison](../../../farnell/audio/comparisons/idiophonics/uneven2.png) |
| Creaking door | [creaking.k](../../../farnell/klang/Idiophonics/Creaking/creaking.k) | [Online](../../../farnell/audio/32-creaking/creaking-online.wav) | [PD](../../../farnell/audio/32-creaking/creaking-pd.wav) / [Kleine](../../../farnell/audio/32-creaking/creaking-kleine.wav) | [Comparison](../../../farnell/audio/comparisons/idiophonics/creaking.png) |
| Boing, website | [boing.k](../../../farnell/klang/Idiophonics/Boing/boing.k) | [Online](../../../farnell/audio/33-boing/boing-online.wav) | [PD](../../../farnell/audio/33-boing/boing-pd.wav) / [Kleine](../../../farnell/audio/33-boing/boing-kleine.wav) | [Comparison](../../../farnell/audio/comparisons/idiophonics/boing.png) |

Hann sonograms use FFT 16384/hop 1024 and consistent axes/−100…0 dB colours. Each main comparison also has a `-2048.png` image for contact and transient detail. The website performances have unknown controls, timing and oscillator/noise states; their plots provide reference context, not a residual-based claim to reproduce the recordings exactly.

### Creaking: correction after listening review

The original fixture held force at 0.7, 0.95, 0.4 and 0.8 for long stretches. It was useful for comparing the two implementations, but a poor demonstration of the sound: steady contact rates produce sustained buzzing. The website recording contains two accelerating/decelerating movements, reaching roughly 333 contacts/second at maximum force and slowing towards 20 contacts/second near stopping.

The main [Kleine](../../../farnell/audio/32-creaking/creaking-kleine.wav) and [PD](../../../farnell/audio/32-creaking/creaking-pd.wav) fixtures now use a moving-force contour fitted to those gestures. The default `--render creaking` demonstration uses the same [shared contour](../../../tests/pd/creaking-force.inc). The underlying wood/panel and stick/slip model are unchanged by this performance correction. The initial held-force comparison remains available as [PD](../../../farnell/audio/32-creaking/creaking-controls-pd.wav) / [Kleine](../../../farnell/audio/32-creaking/creaking-controls-kleine.wav) and still supplies the pulse diagnostic's controls.

[Analysis](../../../farnell/audio/comparisons/idiophonics/creaking-performance.png) and [measurements](../../../farnell/audio/comparisons/idiophonics/creaking-performance.json) support movement as the main explanation:

- Inverting the independently measured wood/panel impulse response gives contact intervals consistent with two force sweeps. The estimate uses the source patch's average interval jitter; it does not recover the original slider messages or seed.
- The initial 35 ms contact matches the online sample at about **−55 dB residual** after explicit time alignment and a **0.9932 gain** adjustment. This is strong evidence for the supplied material response; no PD-version or additional-processing explanation is needed for this contact.
- At original time and raw gain, **100 ms RMS envelope error falls from 74.5% to 14.4%**. Mean inferred-force error, in windows where both are active, falls from **0.320 to 0.016** on the 0–1 scale. These are fitted performance measurements, not independent perceptual validation.

The revised clip approximates the website's articulation. The user's criticism of the model's realism is not resolved merely by matching that performance; a preferred game adaptation remains separate work.

### Supplements and source differences

| Source / case | Retained implementation and evidence |
| --- | --- |
| `BOUNCINGBALL/bb1.pd` | Website pitch follows the fourth power of the impact envelope. The bulk patch instead uses `pow~`: modern wiring gives `2.71828^envelope`, while explicitly swapping its inlets gives `envelope^2.71828`. Both are separate options: [bulk PD](../../../farnell/audio/30-bouncing/bouncing-bulk-pd.wav) / [Kleine](../../../farnell/audio/30-bouncing/bouncing-bulk-kleine.wav), [legacy-convention PD](../../../farnell/audio/30-bouncing/bouncing-legacy-pd.wav) / [Kleine](../../../farnell/audio/30-bouncing/bouncing-legacy-kleine.wav). This is not evidence of one historical authoring version. |
| `ROLLING/tincan.pd` | Supplies the separate struck can missing from the website's duplicate `rolling1.pd` link. It has different modal weights and ±0.6 clipping, versus ±0.3 in the rolling can; both are preserved. |
| Bulk `rolling1.pd` / `uneven2.pd` | Both revisions were rendered independently. The recorded/graph data and presentation differences do not change the measured outputs under the shared controls/seeds. |
| `ROLLING/uneven.pd` | `Rolling::Study` implements the audible random linear gestures and parabolic shaping: [PD](../../../farnell/audio/31-rolling/uneven-pd.wav) / [Kleine](../../../farnell/audio/31-rolling/uneven-kleine.wav). The noise/phasor experiments are disconnected from its audible output. The rendering copy explicitly enables its saved spigot control, starts the recursive delay loop, then stops it; the last value is held, so this diagnostic ground signal has DC and no silence tail. |
| `MRBOINGY/twang.pd` | With linear decay `d`, website `env16p` actually computes `3*d^64`. The bulk source computes `15^(d^4)` on modern PD; swapping the historical inlet convention gives `(d^4)^15`. [Bulk comparison](../../../farnell/audio/comparisons/idiophonics/boing-bulk.png), [legacy-convention comparison](../../../farnell/audio/comparisons/idiophonics/boing-legacy.png). Raw bulk output exceeds unity (about 1.015 at 48 kHz; 1.058 at 44.1 kHz) in both renderers. Quieter audition copies apply exactly 0.8 gain to both: [PD](../../../farnell/audio/33-boing/boing-bulk-pd-listening.wav) / [Kleine](../../../farnell/audio/33-boing/boing-bulk-kleine-listening.wav). Raw files remain available. |
| `BELL/A0`, `A1`, `A2` | [studies.k](../../../farnell/klang/Idiophonics/Telephone%20Bell/studies.k) uses PD oscillators, finite squared decay and `BellStudies::Partials`. A2's sum gain is 0.5, different from the final partial group: [PD](../../../farnell/audio/29-telephone-bell/bell-a2-pd.wav) / [Kleine](../../../farnell/audio/29-telephone-bell/bell-a2-kleine.wav). A0/A1 are diagnostic fixtures. |
| `BELL/A3-bell-ratio` / `A3-bell-scale` | `BellStudies::Ratios` preserves the double-burst cadence, unused even ringer outlet and disconnected middle overtone group: [PD](../../../farnell/audio/29-telephone-bell/bell-a3-pd.wav) / [Kleine](../../../farnell/audio/29-telephone-bell/bell-a3-kleine.wav). The dynamic numeric `$1`/`$2` object boxes **load successfully in PD 0.55.2**; the scale helper is exercised by the A3 render, with no reconstruction. The static dependency scanner still cannot expand arbitrary argument-named objects. |
| `BELL/A4-bell-telephone` | `BellStudies::Telephone` retains its different partial ratios, block-rounded linear/squared/fourth-power envelopes, noise strike and 59 ms body resonance: [PD](../../../farnell/audio/29-telephone-bell/bell-a4-pd.wav) / [Kleine](../../../farnell/audio/29-telephone-bell/bell-a4-kleine.wav). |
| `BELL/partial`, `group`, `testgroup` | Independent per-partial decays and the earlier 0.3333 group gain are covered separately from the final shared envelope. [Group PD](../../../farnell/audio/29-telephone-bell/bell-group-pd.wav) / [Kleine](../../../farnell/audio/29-telephone-bell/bell-group-kleine.wav), [testgroup PD](../../../farnell/audio/29-telephone-bell/bell-testgroup-pd.wav) / [Kleine](../../../farnell/audio/29-telephone-bell/bell-testgroup-kleine.wav). |

Original PD sources, the parked old archive and book assets are unchanged. The existing figure mappings and their confidence are retained; a completed render does not upgrade a topic-level figure match to a caption match.

## Model controls

Construct models after setting `klang::fs`, then attach them to Kleine's Processor. The processor calls `prepare()`; do not call it in constructors or setters. These fixtures establish fresh instances at the two fixed rates, not live sample-rate changes.

| Model | Controls and defaults | Gesture |
| --- | --- | --- |
| `Bouncing` | `set(source=0, gain=0.2)`: source 0 website, 1 bulk modern, 2 historical inlet convention | `trigger()` starts/restarts the fixed three-second fall. The silent end-state metro remains active because its envelope state affects retriggering. |
| `Rolling` | `set(struckCan=0, gain=1)` | `trigger()` gives a 500 ms push or a 2 ms struck-can excitation. Repeated pushes accumulate through the 0.1 Hz inertia filter. |
| `Rolling::Uneven` | Supplied fixed ground/can parameters | Continuous seeded noise; no trigger. |
| `Rolling::Study` | Supplied fixed shaping | `trigger()` starts the recursive random gesture; `stop()` cancels its next update and holds the final position. |
| `Creaking` | `set(force, gain=0.2)`, force clamped to 0…1 | Force slews over 100 ms with 20 ms control updates. Above 0.3 it produces stick/slip impulses; zero stops new impulses after the control slew, retaining the panel tail. |
| `Boing` | `set(frequency=426 Hz, vibratoDepth=6 Hz, source=0, gain=1)`; vibrato rate fixed at 5 Hz | `trigger()` resets the drive phase and strikes both envelope families. Source 0 website, 1 bulk modern, 2 historical inlet convention. |
| `BellStudies::Ratios` / `Telephone` | Earlier fixed studies, not replacements for `TelephoneBell` | `ring(bool)` on Ratios; `set(fundamental Hz)` and `trigger()` on Telephone. |

The [render recipes](../../../tools/render_idiophonics.py) exercise repeated drops/pushes/strikes, Boing retuning, force changes and stopping. Events use a 64-sample grid. PD parses delay times as float milliseconds a quarter-sample inside the intended block; the renderer supplies the same fractional offset to Klang's finite gestures. Model calls outside these comparison scripts use zero offset. Noise is seeded to 404933 and control random objects to 1; arbitrary seeds and controls are outside the retained parity scope.

## Validation and limits

- [58 isolated primitive cases](../../../tests/pd/results.json) pass across both rates. `vcf~` now uses PD's actual complex table resonator and gain; real/imaginary outputs, negative/positive FM and Q=0/80 are compared. New `samphold~` and `rzero~` ports have seeded fixtures. Lowpass and high-Q bandpass arithmetic now matches the checked PD source, including its constants and operation order.
- All 66 new paired cases pass their recorded trial limits. Most deterministic residuals are below −70 dB. Rolling retains occasional noise-contact differences: the worst measured 10 ms envelope error is about 0.38%, with median active-spectrum difference about 0.0014 dB. Creaking retains small sub-millisecond pulse/control timing differences in both the moving and held-force cases; the full models remain within the contact-model limits below. These are documented approximations, not exact sample parity.
- The creaking delay and complete panel impulse responses are sample-identical at both rates. This isolates the remaining **PD/Klang numerical discrepancy** to the pulse/control path; it is distinct from the website performance mismatch discussed above. Material response, Boing phase/pitch/free/clamped modes, and bounce height/envelopes have separate diagnostic cases.
- Contact-model limits are level difference <0.01 dB, envelope error <2%, median spectrum difference <0.1 dB and residual below −35 dB; the unfiltered creak pulse diagnostic uses −25 dB. Other cases use <0.002 dB level difference and residual below −70 dB. These thresholds describe these trials, not universal perceptual acceptance criteria.
- Every render checks finite, non-silent output; records raw peak/RMS/DC, samples at/above full scale and the last 100 ms RMS. Graph/control fixtures can legitimately have DC or exceed unity. The original bulk Boing overload is retained separately from the quieter audition files. The rolling inertia study and some oscillator/control diagnostics intentionally have no silence tail.
- The earlier Artificial Sounds fixtures pass again at both rates after the shared filter changes; the chapter 25/29 regression renders also remain close. [Smoke checks](../../../farnell/audio/comparisons/idiophonics/smoke-results.txt) exercise rapid control retargets and existing Harrier use of `pd::vcf`. Harrier required materialising its turbine output to compile; its selected smoke settings exceed unity substantially, so this establishes finite processing, not level safety or equivalence to its old timbre.

The shared [finite gesture/delay helpers](../../../farnell/klang/Idiophonics/common.h) cover these models. They are not complete `vline~`, named PD delay-buffer or general scheduler ports. Core `include/klang.h` remains unchanged; promotion and lifecycle work is recorded in [KLANG-REVIEW.md](../reviews/KLANG-REVIEW.md).

## Reproduce

From the project root, with the Release executable and `tools/requirements.txt` dependencies installed:

```text
python tools/check_pd_primitives.py
python tools/render_idiophonics.py --retain
python tools/render_idiophonics.py --rate 44100
python tools/package_idiophonics.py
build\x64-release\kleine.exe --render bouncing build/bouncing.wav 9 48000
build\x64-release\kleine.exe --render rolling build/rolling.wav 8 48000
build\x64-release\kleine.exe --render creaking build/creaking.wav 8 48000
build\x64-release\kleine.exe --render boing build/boing.wav 10 48000
```

`--cases` selects individual comparisons; `--pd` and `--kleine` override executable paths. Scratch PD copies, event scripts and diagnostic WAVs stay under `build/idiophonics/`. The C++ smoke probe can be compiled from a Visual Studio developer prompt with:

```text
cl /nologo /std:c++17 /EHsc /O2 /DNOMINMAX /Iinclude tests/pd/idiophonics-smoke.cpp /Febuild/idiophonics/smoke.exe /Fobuild/idiophonics/smoke.obj
build\idiophonics\smoke.exe
```

Attribution: Andy Farnell, *Designing Sound* (MIT Press, 2010); Pure Data primitive/control source by Miller Puckette and contributors under the retained BSD notice. The Farnell code permission does not establish recording redistribution rights. See the [repository licensing position](../../../README.md#licensing-and-attribution).
