# Website levels and PD compatibility

Rechecked 16 September 2026 against the current Release build and the latest local website patch. **PD and Kleine agree in pulse level. Both are quieter than the online example. The tested PD-version changes do not explain the website's roughly 6 dB increase.**

| Comparison at 48 kHz | Measured level difference |
| --- | ---: |
| Kleine pulse versus current PD pulse | 0 dB; negligible filter-tail differences |
| Latest website pulse patch versus original collection patch | Sample-identical |
| Website pulse versus current PD, matched individual contacts | +5.902 dB (about 1.973× amplitude) |
| Legacy PD compatibility versus current PD, full pulse patch | +0.102632 dB |
| Legacy versus current `osc~ 440` | −0.0000919 dB |
| Legacy versus current FM test, overall RMS | −0.0001844 dB |

The website comparison uses 26 matching 100 ms pulse periods from the existing unprocessed excerpt. Cross-correlation finds integer-sample alignment, then a least-squares scalar fits the gain. Fitted gains span 1.972923–1.973010, with residuals about −54 dB after gain fitting. This is strong evidence that essentially the same pulse shape appears at a different level. The [raw results](../../../../farnell/audio/comparisons/reference-levels.json) retain exact source times, fits and residuals. The retained listening files remain unnormalised.

The pulse patch contains no oscillator. It drives a clipped contact signal through two `bp~` filters and two `hip~ 90` filters. Comparing the [0.42-5 filter source](https://github.com/pure-data/pure-data/blob/0.42-5/src/d_filter.c) with [0.55-2](https://github.com/pure-data/pure-data/blob/0.55-2/src/d_filter.c) identifies a relevant highpass change: modern `hip~` multiplies by `0.5 * (1 + coefficient)`, while the older routine does not. For two 90 Hz filters at 48 kHz this predicts the measured +0.1026 dB legacy difference. The bandpass gain formula is the same in these inspected releases. These findings do not establish the gain of every historical fork/build.

## Oscillators and FM

The [0.42-5 oscillator source](https://github.com/pure-data/pure-data/blob/0.42-5/src/d_osc.c) uses a 512-point table constructed with accumulated single-precision phase. [0.55-2](https://github.com/pure-data/pure-data/blob/0.55-2/src/d_osc.c) defaults to 2048 points, calculates each phase directly and fixes the quarter-cycle entries; it retains the old table/routines for compatibility. Both tables have nominal unit amplitude. There is no factor-of-two output-level change in the tested fixed-frequency cases.

There **is** an FM consequence. In the 48 kHz test, the old 300 Hz modulator's mean differs by approximately `2.653e-6`. Multiplying it by the 2000 Hz modulation depth introduces approximately 0.00531 Hz of carrier-frequency bias. Over one second this predicts about 0.0333 radians of phase drift, consistent with the observed maximum waveform difference of 0.03336 despite virtually unchanged RMS level. Subtracting that measured DC difference in an otherwise identical legacy FM patch reduces the relative residual from **−34.32 dB to −75.40 dB**. This experiment supports DC bias as the dominant cause here; smaller table/interpolation differences remain. The 44.1 kHz experiment gives the same qualitative result.

The new `pd::osc` implementation matches the current PD oscillator's decoded samples in the tested fixed-frequency, phase, negative-frequency and FM cases at both sample rates. Those isolated fixtures are in [tests/pd](../../../tests/pd/README.md); their [results](../../../../tests/pd/results.json) also contain the compatibility and DC-removal experiments. Compatibility 0.43 is an option of the installed **0.55.2 executable**, not a claim that a historical 0.43 binary was run.

## What remains unresolved

The pulse recording's near-constant extra gain is consistent with a different output/recording gain or a gain stage not present in the supplied patch. It does **not** prove that Andy post-processed it. The online bell is also louder, but the measured pulse ratio should not be generalised to every recording. Establishing the exact capture path or original rendering session would distinguish those explanations; no compensating gain has been silently added to the models.

Reproduce the investigation after building Release:

```text
python tools/check_pd_primitives.py
python tools/render_farnell.py
python tools/check_reference_levels.py
python tools/package_farnell.py
```

All scripts accept `--pd`; render/primitive scripts also accept `--kleine`. Original downloaded source files stay under `build/trials/pd-history`; their upstream URLs, tags and SHA-256 hashes are retained in [sources.json](../../../../tests/pd/sources.json).
