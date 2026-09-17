# Idiophonics measurements

> These retained measurements describe the earlier implementation. See the [topology review](../../../reviews/KLANG-TOPOLOGY-REVIEW.md) for current raw differences and pending listening review.

PD 0.55.2, compatibility 0.55, block 64; one voice; raw unity-gain bounces. No time alignment or fitted gain in these residuals. See the [series guide](../../idiophonics.md) for control recipes, source differences and acceptance scope.

| Case | 48 kHz residual (dB) | 44.1 kHz residual (dB) | Worst level delta (dB) | Worst 10 ms envelope error |
| --- | ---: | ---: | ---: | ---: |
| bouncing | -87.58 | -86.90 | 0.000003 | 0.001% |
| bouncing-bulk | -86.73 | -80.52 | 0.000001 | 0.001% |
| bouncing-legacy | -88.35 | -85.44 | 0.000002 | 0.001% |
| rolling | -46.32 | -48.72 | 0.000248 | 0.383% |
| rolling-bulk | -46.32 | -48.72 | 0.000248 | 0.383% |
| tincan | -88.59 | -84.87 | 0.000230 | 0.004% |
| uneven2 | -108.04 | -110.92 | 0.000000 | 0.000% |
| uneven2-bulk | -108.04 | -110.92 | 0.000000 | 0.000% |
| creaking | -38.24 | -67.07 | 0.006092 | 1.057% |
| creaking-controls | -71.90 | -41.65 | 0.001795 | 0.637% |
| boing | -93.20 | -88.39 | 0.000012 | 0.000% |
| boing-bulk | -89.01 | -84.48 | 0.000016 | 0.000% |
| boing-legacy | -101.80 | -97.76 | 0.000005 | 0.000% |
| boing-phase | -117.64 | -112.71 | 0.000014 | 0.000% |
| boing-pitch | -119.24 | -116.96 | 0.000004 | 0.000% |
| boing-clamped | -94.83 | -89.96 | 0.000013 | 0.000% |
| boing-free | -95.15 | -90.47 | 0.000003 | 0.001% |
| boing-frequency | -145.63 | -143.84 | 0.000000 | 0.000% |
| bouncing-height | identical | identical | 0.000000 | 0.000% |
| bouncing-envelope | -82.07 | -77.37 | 0.000598 | 0.012% |
| bell-a0 | identical | identical | 0.000000 | 0.000% |
| bell-a1 | -142.44 | -140.00 | 0.000001 | 0.000% |
| bell-a2 | -140.09 | -138.57 | 0.000001 | 0.000% |
| bell-a3 | -119.75 | -115.10 | 0.000011 | 0.000% |
| bell-a4 | -128.75 | -71.71 | 0.000019 | 0.003% |
| bell-partial | -142.18 | -139.73 | 0.000001 | 0.000% |
| bell-group | -139.89 | -137.46 | 0.000001 | 0.000% |
| bell-testgroup | -139.89 | -138.70 | 0.000000 | 0.000% |
| uneven | -115.19 | -110.50 | 0.000001 | 0.000% |
| creaking-pulse | -72.05 | -31.70 | 0.001757 | 0.695% |
| creaking-wood | -144.35 | -145.58 | 0.000000 | 0.000% |
| creaking-panel | -300.00 | -300.00 | 0.000000 | 0.000% |
| creaking-delay | -300.00 | -300.00 | 0.000000 | 0.000% |

## Process measurements

CPU and RSS are sampled process totals (lower bounds), including startup and file I/O. The DSP column measures Kleine Processor calls, including buffer preparation. Concurrent trial processes can affect wall time. These are context, not a comparative DSP benchmark.

| Case (48 kHz) | PD / Kleine CPU (ms/audio s) | PD / Kleine elapsed (s) | PD / Kleine RSS (MiB) | Kleine DSP wall (ms/audio s) |
| --- | ---: | ---: | ---: | ---: |
| bouncing | 20.83 / 1.74 | 0.386 / 0.031 | 18.82 / 5.85 | 0.817 |
| rolling | 29.30 / 9.77 | 0.336 / 0.064 | 21.74 / 5.63 | 4.375 |
| tincan | 42.97 / 3.91 | 0.323 / 0.030 | 20.39 / 5.02 | 1.887 |
| uneven2 | 27.34 / 1.95 | 0.329 / 0.038 | 21.64 / 5.55 | 2.546 |
| creaking | 31.25 / 4.46 | 0.338 / 0.048 | 21.81 / 5.48 | 4.582 |
| boing | 21.88 / 7.81 | 0.354 / 0.093 | 22.59 / 6.03 | 7.277 |
| boing-bulk | 25.00 / 6.25 | 0.388 / 0.081 | 22.45 / 6.02 | 6.265 |
| boing-legacy | 23.44 / 7.81 | 0.373 / 0.092 | 22.42 / 6.02 | 7.339 |
