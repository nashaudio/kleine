# Development alternatives

Alternate implementations of the same sound live in separate `variants/` files.
Unreviewed models retain their defaults. We choose one implementation after listening;
the filenames describe their DSP differences, not their distribution history.
Distinct-sound selectors (PhoneTones, AlarmGenerator studies, Rolling/struck can and
the combined/standalone PhoneEffects setups) remain for now.

Include a variant file directly and instantiate its class in `farnell::variants`.
The comparison renderer includes [variants.h](../../../farnell/variants.h) and retains the old
listening labels. Main models do not include that development collection.

**Phone Tones selection, 16 September:** the main model now contains the selected
350/450 Hz dial and online smoothed busy; the brighter ringback remains. Its three
development variant files are removed. The old 350/440 dial, unsmoothed busy and
rejected duller ringback survive only as [historical test fixtures](../../../tests/pd/phone-tones-archive.h),
so previous render commands and evidence remain reproducible. These are not open
listening choices. Original retained WAVs keep their historical labels/content.

| Render label | Implementation | Reference / difference |
| --- | --- | --- |
| `dial` | Test-only `validation::Dial440` | Historical fixture label: 350/440 Hz; not the default model. |
| `dial-web` | `PhoneTones(Dial)` | Selected website dialtone: 350/450 Hz. |
| `busy-archive` | Test-only `validation::BusyUnsmoothed` | Historical unsmoothed gate; perceptually accepted. |
| `busy` | `PhoneTones(Busy)` | Selected website busy: 100 Hz lowpass on gate. |
| `ringback` | `PhoneTones(Ringback)` | PHONETONES ringingtone. |
| `ringback-bulk` | Test-only `validation::RingbackHandset` | Rejected ALARMS handset; historical comparisons only. |
| `dtmf` | `DTMFTones` | Website dtmf, highpass and gain 0.25. |
| `dtmf-bulk` | Test-only [validation::DTMFLevel03](../../../tests/pd/dtmf-archive.h) | Discarded as perceptually equivalent; gain 0.3. Historical comparisons only. |
| `dtmf-study` | [DTMFUnfiltered](../../../farnell/klang/Artificial%20Sounds/DTMF%20Tones/variants/dtmf-unfiltered.k) | ALARMS dtmf00, no highpass, gain 0.125. |
| `police` | `Police` | Current website pow~ convention. |
| `police-legacy` | [PoliceExponential](../../../farnell/klang/Artificial%20Sounds/Police/variants/police-exponential.k) | Historical reversed pow~ operands. |
| `police-*` components | [Oscillator studies](../../../farnell/klang/Artificial%20Sounds/Police/variants/oscillators.k) | Explicit power/exponential/triangle flows; component selection is in the renderer. |
| `bouncing` | `Bouncing` | Website fourth-power pitch shape. |
| `bouncing-bulk` | [BouncingExponential](../../../farnell/klang/Idiophonics/Bouncing/variants/bouncing-exponential.k) | BOUNCINGBALL bb1, modern operands: exp(envelope). |
| `bouncing-legacy` | [BouncingPower](../../../farnell/klang/Idiophonics/Bouncing/variants/bouncing-power.k) | bb1 reversed operands: envelope raised to e; negative-base guard matches PD. |
| `boing` | `Boing` | Website env16p actually raises to 64. |
| `boing-bulk` | [BoingExponential](../../../farnell/klang/Idiophonics/Boing/variants/boing-exponential.k) | MRBOINGY twang, modern operands: 15 raised to envelope. |
| `boing-legacy` | [BoingPower](../../../farnell/klang/Idiophonics/Boing/variants/boing-power.k) | MRBOINGY twang, reversed operands: envelope raised to 15. |

DTMF alternatives now use their own render names. The old `dtmf` command plus a
source-selection TSV event must become `dtmf-bulk` or `dtmf-study`. Existing TSV
configuration rows remain valid as metadata checks against the selected model;
they cannot switch its implementation while rendering. `render_artificial.py`
has been updated accordingly.

## Verification

The Release build and idiophonics control/retrigger smoke checks pass. The
[raw comparison report](../../../farnell/audio/comparisons/model-variants-review.json) compares
44 cases at both 48 and 44.1 kHz against the executable saved before this cleanup:
all 88 are sample-for-sample identical. Cases include all extracted alternatives,
oscillator components, control changes, retriggers and combined phone demos.
No gain matching or alignment is applied; retained listening WAVs are unchanged.
This establishes preservation of the previous behavior, not new PD fidelity or
listening acceptance. The separate 64-sample scheduling review remains pending.

Reproduce with a saved pre-change executable:

```text
python tools/review_model_variants.py --before build/variant-review/before.exe
```
