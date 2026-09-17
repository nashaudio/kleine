# Redundancy in the parked `farnell/pd/old` archive

Compared the working trees on 15 September 2026. Chris subsequently moved `old` inside `pd`; links now follow that move. The archive is parked unchanged, and this historical report is not an active cleanup instruction. No source files were removed or changed. The [complete manifest](../../../farnell/reference/old-redundancy.json) records every old file's SHA-256, byte count, classification, matching `pd` paths and detected local references.

| Result | Old files |
| --- | ---: |
| Byte-identical at the same relative path in `pd` | 418 |
| Byte-identical elsewhere in `pd` | 5 |
| Different contents at the same path, with no exact match elsewhere | 16 |
| No matching path or contents anywhere in `pd` | 31 |
| Total | 470 |

There are **423 exact duplicates**, occupying **6,490,360 bytes (6.19 MiB)**. Matching uses SHA-256 followed by actual byte equality across all 505 `pd` files, regardless of filename or extension. This is file-content comparison, not an audio-equivalence test. Two more patches differ only in saved canvas window state; see below.

This supersedes the earlier [same-path inventory](../../../farnell/reference/old-inventory.json) for cleanup decisions. That snapshot recorded 417 same-path matches; the current old audio file is named `BASICS/sounds/criticsgetmean.wav` and now matches the `pd` path. The earlier snapshot recorded it as `critics.wav`.

## Identical files under different paths

Paths below are relative to their respective source trees.

| Old file | Exact copy in `pd` |
| --- | --- |
| [BASICS/sounds/ocsnr.wav](../../../farnell/pd/old/BASICS/sounds/ocsnr.wav) | [BASICS/sounds/osnr5.wav](../../../farnell/pd/BASICS/sounds/osnr5.wav) |
| [BELL/bell-separate-voices.pd](../../../farnell/pd/old/BELL/bell-separate-voices.pd) | [drum/drum5.pd](../../../farnell/pd/drum/drum5.pd) |
| [BELL/envlp-ad.pd](../../../farnell/pd/old/BELL/envlp-ad.pd) | [drum/envlp-ad.pd](../../../farnell/pd/drum/envlp-ad.pd) |
| [BELL/listosc2.pd](../../../farnell/pd/old/BELL/listosc2.pd) | [drum/listosc2.pd](../../../farnell/pd/drum/listosc2.pd) |
| [BELL/voice1.pd](../../../farnell/pd/old/BELL/voice1.pd) | [drum/voice1.pd](../../../farnell/pd/drum/voice1.pd) |

The `BELL` files are therefore redundant in content even though their names/location suggest a different practical. If retaining an entry patch, its abstraction names and search directory still matter.

## Same-path differences

These 16 old files have no exact byte match anywhere in `pd`. Changes described here compare the old copy with the same-path `pd` copy; the folder names do not establish which edit happened first.

| Old file | Difference / recommendation |
| --- | --- |
| [ABOUTPD/midi-monosynth.pd](../../../farnell/pd/old/ABOUTPD/midi-monosynth.pd) | Envelope timing differs: old attack/decay message uses `1 10 0, 0 100 20`; `pd` uses `1 0 0, 0 300 0`. Retain. |
| [BUBBLES/dripfactory.pd](../../../farnell/pd/old/BUBBLES/dripfactory.pd) | Different top-level triggering, control automation and output gains. Retain. |
| [FOOTSTEPS/foot1.pd](../../../farnell/pd/old/FOOTSTEPS/foot1.pd) | Old has an extra `*~ 0.1` on each output; `pd` connects the test oscillators directly to `dac~`. Retain as a level variant. |
| [FOOTSTEPS/foot2.pd](../../../farnell/pd/old/FOOTSTEPS/foot2.pd) | Saved walkspeed differs, and old has an additional connected `loadbang`. Retain. |
| [GUNS/guns1.pd](../../../farnell/pd/old/GUNS/guns1.pd) | Different distortion implementation: old contains two continued-fraction tanh subpatches and a selector; `pd` uses a lookup table. Also differs in saved arrays and controls. Retain. |
| [GUNS/shellcasings.pd](../../../farnell/pd/old/GUNS/shellcasings.pd) | Old includes a manual trigger and connected stereo `dac~` absent from `pd`. Retain the runnable test variant. |
| [MAMMALS/roar.pd](../../../farnell/pd/old/MAMMALS/roar.pd) | **Window-state-only candidate:** main/subpatch window coordinates and subpatch visibility differ. All remaining bytes match after normalising those canvas fields. Can discard if saved window state is unimportant. |
| [MAMMALS/vowels.pd](../../../farnell/pd/old/MAMMALS/vowels.pd) | Different excitation and output processing, with additional circuitry in `pd`. Retain. |
| [MOTORS/motorenv.pd](../../../farnell/pd/old/MOTORS/motorenv.pd) | The two inputs to `pow~` are swapped. Retain as direct evidence for the README's compatibility warning. |
| [PHONETONES/busy-signal.pd](../../../farnell/pd/old/PHONETONES/busy-signal.pd) | Old includes `lop~ 100` on the gate; `pd` omits it and adds a separate triggered 600 Hz example. Retain. |
| [RAIN/rainonwater.pd](../../../farnell/pd/old/RAIN/rainonwater.pd) | Different `pow~` inlet wiring; old uses an extra `sig~` for the exponent path. Retain for the rain/version investigation. |
| [ROCKETLAUNCHER/rocket.pd](../../../farnell/pd/old/ROCKETLAUNCHER/rocket.pd) | **Window-state-only candidate:** only the `exit` subpatch's saved visibility changes from 0 to 1. Can discard if saved window state is unimportant. |
| [RUNNINGWATER/running-water4.pd](../../../farnell/pd/old/RUNNINGWATER/running-water4.pd) | **Keep:** old contains a complete three-voice `waterflow` patch; the `pd` file contains only an empty canvas. |
| [RUNNINGWATER/water2.pd](../../../farnell/pd/old/RUNNINGWATER/water2.pd) | Substantial differences in excitation, control defaults and monitoring graphs. Retain. |
| [SHAPING/shaping-expressions.pd](../../../farnell/pd/old/SHAPING/shaping-expressions.pd) | Saved slider values, graph names and array contents differ. Retain pending a decision about preserving example state. |
| [WATER/bubble2c.pd](../../../farnell/pd/old/WATER/bubble2c.pd) | Output multiplier is `0.5` in old versus `0.1` in `pd`; object ordering also differs. Retain as a level variant. |

The two window-state candidates are separate from the 423 exact matches. The normalisation ignores only `#N canvas` window geometry and subpatch visibility; it retains object positions, names, connections, comments, control values and array data. No listening or PD execution was needed to establish these limited textual differences.

## Files without a counterpart in `pd`

Keep these 31 paths, apart from optional deduplication within `old` described below:

- [AAA-README.FIRST](../../../farnell/pd/old/AAA-README.FIRST): the redistribution/modification permission and version warning; preserve its original text.
- [ABOUTPD/running-max.pd](../../../farnell/pd/old/ABOUTPD/running-max.pd)
- [PD-COMMON/message-lowpass.pd](../../../farnell/pd/old/PD-COMMON/message-lowpass.pd)
- [TECHNIQUE/additive-s.pd](../../../farnell/pd/old/TECHNIQUE/additive-s.pd)
- [TECHNIQUE/additive1.pd](../../../farnell/pd/old/TECHNIQUE/additive1.pd)
- [TECHNIQUE/additive2.pd](../../../farnell/pd/old/TECHNIQUE/additive2.pd)
- [TECHNIQUE/blit-dodge.pd](../../../farnell/pd/old/TECHNIQUE/blit-dodge.pd)
- [TECHNIQUE/complexfm2.pd](../../../farnell/pd/old/TECHNIQUE/complexfm2.pd)
- [TECHNIQUE/grainvoice.pd](../../../farnell/pd/old/TECHNIQUE/grainvoice.pd)
- [TECHNIQUE/granular-plot.pd](../../../farnell/pd/old/TECHNIQUE/granular-plot.pd)
- [TECHNIQUE/granular1.pd](../../../farnell/pd/old/TECHNIQUE/granular1.pd)
- [TECHNIQUE/granular2.pd](../../../farnell/pd/old/TECHNIQUE/granular2.pd)
- [TECHNIQUE/moorer-dsf2.pd](../../../farnell/pd/old/TECHNIQUE/moorer-dsf2.pd)
- [TECHNIQUE/packet-fakefilter.pd](../../../farnell/pd/old/TECHNIQUE/packet-fakefilter.pd)
- [TECHNIQUE/packet-split2.pd](../../../farnell/pd/old/TECHNIQUE/packet-split2.pd)
- [TECHNIQUE/partial.pd](../../../farnell/pd/old/TECHNIQUE/partial.pd)
- [TECHNIQUE/phase-distortion.pd](../../../farnell/pd/old/TECHNIQUE/phase-distortion.pd)
- [TECHNIQUE/phase-distortion0.pd](../../../farnell/pd/old/TECHNIQUE/phase-distortion0.pd)
- [TECHNIQUE/phase-distortion2.pd](../../../farnell/pd/old/TECHNIQUE/phase-distortion2.pd)
- [TECHNIQUE/precompute.pd](../../../farnell/pd/old/TECHNIQUE/precompute.pd)
- [TECHNIQUE/simplefm2.pd](../../../farnell/pd/old/TECHNIQUE/simplefm2.pd)
- [TECHNIQUE/simplefm3.pd](../../../farnell/pd/old/TECHNIQUE/simplefm3.pd)
- [TECHNIQUE/sounds/wave1.wav](../../../farnell/pd/old/TECHNIQUE/sounds/wave1.wav)
- [TECHNIQUE/spartial.pd](../../../farnell/pd/old/TECHNIQUE/spartial.pd)
- [TECHNIQUE/timestretch-voice.pd](../../../farnell/pd/old/TECHNIQUE/timestretch-voice.pd)
- [TECHNIQUE/timestretch1.pd](../../../farnell/pd/old/TECHNIQUE/timestretch1.pd)
- [TECHNIQUE/tmp.pd](../../../farnell/pd/old/TECHNIQUE/tmp.pd)
- [TECHNIQUE/vector1.pd](../../../farnell/pd/old/TECHNIQUE/vector1.pd)
- [TECHNIQUE/wavetable1.pd](../../../farnell/pd/old/TECHNIQUE/wavetable1.pd)
- [TECHNIQUE/wavetable2.pd](../../../farnell/pd/old/TECHNIQUE/wavetable2.pd)
- [TECHNIQUE/wavetable3.pd](../../../farnell/pd/old/TECHNIQUE/wavetable3.pd)

There are also two byte-identical pairs **within this unmatched old set**, so retain at least one member of each:

| Old file | Identical old file |
| --- | --- |
| `TECHNIQUE/tmp.pd` | `TECHNIQUE/additive1.pd` |
| `TECHNIQUE/phase-distortion0.pd` | `TECHNIQUE/moorer-dsf2.pd` |

`tmp.pd` is the clearer candidate to remove in the first pair. Either name in the second pair may help locate the example by topic; neither has a matching copy in `pd`.

## Dependencies to preserve during cleanup

Content redundancy alone does not mean that removing a file preserves local patch loading. A static scan of all 47 unmatched old files, following literal local abstractions and assets transitively, finds these **12 exact duplicates used by retained old variants**:

| Duplicate old support file | Referencing old patch |
| --- | --- |
| `BUBBLES/expcurve~.pd` | `BUBBLES/dripfactory.pd` |
| `FOOTSTEPS/splitphase.pd` | `FOOTSTEPS/foot2.pd` |
| `GUNS/bp-programmer.pd` | `GUNS/guns1.pd` |
| `GUNS/bpar8~.pd` | `GUNS/guns1.pd` |
| `GUNS/combsweep.pd` | `GUNS/guns1.pd` |
| `GUNS/reload.pd` | `GUNS/guns1.pd` |
| `MAMMALS/articulate.pd` | `MAMMALS/roar.pd` |
| `RAIN/gaussianoise.pd` | `RAIN/raindrops.pd` |
| `RAIN/raindrops.pd` | `RAIN/rainonwater.pd` |
| `RUNNINGWATER/waterflow.pd` | `RUNNINGWATER/running-water4.pd` |
| `WATER/abs-bubble.pd` | `WATER/bubble2c.pd` |
| `WATER/env-4pow-up.pd` | `WATER/abs-bubble.pd` |

Keep these locally if the retained old patches should remain loadable without a new search-path setup. The count includes support for the optional `roar.pd` window-state variant. This scan does not resolve external libraries, `declare`/search paths, `clone`, or dynamically constructed paths, and is not proof that the original patches run successfully.

There is also a reverse dependency worth preserving: the byte-identical `MOTORS/motor-env-test.pd`, `motor1.pd` and `motor2.pd` load the **different** old `motorenv.pd`. The same entry patches opened from `pd/MOTORS` use that directory's different envelope. Keep the old entry patches, or make explicit wrappers that select the old abstraction, to retain an easily runnable comparison.

Recommended cleanup: retain the README, distinct examples and assets; remove exact duplicates where they are not needed to load those examples; retain the support files and motor entry patches above until equivalent wrappers/search paths are tested. The two window-state variants and two internal duplicate pairs are optional further reductions. Keep `pd` untouched.

## Reproduce the inventory

From the repository root, using Python 3.9 or later and only the standard library:

```powershell
python tools/compare_farnell_trees.py
```

This regenerates `old-redundancy.json`; it never edits either input tree. Each `files` entry has `pd_matches` (all exact copies), `canvas_only_pd_matches` (separate non-byte candidates), and `local_old_references`. The prose observations above describe this snapshot and should be reviewed if the sources change.
