# Klang topology review

16 September 2026. Scope: all twelve implemented Farnell model files and their
shared helper, plus the PD primitive API documentation. Original PD patches and
retained listening WAVs are unchanged.

## Decisions from Pedestrians

- The model should read against its specific PD patch: matching objects, constants,
  controls and signal flow. Recording analysis is evidence, not additional topology.
- Use reusable PD objects in place of model-local simulations. Aim for complete
  primitive interfaces; label remaining subsets and discuss awkward interfaces.
- Keep the patch's constants fixed. Put diagnostic controls and fitted performances
  in the renderer; distinguish real source variants from new adaptations.
- Use `set(param on)` and `metro = on`. Inline inexpensive object configuration;
  retain change checks for expensive reconfiguration. Inline short single-use
  expressions; retain reused values and names that explain complex DSP stages.
- Review control/audio pairs individually, favouring one polymorphic class. No
  namespace or suffix convention was adopted. `pd::line` now uses setter arity.
- Keep PD block workarounds explicit and separable. This pass removes model-local
  block rounding/lookahead and routing padding; it does not choose replacements.
- Keep Doxygen API documentation and examples for PD primitives. Models use short
  `//` object descriptions and at most one longer Doxygen overview before the main
  model, following Chris's subsequent comment-style review.

Pedestrians' **sound review is accepted**. Chris hears the phase-set PD 0.55 fixture
as equivalent to the online example. That file was generated with an explicit
initial phase, not taken from a long-running capture. Reconstructed waveform
ringing explains the display observation sufficiently for this review; the exact
Audition interpolation kernel and Andy's recording history remain unproved.
Further historical investigation is closed unless new audible evidence warrants it.

## Model changes

| Model / source | Changes | Remaining primitive or block review |
| --- | --- | --- |
| Pedestrians — `zip/p01/pedestrian-beep.pd` | Retained the agreed pure `osc`/`metro` model; documented start/stop gate behaviour. | 44.1 kHz gate delivery remains sample-timed. |
| Phone tones — `pd/PHONETONES/{dialtone1,dialtone2,busy-signal,ringingtone,pulsedial}.pd`, website p02 variants | Fixed oscillator values appear in the flow; source selection moved to construction. Ringback uses `pd::metro(1000)` and a counter. Removed invented frequency/smoothing UI controls and zero-to-ten pulse-count conversion. | Pulse still uses a local finite-contact timer; complete metro/delay composition should replace it. |
| Phone effects — `pd/ALARMS/{phone-effects,dt-call-recogniser}.pd` | Ringback uses a metro. Removed redundant alarm voice configuration and explicit 64-sample deadline rounding. Pending decoded keys are consumed on the next sample. | Delay objects, env~ message delivery, number-match reconstruction and Tom's coincident delayed messages need review. |
| DTMF — `zip/p03/dtmf.pd`, `pd/ALARMS/{dtmf,dtmf00}.pd` | Removed invented duration/gain/filter setter; named source variants carry the actual constants. Retained keypad and study inlets. Release is 200 ms without block rounding. | Full delay port; line~ release timing at 44.1 kHz. Decoder comparison uses a dialler whose timing has also changed. |
| Alarms — `zip/p04/alarm01..07.pd`, `pd/ALARMS/2tone-{15,20}.pd` | Studies 1/5 use `pd::metro`, `set(on)` and direct counters. Fixed oscillator constants are visible. Voice parameters are applied inline, removing phone-demo reconfiguration. Programme reset no longer rounds to a block. | Full delay port and line~ reset timing. Explicit initial stop now suppresses the harness's extra startup bang in both renderers. |
| Police — `zip/p05/{police_siren,logosc,plastichorn,environment}.pd` and bulk variants | Fixed 300/800 Hz sweep mapping in the flow; removed invented ambience/base/depth controls. Removed 64 extra samples from each 165/121/33 ms echo. | Full named delay ports and feedback/routing order. Historical pow~ variants remain explicit. |
| Telephone Bell — `pd/BELL/striker.pd` | Fixed 650/653 Hz, strength 1, decay 2000 ms and metro 60 ms. `set(on)` replaces UI/ring wrapper. Dry diagnostic moved to renderer. Removed 128-sample outer routing, 64-sample material return and artificial minimum width of 64 samples. | Physical 0.77/0.88 ms casing delays now determine feedback. This can change timbre; review before acceptance. Decay/delay helpers remain incomplete PD ports. |
| Bell studies — `pd/BELL/A0..A4`, partial/group/testgroup | A3 uses `set(on)` and no block lookahead; source's disconnected group stays disconnected. A4 retains its actual 59 ms body delay. | A3 metro/delay sequencing should become full primitive composition. |
| Bouncing — `zip/p07/bouncing.pd`, `pd/BOUNCINGBALL/bb1.pd` | Replaced manual height clock with control `pd::line`; replaced bounce scheduler with `pd::metro`. Kept 3000/20 ms height ramp, dynamic interval, 120 Hz modulation and fixed 0.2 gain. | Full vline queue; dynamic metro/control delivery, especially near the inaudible zero-height tail. |
| Rolling — `zip/p08/rolling1.pd`, `pd/ROLLING/{tincan,uneven,uneven2}.pd` | Removed added output-gain control and Uneven2's 64-sample signal route. Source selection immediately updates can weights. Earlier uneven study has no block lookahead. | Complete vline/control-delay ports for the study; assess Uneven2 routing audibly. |
| Creaking — `zip/p09/{doorcreaker,dfbef}.pd` | Exposed `StickSlip`, `Wood`, `Panel` subpatches. Force uses `pd::line(target,100,20)`. Removed added gain control, block lookahead and 64-sample feedback routes. Pure top-level flow is `stickslip >> wood >> panel`. | Metro outlet feedback must set the next random interval before rearming. Timer/random/vline/named delays still have helpers; this is not claimed as a complete port. |
| Boing — `zip/p10/twang.pd`, `pd/MRBOINGY/twang.pd` | Removed the hidden 64-sample frequency route and added output-gain control. Fixed vibrato rate 5 Hz appears inline; renamed its exposed amount to depth. Single-frequency setter preserves other controls. | Review audible phase change; complete vline queue. Website power 64 and bulk pow~ conventions remain unchanged. |

**Subsequent vline port:** Gesture has been removed and its consumers now use
`pd::vline` target/duration/delay messages. The full queue and message contract is
covered by [50 isolated fixtures](../tests/pd/vline.md); host clock anchoring is
explicit in the driver and absent from the pure model code. The vline queue work
listed above is now complete. The [migration comparisons](../tests/pd/vline-model-results.json)
retain the remaining model timing/routing differences for listening review.

`SampleDelay`, `ControlRandom`, and bell `Decay`/`Delay` remain documented helpers.
Creaking's feedback-sensitive metro is still an interface case to discuss; the
current polling metro cannot preserve synchronous outlet feedback. Legacy
Bouncing now matches PD's zero result for a negative base under a fractional
power; this avoids NaN from a tiny envelope endpoint undershoot. AlarmGenerator's
Bank accumulates directly into `out`, with identical before/after samples.

**Subsequent alternative separation:** PhoneTones, DTMFTones, Police, Bouncing and
Boing no longer contain runtime source-history switches. Their alternate flows
are separate development files, selected by the comparison renderer. Distinct-sound
selectors remain. [File/source mapping and verification](klang/VARIANTS.md) supersede
the construction/source-selection descriptions above; no listening winner is chosen.

**Subsequent DTMF review:** The dialler now uses a reusable `pd::del{200}`;
[52 isolated cases](../tests/pd/del.md) cover its message and tempo contract.
`DTMFTones::Sand` explicitly ports `sand.pd`; separate `DigitDecoder` and `Decoder`
classes preserve the two source graphs' message routing. All 56 shared-input
decoder cases pass, and 18 existing audio recipes remain sample-identical to
the pre-revision executable. See [evidence](../tests/pd/dtmf-messages.md).
This resolves the DTMF delay and decoder-topology work above. Other local timers,
the reconstructed number matchers and any audible need for block delivery remain
separate model reviews.

## Shared line API

```cpp
pd::line line;
line.set(1, 100);       // line~: audio ramp, duration in milliseconds.
line.set(0, 500, 20);   // line: held output, emitted every 20 ms.
line = { {0, 1}, {3000, 0}, 20 }; // Two {timeMs,value} endpoints, control grain.
line.set({0, 1}, {3000, 0}, 20);  // Equivalent form.
signal value = line;   // Evaluate once per sample.
if (line.updated) level = value;
```

The one-argument setter retains the mode. `duration(ms)` supplies the next target's
duration and is then consumed. `grain(ms)` persists; nonpositive grain means 20 ms.
`stop()` freezes internal ramp state; `reset(value)` implements control `set value`
without emitting. Pre-0.48 control stop is selected by `legacy`.

`updated`/`updates` distinguish emission from value change. This is a polled
interface: multiple sub-sample outputs expose their final held value and count,
not synchronous callbacks. Audio mode retains its existing block-64 contract;
this pass does not hide that limitation or redesign it.

## Validation and listening status

- Release build and isolated primitive comparisons pass; see
  [line tests](../tests/pd/line.md) and [primitive evidence](../tests/pd/README.md).
- Current model comparisons record raw PD residuals, peaks, DC and levels at
  48/44.1 kHz. [Before/after report](audio/comparisons/klang-topology-review.json)
  covers 168 cases: 96 are sample-identical to the pre-review executable, without
  alignment or gain fitting. Others include intended scheduling/routing changes.
- `--review` in the Artificial/Idiophonics runners collects differences while
  retaining `passed: false` for failures of the old parity limits. It cannot be
  combined with `--retain`. Normal runs still enforce the existing limits.
- Old retained WAVs and plots describe the previous implementation. Newly changed
  sounds require the planned one-by-one listening review; no new block adapter or
  blanket audible-equivalence claim is introduced here.
- Some diagnostic streams encode frequencies/counts or unit impulses. Values over
  unity in those streams are not playback clipping. Bulk Boing can exceed unity in
  both reference and current audio; gain remains the source's fixed gain.

Doxygen documentation can be generated with `doxygen tools/Doxyfile.pd`; the HTML
is written under `build/pd-docs/html/`. The C++ API tests cover both requested
brace forms and mode dispatch; model-control/Harrier smoke tests remain finite
and active at both rates. These are not a listening verdict or a gain-safety test.

Reproduce scratch comparisons with `render_artificial.py --review --rate RATE`,
`render_idiophonics.py --review --rate RATE`, and `render_farnell.py` with scratch
output/results paths. `tools/review_klang_topology.py` combines their results and
the saved pre-review executable under `build/topology-review/before/`.
