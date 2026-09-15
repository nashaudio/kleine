# PD primitive coverage and usage

Snapshot: 2026-09-15. See [model coverage](COVERAGE.md), [Klang review backlog](KLANG-REVIEW.md), and [isolated comparison evidence](../tests/pd/README.md).

## Overview

The active inventory contains **732 patch files**: 298 website files and 434 bulk files, representing **486 byte-distinct contents**. The parked `pd/old` archive is excluded. There are **127 distinct normalised Vanilla node names used**, plus bundled abstractions and non-Vanilla/local/dynamic objects listed separately.

Eleven dedicated structs exist in [include/klang/pd.h](../include/klang/pd.h). **Ten have isolated comparison fixtures; `vcf` remains unvalidated.** New ports are `phasor`, `cos`, `wrap`, `line` and the default-window `env`. The bell's limited decay helper also has a fixture, but is not a full `vline~` port. All 40 retained cases passed across 48/44.1 kHz on PD 0.55.2; this describes those fixtures, not universal parity. [Results](../tests/pd/results.json) and [source revisions](../tests/pd/sources.json) preserve the evidence.

![Complete](status/complete.svg) · ![Current](status/current.svg) · ![Problem](status/problem.svg) · ![Not started](status/not-started.svg) · ![—](status/none.svg)

**Node coverage:** ![8.5% (10/118) complete; 5 current, 0 problematic, 103 not started](status/progress-10-5-0-103.svg)

Green means validated within the retained fixture scope; amber means an existing implementation/helper still needs work. White includes native/control candidates whose PD semantics have not yet been checked. Grey GUI rows need no dedicated primitive port and are excluded from the percentage; model control translation still applies. Each remaining node counts once, regardless of usage frequency. Red in the dependency table marks unresolved source requirements, not a failed audio test.

## Counting rules

`Web patches` counts files containing the node at least once; `Web nodes` counts saved object boxes in all 298 website patches. `All patches/nodes` counts one representative of each exact-content hash across both active trees. Changed revisions remain distinct. Aliases (`t/trigger`, `f/float`, `i/int`, `b/bang`, `s/send`, `r/receive`, signal send/receive, `del/delay`, `sel/select`) are combined. Embedded subpatch contents are counted once as saved; an abstraction called ten times is not expanded ten times. These are source usage counts, not runtime instances or CPU cost.

Message boxes, comments, array data and GUI atom boxes are not primitives. Named GUI objects such as `hsl` are counted but separated from DSP work. Local abstractions and bundled `hilbert~`/`rev3~` are not misclassified as primitive ports. Whole-graph scheduling, summing and inlet semantics still need preserving when replacing PD glue with native code.

## Next port work

Prioritise `vcf~` verification, queued `vline~` ramps and general delays (`vd~`, `delread~`, `delwrite~`), then `samphold~`, `rzero~` and further noise/envelope work as required by the next practical. The Artificial Sounds trial now covers `phasor~`, `cos~`, `wrap~`, block-64 `line~` and default `env~`. High counts alone do not justify porting every control object: arithmetic, lists and GUI logic often translate more clearly into Klang/C++. Police fixtures distinguish the two `pow~` inlet conventions; wider numeric-domain compatibility remains open.

## Vanilla node inventory

| PD node | Web patches | Web nodes | All patches | All nodes | Port status | Klang path/type | Remaining work |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `*~` | 211 | 1678 | 353 | 2769 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `dac~` | 134 | 155 | 233 | 263 | ![Not started](status/not-started.svg) Host/scheduling gap | Kleine / model wiring | Check block delays, fan-in, feedback, events and graph enable/disable; [K-009](KLANG-REVIEW.md#k-009). |
| `+~` | 123 | 399 | 198 | 613 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `outlet~` | 115 | 355 | 200 | 621 | ![Not started](status/not-started.svg) Host/scheduling gap | Kleine / model wiring | Check block delays, fan-in, feedback, events and graph enable/disable; [K-009](KLANG-REVIEW.md#k-009). |
| `inlet` | 97 | 356 | 173 | 648 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `inlet~` | 95 | 312 | 156 | 506 | ![Not started](status/not-started.svg) Host/scheduling gap | Kleine / model wiring | Check block delays, fan-in, feedback, events and graph enable/disable; [K-009](KLANG-REVIEW.md#k-009). |
| `osc~` | 93 | 225 | 147 | 356 | ![Complete](status/complete.svg) Tested port | pd::osc | Phase, negative frequency and FM fixtures; exact decoded samples in tested cases. |
| `+` | 78 | 259 | 132 | 495 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `trigger` | 77 | 292 | 144 | 571 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `sig~` | 76 | 184 | 117 | 290 | ![Not started](status/not-started.svg) Native candidate | signal / param | Preserve PD block quantisation where audible; pulse trial uses host event rounding. |
| `-~` | 75 | 216 | 125 | 375 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `metro` | 73 | 93 | 107 | 147 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `lop~` | 72 | 220 | 118 | 339 | ![Complete](status/complete.svg) Tested port | pd::lop | 100 Hz impulse at two rates; broader modulation/reset coverage remains. |
| `loadbang` | 70 | 122 | 103 | 180 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `bng` | 68 | 111 | 109 | 225 | ![—](status/none.svg) Host/UI translation | Controls / native state | No DSP port; preserve defaults, ranges and event behaviour. |
| `*` | 67 | 386 | 110 | 577 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `receive` | 66 | 297 | 91 | 452 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `hip~` | 63 | 154 | 114 | 272 | ![Complete](status/complete.svg) Tested port | pd::hip | Impulse / zero cutoff / high cutoff / legacy normalisation fixtures. |
| `noise~` | 61 | 133 | 105 | 214 | ![Complete](status/complete.svg) Tested port | pd::noise | Explicit-seed sample parity; default global seed order remains an integration concern. |
| `phasor~` | 61 | 121 | 89 | 154 | ![Complete](status/complete.svg) Tested port | pd::phasor | Positive/negative frequency, initial phase and wrap at 48/44.1 kHz; preserve 64-sample phase maintenance. |
| `send` | 60 | 119 | 82 | 223 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `float` | 58 | 111 | 103 | 213 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `bp~` | 53 | 213 | 99 | 419 | ![Complete](status/complete.svg) Tested port | pd::bpf | Two frequency/Q fixtures; keep existing name until reviewed. |
| `vline~` | 44 | 79 | 79 | 135 | ![Current](status/current.svg) Limited model helper | TelephoneBell::Decay | Only immediate attack plus linear decay is tested; no queued/delayed ramp contract. [K-007](KLANG-REVIEW.md#k-007). |
| `clip~` | 44 | 120 | 78 | 210 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `tabwrite~` | 42 | 98 | 63 | 137 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `tgl` | 41 | 61 | 68 | 104 | ![—](status/none.svg) Host/UI translation | Controls / native state | No DSP port; preserve defaults, ranges and event behaviour. |
| `cos~` | 41 | 104 | 67 | 173 | ![Complete](status/complete.svg) Tested port | pd::cos | Negative/positive cycle lookup; exact fixture samples and alarm waveshaping comparisons. |
| `receive~` | 41 | 102 | 58 | 134 | ![Not started](status/not-started.svg) Host/scheduling gap | Kleine / model wiring | Check block delays, fan-in, feedback, events and graph enable/disable; [K-009](KLANG-REVIEW.md#k-009). |
| `send~` | 41 | 79 | 55 | 101 | ![Not started](status/not-started.svg) Host/scheduling gap | Kleine / model wiring | Check block delays, fan-in, feedback, events and graph enable/disable; [K-009](KLANG-REVIEW.md#k-009). |
| `delwrite~` | 36 | 63 | 60 | 103 | ![Current](status/current.svg) Limited model helper | TelephoneBell::Delay / Police::Environment | Police feedback impulse is sample-identical at both rates; no general named-buffer port. [K-008](KLANG-REVIEW.md#k-008). |
| `/` | 35 | 103 | 58 | 151 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `hsl` | 33 | 155 | 59 | 264 | ![—](status/none.svg) Host/UI translation | Controls / native state | No DSP port; preserve defaults, ranges and event behaviour. |
| `select` | 31 | 53 | 56 | 137 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `cnv` | 29 | 76 | 34 | 85 | ![—](status/none.svg) Host/UI translation | Controls / native state | No DSP port; preserve defaults, ranges and event behaviour. |
| `delay` | 28 | 108 | 54 | 242 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `/~` | 27 | 61 | 36 | 83 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `max~` | 26 | 32 | 48 | 60 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `random` | 26 | 110 | 47 | 164 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `unpack` | 26 | 40 | 45 | 66 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `-` | 26 | 65 | 42 | 92 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `pack` | 23 | 46 | 48 | 80 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `line~` | 21 | 37 | 35 | 59 | ![Complete](status/complete.svg) Tested port | pd::line | 64-sample grid; short ramp, retarget, stop, immediate set at both rates. No arbitrary block size contract. |
| `delread~` | 20 | 45 | 36 | 74 | ![Current](status/current.svg) Limited model helper | TelephoneBell::Delay / Police::Environment | Fixed taps and routing delays tested; arbitrary delay/control/reset semantics remain. [K-008](KLANG-REVIEW.md#k-008). |
| `min~` | 20 | 30 | 29 | 46 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `vd~` | 19 | 49 | 32 | 75 | ![Not started](status/not-started.svg) Not ported | candidate: Klang Delay | PD interpolation, minimum delay and block ordering must be checked. [K-008](KLANG-REVIEW.md#k-008). |
| `wrap~` | 19 | 47 | 31 | 69 | ![Complete](status/complete.svg) Tested port | pd::wrap | Signed phase ramp including negative integers; finite-input fixture scope. |
| `mod` | 17 | 20 | 31 | 34 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `throw~` | 17 | 82 | 28 | 126 | ![Not started](status/not-started.svg) Host/scheduling gap | Kleine / model wiring | Check block delays, fan-in, feedback, events and graph enable/disable; [K-009](KLANG-REVIEW.md#k-009). |
| `swap` | 17 | 34 | 27 | 49 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `vcf~` | 17 | 39 | 22 | 55 | ![Current](status/current.svg) Present, unvalidated | pd::vcf | Audit real/imaginary outlets, Q, frequency modulation and coefficient refresh; [K-005](KLANG-REVIEW.md#k-005). |
| `outlet` | 16 | 49 | 38 | 142 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `catch~` | 16 | 29 | 23 | 47 | ![Not started](status/not-started.svg) Host/scheduling gap | Kleine / model wiring | Check block delays, fan-in, feedback, events and graph enable/disable; [K-009](KLANG-REVIEW.md#k-009). |
| `moses` | 12 | 27 | 23 | 49 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `>` | 11 | 38 | 21 | 60 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `nbx` | 11 | 22 | 14 | 29 | ![—](status/none.svg) Host/UI translation | Controls / native state | No DSP port; preserve defaults, ranges and event behaviour. |
| `sqrt~` | 10 | 20 | 18 | 38 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ square root | PD 0.55 changed this algorithm; test pre-0.55 compatibility separately (installed unops-tilde help). |
| `route` | 10 | 16 | 16 | 23 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `line` | 10 | 36 | 15 | 46 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `tabread4~` | 10 | 14 | 12 | 18 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `pow~` | 8 | 12 | 27 | 39 | ![Current](status/current.svg) Native subset validated | Police::LogOsc / std::pow | Both police inlet conventions compared with PD; other domains and legacy numeric approximations remain. |
| `env~` | 8 | 36 | 15 | 58 | ![Complete](status/complete.svg) Tested port | pd::env | Default 1024-point Hann/512-sample hop only; detector states match. Configurable windows remain outside this port's scope. |
| `vsl` | 8 | 11 | 15 | 38 | ![—](status/none.svg) Host/UI translation | Controls / native state | No DSP port; preserve defaults, ranges and event behaviour. |
| `until` | 8 | 15 | 14 | 39 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `==` | 8 | 19 | 13 | 24 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `soundfiler` | 8 | 8 | 12 | 12 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `tabwrite` | 8 | 17 | 12 | 39 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `change` | 6 | 8 | 10 | 12 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `openpanel` | 6 | 6 | 10 | 10 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `expr` | 6 | 10 | 9 | 51 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `tabosc4~` | 6 | 9 | 7 | 10 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `spigot` | 5 | 17 | 9 | 22 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `sqrt` | 5 | 9 | 9 | 16 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `print` | 5 | 5 | 7 | 7 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `list` | 4 | 17 | 14 | 43 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `tabsend~` | 4 | 9 | 9 | 22 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `int` | 4 | 6 | 8 | 13 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `max` | 4 | 9 | 8 | 15 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `switch~` | 4 | 29 | 8 | 45 | ![Not started](status/not-started.svg) Host/scheduling gap | Kleine / model wiring | Check block delays, fan-in, feedback, events and graph enable/disable; [K-009](KLANG-REVIEW.md#k-009). |
| `rzero~` | 4 | 6 | 7 | 10 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `clip` | 4 | 5 | 5 | 6 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `block~` | 3 | 7 | 11 | 21 | ![Not started](status/not-started.svg) Host/scheduling gap | Kleine / model wiring | Check block delays, fan-in, feedback, events and graph enable/disable; [K-009](KLANG-REVIEW.md#k-009). |
| `samphold~` | 3 | 3 | 9 | 10 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `tabplay~` | 3 | 3 | 8 | 8 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `rfft~` | 3 | 7 | 7 | 17 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `rpole~` | 3 | 4 | 6 | 7 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `mtof` | 3 | 5 | 5 | 7 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `pow` | 3 | 6 | 5 | 11 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `table` | 3 | 3 | 5 | 5 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `expr~` | 3 | 8 | 4 | 10 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `notein` | 3 | 3 | 4 | 4 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `makefilename` | 3 | 3 | 3 | 3 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `timer` | 3 | 3 | 3 | 3 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `biquad~` | 2 | 2 | 5 | 10 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `&&` | 2 | 2 | 4 | 19 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `min` | 2 | 2 | 4 | 5 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `pipe` | 2 | 5 | 4 | 10 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `textfile` | 2 | 3 | 4 | 5 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `cos` | 2 | 3 | 3 | 10 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `dbtorms` | 2 | 2 | 3 | 3 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `fexpr~` | 2 | 3 | 3 | 4 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `symbol` | 2 | 2 | 3 | 3 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `tabread` | 2 | 10 | 3 | 11 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `threshold~` | 2 | 2 | 3 | 3 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `ctlin` | 2 | 2 | 2 | 2 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `exp` | 2 | 2 | 2 | 2 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `tabread~` | 2 | 12 | 2 | 12 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `writesf~` | 1 | 1 | 4 | 4 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `adc~` | 1 | 1 | 3 | 3 | ![Not started](status/not-started.svg) Host/scheduling gap | Kleine / model wiring | Check block delays, fan-in, feedback, events and graph enable/disable; [K-009](KLANG-REVIEW.md#k-009). |
| `bang` | 1 | 3 | 3 | 5 | ![Not started](status/not-started.svg) Native/control translation | param / state / event code | Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime. |
| `<=` | 1 | 4 | 2 | 8 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `snapshot~` | 1 | 1 | 2 | 2 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `stripnote` | 1 | 1 | 2 | 2 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `!=` | 1 | 4 | 1 | 4 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `bang~` | 1 | 1 | 1 | 1 | ![Not started](status/not-started.svg) Host/scheduling gap | Kleine / model wiring | Check block delays, fan-in, feedback, events and graph enable/disable; [K-009](KLANG-REVIEW.md#k-009). |
| `makenote` | 1 | 1 | 1 | 1 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `noteout` | 1 | 1 | 1 | 1 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `poly` | 1 | 1 | 1 | 1 | ![Not started](status/not-started.svg) Not ported | — | Add isolated fixture when first required; assess existing Klang equivalents. |
| `q8_sqrt~` | 1 | 1 | 1 | 1 | ![Not started](status/not-started.svg) Legacy compatibility gap | — | Now an alias of sqrt~ in PD 0.55.2; older fast approximation needs separate reference. |
| `samplerate~` | 1 | 1 | 1 | 1 | ![Not started](status/not-started.svg) Host/scheduling gap | Kleine / model wiring | Check block delays, fan-in, feedback, events and graph enable/disable; [K-009](KLANG-REVIEW.md#k-009). |
| `sin` | 1 | 3 | 1 | 3 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `abs~` | 0 | 0 | 2 | 2 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `%` | 0 | 0 | 1 | 1 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |
| `hradio` | 0 | 0 | 1 | 1 | ![—](status/none.svg) Host/UI translation | Controls / native state | No DSP port; preserve defaults, ranges and event behaviour. |
| `vradio` | 0 | 0 | 1 | 1 | ![—](status/none.svg) Host/UI translation | Controls / native state | No DSP port; preserve defaults, ranges and event behaviour. |
| `vu` | 0 | 0 | 1 | 1 | ![—](status/none.svg) Host/UI translation | Controls / native state | No DSP port; preserve defaults, ranges and event behaviour. |
| `wrap` | 0 | 0 | 1 | 1 | ![Not started](status/not-started.svg) Native candidate | Klang/C++ arithmetic | Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity. |

## Local abstractions, bundled helpers and unresolved objects

These names are not counted as missing Vanilla primitive ports. A name can exist somewhere in the collection but still be absent from a caller's local directory. Candidate files are leads, not an automatic search-path fix; the coverage table records literal local dependencies. External identity is provisional unless verified by its source/package.

| Object | Web patches | Web nodes | All patches | All nodes | Kind | Source / resolution work |
| --- | --- | --- | --- | --- | --- | --- |
| `timebase` | 4 | 4 | 4 | 4 | ![—](status/none.svg) Local abstraction | [pd/ABOUTPD/timebase.pd](pd/ABOUTPD/timebase.pd), [pd/PD-COMMON/timebase.pd](pd/PD-COMMON/timebase.pd), [zip/ch14/timebase.pd](zip/ch14/timebase.pd) |
| `fcpan` | 3 | 9 | 8 | 18 | ![—](status/none.svg) Local abstraction | [pd/WIND/fcpan.pd](pd/WIND/fcpan.pd), [zip/p18/fcpan.pd](zip/p18/fcpan.pd) |
| `expcurve~` | 3 | 3 | 5 | 5 | ![—](status/none.svg) Local abstraction | [pd/BUBBLES/expcurve~.pd](pd/BUBBLES/expcurve~.pd), [pd/SIGNALS/expcurve~.pd](pd/SIGNALS/expcurve~.pd), [zip/p12/expcurve~.pd](zip/p12/expcurve~.pd) |
| `bangburst` | 3 | 3 | 4 | 4 | ![—](status/none.svg) Local abstraction | [pd/CLOCK/bangburst.pd](pd/CLOCK/bangburst.pd), [zip/p20/bangburst.pd](zip/p20/bangburst.pd) |
| `rev3~` | 3 | 3 | 4 | 4 | ![—](status/none.svg) Vanilla bundled abstraction | Resolve from Pd distribution / original package |
| `ead~` | 2 | 28 | 11 | 91 | ![Problem](status/problem.svg) External / unresolved | not local in 11 files; e.g. [pd/analysis-impulse.pd](pd/analysis-impulse.pd) |
| `combsweep` | 2 | 2 | 4 | 4 | ![—](status/none.svg) Local abstraction | [pd/EXPLOSIONS/combsweep.pd](pd/EXPLOSIONS/combsweep.pd), [pd/GUNS/combsweep.pd](pd/GUNS/combsweep.pd), [zip/p30/combsweep.pd](zip/p30/combsweep.pd), [zip/p31/combsweep.pd](zip/p31/combsweep.pd) |
| `mclick` | 2 | 2 | 4 | 4 | ![—](status/none.svg) Local abstraction | [pd/CLOCK/mclick.pd](pd/CLOCK/mclick.pd), [zip/p20/mclick.pd](zip/p20/mclick.pd) |
| `bubblepattern` | 2 | 2 | 3 | 3 | ![—](status/none.svg) Local abstraction | [pd/BUBBLES/bubblepattern.pd](pd/BUBBLES/bubblepattern.pd), [zip/p12/bubblepattern.pd](zip/p12/bubblepattern.pd) |
| `bubblesound` | 2 | 5 | 3 | 9 | ![—](status/none.svg) Local abstraction | [pd/BUBBLES/bubblesound.pd](pd/BUBBLES/bubblesound.pd), [zip/p12/bubblesound.pd](zip/p12/bubblesound.pd) |
| `cycleround` | 2 | 2 | 3 | 3 | ![—](status/none.svg) Local abstraction | [pd/BUBBLES/cycleround.pd](pd/BUBBLES/cycleround.pd), [zip/p12/cycleround.pd](zip/p12/cycleround.pd) |
| `motorenv` | 2 | 2 | 3 | 3 | ![—](status/none.svg) Local abstraction | [pd/MOTORS/motorenv.pd](pd/MOTORS/motorenv.pd), [zip/p21/motorenv.pd](zip/p21/motorenv.pd) |
| `partialgroup` | 2 | 15 | 3 | 16 | ![—](status/none.svg) Local abstraction | [pd/BELL/partialgroup.pd](pd/BELL/partialgroup.pd), [zip/p06/partialgroup.pd](zip/p06/partialgroup.pd) |
| `sqdec` | 2 | 5 | 3 | 6 | ![—](status/none.svg) Local abstraction | [pd/CLOCK/sqdec.pd](pd/CLOCK/sqdec.pd), [zip/p20/sqdec.pd](zip/p20/sqdec.pd) |
| `vposc` | 2 | 3 | 3 | 5 | ![—](status/none.svg) Local abstraction | [pd/BIRDS/vposc.pd](pd/BIRDS/vposc.pd), [zip/p28/vposc.pd](zip/p28/vposc.pd) |
| `comb1` | 2 | 2 | 2 | 2 | ![—](status/none.svg) Local abstraction | [pd/ELECTRICITY/comb1.pd](pd/ELECTRICITY/comb1.pd), [zip/p16/comb1.pd](zip/p16/comb1.pd) |
| `sin~` | 2 | 4 | 2 | 4 | ![Problem](status/problem.svg) External / unresolved | not local in 3 files; e.g. [pd/SHAPING/shaping-expressions.pd](pd/SHAPING/shaping-expressions.pd) |
| `init` | 1 | 2 | 5 | 11 | ![Problem](status/problem.svg) External / unresolved | not local in 5 files; e.g. [pd/CLOCK/clock2.pd](pd/CLOCK/clock2.pd) |
| `\$1` | 1 | 2 | 4 | 6 | ![—](status/none.svg) Dynamic object name | not local in 5 files; e.g. [pd/ALARMS/sand.pd](pd/ALARMS/sand.pd) |
| `bp-programmer` | 1 | 1 | 4 | 4 | ![—](status/none.svg) Local abstraction | [pd/EXPLOSIONS/bp-programmer.pd](pd/EXPLOSIONS/bp-programmer.pd), [pd/GUNS/bp-programmer.pd](pd/GUNS/bp-programmer.pd), [zip/p30/bp-programmer.pd](zip/p30/bp-programmer.pd) |
| `bpar8~` | 1 | 1 | 4 | 4 | ![—](status/none.svg) Local abstraction | [pd/EXPLOSIONS/bpar8~.pd](pd/EXPLOSIONS/bpar8~.pd), [pd/GUNS/bpar8~.pd](pd/GUNS/bpar8~.pd), [zip/p30/bpar8~.pd](zip/p30/bpar8~.pd) |
| `articulate` | 1 | 1 | 3 | 3 | ![—](status/none.svg) Local abstraction | [pd/BIRDS/articulate.pd](pd/BIRDS/articulate.pd), [pd/MAMMALS/articulate.pd](pd/MAMMALS/articulate.pd), [zip/p29/articulate.pd](zip/p29/articulate.pd) |
| `gaussianoise` | 1 | 1 | 3 | 3 | ![—](status/none.svg) Local abstraction | [pd/RAIN/gaussianoise.pd](pd/RAIN/gaussianoise.pd), [zip/p15/gaussianoise.pd](zip/p15/gaussianoise.pd) |
| `ln~` | 1 | 1 | 3 | 3 | ![Problem](status/problem.svg) External / unresolved | not local in 4 files; e.g. [pd/RAIN/gaussianoise.pd](pd/RAIN/gaussianoise.pd) |
| `raindrops` | 1 | 1 | 3 | 3 | ![Problem](status/problem.svg) Local abstraction | [pd/RAIN/raindrops.pd](pd/RAIN/raindrops.pd); not local in 1 files; e.g. [zip/p15/rain_on_water.pd](zip/p15/rain_on_water.pd) |
| `spark6formant` | 1 | 1 | 3 | 3 | ![Problem](status/problem.svg) Local abstraction | [pd/ELECTRICITY/spark6formant.pd](pd/ELECTRICITY/spark6formant.pd), [pd/EXPLOSIONS/spark6formant.pd](pd/EXPLOSIONS/spark6formant.pd), [pd/REVERB/spark6formant.pd](pd/REVERB/spark6formant.pd), [pd/SCIFI/spark6formant.pd](pd/SCIFI/spark6formant.pd); not local in 1 files; e.g. [zip/p16/snap.pd](zip/p16/snap.pd) |
| `udly` | 1 | 24 | 3 | 72 | ![—](status/none.svg) Local abstraction | [pd/EXPLOSIONS/udly.pd](pd/EXPLOSIONS/udly.pd), [pd/THUNDER/udly.pd](pd/THUNDER/udly.pd), [zip/p17/udly.pd](zip/p17/udly.pd) |
| `adenv2~` | 1 | 1 | 2 | 2 | ![—](status/none.svg) Local abstraction | [pd/BUBBLES/adenv2~.pd](pd/BUBBLES/adenv2~.pd), [zip/p12/adenv2~.pd](zip/p12/adenv2~.pd) |
| `avian-syrinx-model` | 1 | 1 | 2 | 2 | ![—](status/none.svg) Local abstraction | [pd/BIRDS/avian-syrinx-model.pd](pd/BIRDS/avian-syrinx-model.pd), [zip/p28/avian-syrinx-model.pd](zip/p28/avian-syrinx-model.pd) |
| `bellenv` | 1 | 1 | 2 | 2 | ![—](status/none.svg) Local abstraction | [pd/BELL/bellenv.pd](pd/BELL/bellenv.pd), [zip/p06/bellenv.pd](zip/p06/bellenv.pd) |
| `bellosc` | 1 | 3 | 2 | 4 | ![—](status/none.svg) Local abstraction | [pd/BELL/bellosc.pd](pd/BELL/bellosc.pd), [zip/p06/bellosc.pd](zip/p06/bellosc.pd) |
| `bodyscale` | 1 | 2 | 2 | 4 | ![Problem](status/problem.svg) Local abstraction | [pd/CLOCK/bodyscale.pd](pd/CLOCK/bodyscale.pd); not local in 1 files; e.g. [zip/p20/bodyresonance~.pd](zip/p20/bodyresonance~.pd) |
| `bubblesound2` | 1 | 1 | 2 | 2 | ![—](status/none.svg) Local abstraction | [pd/BUBBLES/bubblesound2.pd](pd/BUBBLES/bubblesound2.pd), [zip/p12/bubblesound2.pd](zip/p12/bubblesound2.pd) |
| `chplz` | 1 | 1 | 2 | 2 | ![—](status/none.svg) Local abstraction | [pd/ELECTRICITY/chplz.pd](pd/ELECTRICITY/chplz.pd), [zip/p16/chplz.pd](zip/p16/chplz.pd) |
| `delta~` | 1 | 1 | 2 | 2 | ![Problem](status/problem.svg) External / unresolved | not local in 2 files; e.g. [pd/RUNNINGWATER/water2.pd](pd/RUNNINGWATER/water2.pd) |
| `environment` | 1 | 1 | 2 | 2 | ![—](status/none.svg) Local abstraction | [pd/POLICE/environment.pd](pd/POLICE/environment.pd), [zip/p05/environment.pd](zip/p05/environment.pd) |
| `glasswindow` | 1 | 1 | 2 | 2 | ![—](status/none.svg) Local abstraction | [pd/RAIN/glasswindow.pd](pd/RAIN/glasswindow.pd), [zip/p15/glasswindow.pd](zip/p15/glasswindow.pd) |
| `my-tabosc2` | 1 | 3 | 2 | 6 | ![—](status/none.svg) Local abstraction | [pd/ABSTRACTIONS/my-tabosc2.pd](pd/ABSTRACTIONS/my-tabosc2.pd), [zip/ch12/my-tabosc2.pd](zip/ch12/my-tabosc2.pd) |
| `pnoise` | 1 | 1 | 2 | 2 | ![—](status/none.svg) Local abstraction | [pd/SWITCHES/pnoise.pd](pd/SWITCHES/pnoise.pd), [zip/p19/pnoise.pd](zip/p19/pnoise.pd) |
| `randgate` | 1 | 1 | 2 | 2 | ![—](status/none.svg) Local abstraction | [pd/ELECTRICITY/randgate.pd](pd/ELECTRICITY/randgate.pd), [zip/p16/randgate.pd](zip/p16/randgate.pd) |
| `reload` | 1 | 1 | 2 | 2 | ![—](status/none.svg) Local abstraction | [pd/GUNS/reload.pd](pd/GUNS/reload.pd), [zip/p30/reload.pd](zip/p30/reload.pd) |
| `sdel~` | 1 | 1 | 2 | 4 | ![—](status/none.svg) Local abstraction | [pd/EXPLOSIONS/sdel~.pd](pd/EXPLOSIONS/sdel~.pd), [zip/p31/sdel~.pd](zip/p31/sdel~.pd) |
| `splitphase` | 1 | 1 | 2 | 2 | ![—](status/none.svg) Local abstraction | [pd/FOOTSTEPS/splitphase.pd](pd/FOOTSTEPS/splitphase.pd), [zip/p26/splitphase.pd](zip/p26/splitphase.pd) |
| `trachea` | 1 | 1 | 2 | 2 | ![—](status/none.svg) Local abstraction | [pd/BIRDS/trachea.pd](pd/BIRDS/trachea.pd), [zip/p28/trachea.pd](zip/p28/trachea.pd) |
| `>~` | 1 | 4 | 1 | 4 | ![Problem](status/problem.svg) External / unresolved | not local in 2 files; e.g. [pd/INSECTS/insectsall.pd](pd/INSECTS/insectsall.pd) |
| `GOP-hardsynth` | 1 | 1 | 1 | 1 | ![—](status/none.svg) Local abstraction | [pd/ABSTRACTIONS/GOP-hardsynth.pd](pd/ABSTRACTIONS/GOP-hardsynth.pd), [zip/ch12/GOP-hardsynth.pd](zip/ch12/GOP-hardsynth.pd) |
| `adenv~` | 1 | 1 | 1 | 1 | ![—](status/none.svg) Local abstraction | [pd/BUBBLES/adenv~.pd](pd/BUBBLES/adenv~.pd), [zip/p12/adenv~.pd](zip/p12/adenv~.pd) |
| `bodyresonance~` | 1 | 1 | 1 | 1 | ![—](status/none.svg) Local abstraction | [pd/CLOCK/bodyresonance~.pd](pd/CLOCK/bodyresonance~.pd), [zip/p20/bodyresonance~.pd](zip/p20/bodyresonance~.pd) |
| `clockhand` | 1 | 1 | 1 | 1 | ![—](status/none.svg) Local abstraction | [pd/CLOCK/clockhand.pd](pd/CLOCK/clockhand.pd), [zip/p20/clockhand.pd](zip/p20/clockhand.pd) |
| `clocktick` | 1 | 1 | 1 | 1 | ![—](status/none.svg) Local abstraction | [pd/CLOCK/clocktick.pd](pd/CLOCK/clocktick.pd), [zip/p20/clocktick.pd](zip/p20/clocktick.pd) |
| `clocktick2` | 1 | 1 | 1 | 1 | ![—](status/none.svg) Local abstraction | [pd/CLOCK/clocktick2.pd](pd/CLOCK/clocktick2.pd), [zip/p20/clocktick2.pd](zip/p20/clocktick2.pd) |
| `cmverb~` | 1 | 1 | 1 | 1 | ![—](status/none.svg) Local abstraction | [zip/p12/cmverb~.pd](zip/p12/cmverb~.pd) |
| `cpraingen` | 1 | 3 | 1 | 3 | ![—](status/none.svg) Local abstraction | [pd/RAIN/cpraingen.pd](pd/RAIN/cpraingen.pd), [zip/p15/cpraingen.pd](zip/p15/cpraingen.pd) |
| `cpulse` | 1 | 1 | 1 | 1 | ![—](status/none.svg) Local abstraction | [pd/RAIN/cpulse.pd](pd/RAIN/cpulse.pd), [zip/p15/cpulse.pd](zip/p15/cpulse.pd) |
| `dfbe` | 1 | 6 | 1 | 6 | ![—](status/none.svg) Local abstraction | [pd/REDALERT/dfbe.pd](pd/REDALERT/dfbe.pd), [zip/p35/dfbe.pd](zip/p35/dfbe.pd) |
| `dfbef` | 1 | 8 | 1 | 8 | ![—](status/none.svg) Local abstraction | [pd/CREAKING/dfbef.pd](pd/CREAKING/dfbef.pd), [zip/p09/dfbef.pd](zip/p09/dfbef.pd) |
| `distance` | 1 | 1 | 1 | 1 | ![—](status/none.svg) Local abstraction | [pd/THUNDER/distance.pd](pd/THUNDER/distance.pd), [zip/p17/distance.pd](zip/p17/distance.pd) |
| `drops` | 1 | 1 | 1 | 1 | ![—](status/none.svg) Local abstraction | [pd/RAIN/drops.pd](pd/RAIN/drops.pd), [zip/p15/drops.pd](zip/p15/drops.pd) |
| `dropsig` | 1 | 1 | 1 | 1 | ![—](status/none.svg) Local abstraction | [pd/RAIN/dropsig.pd](pd/RAIN/dropsig.pd), [zip/p15/dropsig.pd](zip/p15/dropsig.pd) |
| `escapement` | 1 | 1 | 1 | 1 | ![—](status/none.svg) Local abstraction | [pd/CLOCK/escapement.pd](pd/CLOCK/escapement.pd), [zip/p20/escapement.pd](zip/p20/escapement.pd) |
| `flywing` | 1 | 2 | 1 | 2 | ![Problem](status/problem.svg) Local abstraction | [pd/INSECTS/flywing.pd](pd/INSECTS/flywing.pd); not local in 1 files; e.g. [zip/p27/buzzing-housefly.pd](zip/p27/buzzing-housefly.pd) |
| `grainvoice` | 1 | 4 | 1 | 4 | ![—](status/none.svg) Local abstraction | [zip/ch21/grainvoice.pd](zip/ch21/grainvoice.pd) |
| `grid` | 1 | 1 | 1 | 1 | ![Problem](status/problem.svg) External / unresolved | not local in 1 files; e.g. [zip/ch18/vector1.pd](zip/ch18/vector1.pd) |
| `hilbert~` | 1 | 2 | 1 | 2 | ![—](status/none.svg) Vanilla bundled abstraction | Resolve from Pd distribution / original package |
| `logosc` | 1 | 2 | 1 | 2 | ![—](status/none.svg) Local abstraction | [zip/p05/logosc.pd](zip/p05/logosc.pd) |
| `mainrotor` | 1 | 1 | 1 | 1 | ![—](status/none.svg) Local abstraction | [pd/HELICOPTER/mainrotor.pd](pd/HELICOPTER/mainrotor.pd), [zip/p25/mainrotor.pd](zip/p25/mainrotor.pd) |
| `mclick~` | 1 | 1 | 1 | 1 | ![Problem](status/problem.svg) Local abstraction | [pd/CLOCK/mclick~.pd](pd/CLOCK/mclick~.pd); not local in 1 files; e.g. [zip/p20/clockhand.pd](zip/p20/clockhand.pd) |
| `mdel` | 1 | 3 | 1 | 3 | ![—](status/none.svg) Local abstraction | [pd/HELICOPTER/mdel.pd](pd/HELICOPTER/mdel.pd), [zip/p25/mdel.pd](zip/p25/mdel.pd) |
| `notch` | 1 | 1 | 1 | 1 | ![Problem](status/problem.svg) External / unresolved | not local in 2 files; e.g. [pd/SHAPING/shaping-filters.pd](pd/SHAPING/shaping-filters.pd) |
| `plastichorn` | 1 | 1 | 1 | 1 | ![—](status/none.svg) Local abstraction | [zip/p05/plastichorn.pd](zip/p05/plastichorn.pd) |
| `prepend` | 1 | 1 | 1 | 1 | ![Problem](status/problem.svg) External / unresolved | not local in 1 files; e.g. [zip/ch17/precompute.pd](zip/ch17/precompute.pd) |
| `sampler` | 1 | 1 | 1 | 1 | ![—](status/none.svg) Local abstraction | [pd/PD-COMMON/sampler.pd](pd/PD-COMMON/sampler.pd), [zip/ch14/sampler.pd](zip/ch14/sampler.pd) |
| `shortping` | 1 | 1 | 1 | 1 | ![—](status/none.svg) Local abstraction | [pd/GUNS/shortping.pd](pd/GUNS/shortping.pd), [pd/SWITCHES/shortping.pd](pd/SWITCHES/shortping.pd), [zip/p19/shortping.pd](zip/p19/shortping.pd), [zip/p30/shortping.pd](zip/p30/shortping.pd) |
| `slideclunk` | 1 | 1 | 1 | 1 | ![—](status/none.svg) Local abstraction | [pd/SWITCHES/slideclunk.pd](pd/SWITCHES/slideclunk.pd), [zip/p19/slideclunk.pd](zip/p19/slideclunk.pd) |
| `snap` | 1 | 1 | 1 | 1 | ![—](status/none.svg) Local abstraction | [pd/ELECTRICITY/snap.pd](pd/ELECTRICITY/snap.pd), [pd/SCIFI/snap.pd](pd/SCIFI/snap.pd), [zip/p16/snap.pd](zip/p16/snap.pd) |
| `strike-pattern` | 1 | 1 | 1 | 1 | ![Problem](status/problem.svg) Local abstraction | [pd/THUNDER/strike-pattern.pd](pd/THUNDER/strike-pattern.pd); not local in 1 files; e.g. [zip/p17/thunder4.pd](zip/p17/thunder4.pd) |
| `strike-sound` | 1 | 4 | 1 | 4 | ![—](status/none.svg) Local abstraction | [pd/THUNDER/strike-sound.pd](pd/THUNDER/strike-sound.pd), [zip/p17/strike-sound.pd](zip/p17/strike-sound.pd) |
| `switchclick` | 1 | 4 | 1 | 4 | ![—](status/none.svg) Local abstraction | [pd/SWITCHES/switchclick.pd](pd/SWITCHES/switchclick.pd), [zip/p19/switchclick.pd](zip/p19/switchclick.pd) |
| `tanh` | 1 | 1 | 1 | 1 | ![Problem](status/problem.svg) External / unresolved | not local in 2 files; e.g. [pd/TECHNIQUE/shaper-sound.pd](pd/TECHNIQUE/shaper-sound.pd) |
| `vrdel` | 1 | 1 | 1 | 1 | ![—](status/none.svg) Local abstraction | [pd/HELICOPTER/vrdel.pd](pd/HELICOPTER/vrdel.pd), [zip/p25/vrdel.pd](zip/p25/vrdel.pd) |
| `waterflow` | 1 | 3 | 1 | 3 | ![—](status/none.svg) Local abstraction | [pd/RUNNINGWATER/waterflow.pd](pd/RUNNINGWATER/waterflow.pd), [zip/p13/waterflow.pd](zip/p13/waterflow.pd) |
| `writefile` | 1 | 1 | 1 | 1 | ![—](status/none.svg) Local abstraction | [pd/PD-COMMON/writefile.pd](pd/PD-COMMON/writefile.pd), [zip/ch14/writefile.pd](zip/ch14/writefile.pd) |
| `\$2` | 0 | 0 | 5 | 5 | ![—](status/none.svg) Dynamic object name | not local in 5 files; e.g. [pd/BELL/A3-bell-scale.pd](pd/BELL/A3-bell-scale.pd) |
| `env-4pow-up` | 0 | 0 | 4 | 4 | ![—](status/none.svg) Local abstraction | [pd/WATER/env-4pow-up.pd](pd/WATER/env-4pow-up.pd) |
| `\$3` | 0 | 0 | 3 | 3 | ![—](status/none.svg) Dynamic object name | not local in 3 files; e.g. [pd/ENGINES/2port.pd](pd/ENGINES/2port.pd) |
| `decode-tone` | 0 | 0 | 3 | 22 | ![—](status/none.svg) Local abstraction | [pd/ALARMS/decode-tone.pd](pd/ALARMS/decode-tone.pd) |
| `dirac~` | 0 | 0 | 3 | 3 | ![Problem](status/problem.svg) External / unresolved | not local in 3 files; e.g. [pd/analysis-impulse.pd](pd/analysis-impulse.pd) |
| `telephone-line` | 0 | 0 | 3 | 3 | ![—](status/none.svg) Local abstraction | [pd/ALARMS/telephone-line.pd](pd/ALARMS/telephone-line.pd), [pd/PHONETONES/telephone-line.pd](pd/PHONETONES/telephone-line.pd) |
| `crack` | 0 | 0 | 2 | 5 | ![—](status/none.svg) Local abstraction | [pd/REVERB/crack.pd](pd/REVERB/crack.pd), [pd/SCIFI/crack.pd](pd/SCIFI/crack.pd) |
| `dline~` | 0 | 0 | 2 | 8 | ![—](status/none.svg) Local abstraction | [pd/ENGINES/dline~.pd](pd/ENGINES/dline~.pd) |
| `f2T` | 0 | 0 | 2 | 4 | ![Problem](status/problem.svg) External / unresolved | not local in 2 files; e.g. [pd/ENGINES/dline-IN.pd](pd/ENGINES/dline-IN.pd) |
| `list-dotprod` | 0 | 0 | 2 | 6 | ![Problem](status/problem.svg) External / unresolved | not local in 2 files; e.g. [pd/ALARMS/dt-call-recogniser.pd](pd/ALARMS/dt-call-recogniser.pd) |
| `list-emath` | 0 | 0 | 2 | 6 | ![Problem](status/problem.svg) External / unresolved | not local in 2 files; e.g. [pd/ALARMS/dt-call-recogniser.pd](pd/ALARMS/dt-call-recogniser.pd) |
| `pink~` | 0 | 0 | 2 | 2 | ![Problem](status/problem.svg) External / unresolved | not local in 2 files; e.g. [pd/analysis-impulse.pd](pd/analysis-impulse.pd) |
| `revx-allpass` | 0 | 0 | 2 | 8 | ![—](status/none.svg) Local abstraction | [pd/REVERB/revx-allpass.pd](pd/REVERB/revx-allpass.pd) |
| `revx-circ` | 0 | 0 | 2 | 2 | ![—](status/none.svg) Local abstraction | [pd/REVERB/revx-circ.pd](pd/REVERB/revx-circ.pd) |
| `revx-comb` | 0 | 0 | 2 | 16 | ![—](status/none.svg) Local abstraction | [pd/REVERB/revx-comb.pd](pd/REVERB/revx-comb.pd) |
| `revx-img+` | 0 | 0 | 2 | 8 | ![—](status/none.svg) Local abstraction | [pd/REVERB/revx-img+.pd](pd/REVERB/revx-img%2B.pd) |
| `revx-img-` | 0 | 0 | 2 | 8 | ![—](status/none.svg) Local abstraction | [pd/REVERB/revx-img-.pd](pd/REVERB/revx-img-.pd) |
| `revxp` | 0 | 0 | 2 | 2 | ![—](status/none.svg) Local abstraction | [pd/REVERB/revxp.pd](pd/REVERB/revxp.pd) |
| `sand` | 0 | 0 | 2 | 20 | ![—](status/none.svg) Local abstraction | [pd/ALARMS/sand.pd](pd/ALARMS/sand.pd) |
| `sd` | 0 | 0 | 2 | 7 | ![—](status/none.svg) Local abstraction | [pd/REVERB/sd.pd](pd/REVERB/sd.pd), [pd/SCIFI/sd.pd](pd/SCIFI/sd.pd) |
| `voice1` | 0 | 0 | 2 | 8 | ![Problem](status/problem.svg) Local abstraction | [pd/drum/voice1.pd](pd/drum/voice1.pd); not local in 1 files; e.g. [pd/additive-noise/adnoise.pd](pd/additive-noise/adnoise.pd) |
| `A0-bell-oscillator` | 0 | 0 | 1 | 3 | ![—](status/none.svg) Local abstraction | [pd/BELL/A0-bell-oscillator.pd](pd/BELL/A0-bell-oscillator.pd) |
| `A1-bell-envelope` | 0 | 0 | 1 | 1 | ![—](status/none.svg) Local abstraction | [pd/BELL/A1-bell-envelope.pd](pd/BELL/A1-bell-envelope.pd) |
| `A2-bell-partial` | 0 | 0 | 1 | 3 | ![—](status/none.svg) Local abstraction | [pd/BELL/A2-bell-partial.pd](pd/BELL/A2-bell-partial.pd) |
| `A3-bell-scale` | 0 | 0 | 1 | 9 | ![—](status/none.svg) Local abstraction | [pd/BELL/A3-bell-scale.pd](pd/BELL/A3-bell-scale.pd) |
| `abs-bubble` | 0 | 0 | 1 | 4 | ![—](status/none.svg) Local abstraction | [pd/WATER/abs-bubble.pd](pd/WATER/abs-bubble.pd) |
| `allpass~` | 0 | 0 | 1 | 1 | ![Problem](status/problem.svg) External / unresolved | not local in 1 files; e.g. [pd/SHAPING/shaping-delayfilter.pd](pd/SHAPING/shaping-delayfilter.pd) |
| `envlp-ad` | 0 | 0 | 1 | 1 | ![—](status/none.svg) Local abstraction | [pd/drum/envlp-ad.pd](pd/drum/envlp-ad.pd) |
| `expdec` | 0 | 0 | 1 | 3 | ![—](status/none.svg) Local abstraction | [pd/CLOCK/expdec.pd](pd/CLOCK/expdec.pd), [zip/p20/expdec.pd](zip/p20/expdec.pd) |
| `expdec~` | 0 | 0 | 1 | 4 | ![—](status/none.svg) Local abstraction | [pd/CLOCK/expdec~.pd](pd/CLOCK/expdec~.pd), [pd/SIGNALS/expdec~.pd](pd/SIGNALS/expdec~.pd) |
| `inv` | 0 | 0 | 1 | 2 | ![Problem](status/problem.svg) External / unresolved | not local in 1 files; e.g. [pd/CLOCK/clock4.pd](pd/CLOCK/clock4.pd) |
| `lin3` | 0 | 0 | 1 | 1 | ![—](status/none.svg) Local abstraction | [pd/BIRDS/lin3.pd](pd/BIRDS/lin3.pd) |
| `listosc2` | 0 | 0 | 1 | 1 | ![—](status/none.svg) Local abstraction | [pd/drum/listosc2.pd](pd/drum/listosc2.pd) |
| `logosc~` | 0 | 0 | 1 | 2 | ![—](status/none.svg) Local abstraction | [pd/POLICE/logosc~.pd](pd/POLICE/logosc~.pd) |
| `loop-sample-player` | 0 | 0 | 1 | 1 | ![—](status/none.svg) Local abstraction | [pd/PD-COMMON/loop-sample-player.pd](pd/PD-COMMON/loop-sample-player.pd), [pd/REVERB/loop-sample-player.pd](pd/REVERB/loop-sample-player.pd), [zip/ch14/loop-sample-player.pd](zip/ch14/loop-sample-player.pd) |
| `my-tabosc` | 0 | 0 | 1 | 3 | ![—](status/none.svg) Local abstraction | [pd/ABSTRACTIONS/my-tabosc.pd](pd/ABSTRACTIONS/my-tabosc.pd), [zip/ch12/my-tabosc.pd](zip/ch12/my-tabosc.pd) |
| `partial` | 0 | 0 | 1 | 3 | ![—](status/none.svg) Local abstraction | [pd/BELL/partial.pd](pd/BELL/partial.pd) |
| `plastichorn~` | 0 | 0 | 1 | 1 | ![—](status/none.svg) Local abstraction | [pd/POLICE/plastichorn~.pd](pd/POLICE/plastichorn~.pd) |
| `shell` | 0 | 0 | 1 | 1 | ![Problem](status/problem.svg) External / unresolved | not local in 1 files; e.g. [pd/SHAPING/table-from-file.pd](pd/SHAPING/table-from-file.pd) |

## Maintenance

Rebuild both inventories with `python tools/catalogue_farnell.py` after changing source collections. Update the port registry in that script only when implementation and evidence warrant the new status. Add isolated patches under `tests/pd` for each new/changed primitive; record level/residual, rate, block size, relevant parameter events and source revision. Preserve explicit compatibility variants. Aggregate Klang implementation issues in the review backlog at the end of a practical series or when requested.

Vanilla classification uses the installed PD 0.55.2 reference help and local source evidence, with legacy `q8_sqrt~` retained as a compatibility item. A headless no-preferences creation probe confirmed that `init` and `>~` are unavailable in this Vanilla installation; they remain unresolved/external dependencies. The installed `unops-tilde-help.pd` documents the sqrt algorithm change in 0.55 and the current q8 aliases. This is a working dependency inventory, not a complete list of all Vanilla objects. No primitive code was changed by this inventory.
