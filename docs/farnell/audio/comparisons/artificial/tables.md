# Artificial Sounds measurements

> These retained measurements describe the earlier implementation. See the [topology review](../../../reviews/KLANG-TOPOLOGY-REVIEW.md) for current raw differences and pending listening review.

PD 0.55.2, block 64; raw residuals have no alignment or gain fitting. Component waveforms and encoded detector states are diagnostics, not listening fixtures. Both sample rates construct fresh models; live sample-rate changes are not established. Pedestrians is intentionally sample-timed: its 44.1 kHz residual includes bounded gate-edge shifts; see [metro contract](../../../../tests/pd/metro.md).

| Case | 48 kHz residual (dB) | 44.1 kHz residual (dB) | 48 kHz level delta (dB) |
| --- | ---: | ---: | ---: |
| pedestrians | identical | -18.43 | 0.000000 |
| dtmf | -300.00 | -300.00 | 0.000000 |
| alarm01 | identical | identical | 0.000000 |
| alarm02 | -300.00 | -300.00 | 0.000000 |
| alarm03 | identical | identical | 0.000000 |
| alarm04 | identical | identical | 0.000000 |
| alarm05 | identical | identical | 0.000000 |
| alarm06 | -300.00 | -300.00 | 0.000000 |
| alarm07 | -300.00 | -300.00 | 0.000000 |
| police | -125.50 | -125.99 | -0.000000 |
| police-legacy | -120.87 | -122.03 | -0.000000 |
| police-environment | -300.00 | -300.00 | 0.000000 |
| police-horn | identical | identical | 0.000000 |
| police-logosc | -184.76 | -176.61 | 0.000000 |
| police-logosc-legacy | -169.86 | -170.39 | -0.000000 |
| police-graph | -176.64 | -179.28 | 0.000000 |
| police-graph-legacy | -179.45 | -170.42 | -0.000000 |
| police-triangle | identical | identical | 0.000000 |
| dtmf-bulk | -300.00 | -300.00 | 0.000000 |
| dtmf-study | identical | identical | 0.000000 |
| alarm15 | -300.00 | -300.00 | 0.000000 |
| alarm-bank | -300.00 | -300.00 | 0.000000 |
| ringback-bulk | -300.00 | -300.00 | 0.000000 |
| police00 | -179.45 | -170.42 | -0.000000 |
| police01 | -139.74 | -138.71 | -0.000000 |
| police1 | -171.66 | -171.06 | -0.000000 |
| police3 | -120.87 | -122.03 | -0.000000 |
| police-osc-bulk | -169.86 | -170.39 | -0.000000 |
| police-compare-log | -174.87 | -170.25 | 0.000000 |
| dtmf-detector | identical | identical | 0.000000 |
| dtmf-decoder | identical | identical | 0.000000 |
| phone-effects | -162.22 | -162.36 | -0.000000 |
| call-recogniser | -300.00 | -300.00 | 0.000000 |
| phone-match | -163.66 | -163.47 | -0.000000 |
| call-match | -300.00 | -300.00 | 0.000000 |
| pedestrians-controls | identical | -18.92 | 0.000000 |
| alarm01-controls | identical | identical | 0.000000 |
| alarm05-controls | identical | identical | 0.000000 |
| dtmf-retrigger | -300.00 | -300.00 | 0.000000 |

Process CPU/RSS include startup and I/O and are sampled lower bounds. Kleine DSP wall time covers Processor::process, including buffer preparation. These single runs do not establish relative PD/Klang DSP efficiency.

| Case (48 kHz) | PD / Kleine CPU (ms/audio s) | PD / Kleine peak RSS (MiB) | Kleine DSP wall (ms/audio s) |
| --- | ---: | ---: | ---: |
| pedestrians | 105.47 / 0.00 | 20.82 / 5.03 | 0.451 |
| dtmf | 28.65 / 0.00 | 21.03 / 5.26 | 0.809 |
| alarm01 | 46.88 / 0.00 | 19.96 / 4.84 | 0.515 |
| alarm02 | 74.22 / 0.00 | 20.18 / 4.82 | 0.928 |
| alarm03 | 50.78 / 0.00 | 20.17 / 4.84 | 1.030 |
| alarm04 | 50.78 / 3.91 | 20.15 / 4.88 | 2.024 |
| alarm05 | 42.97 / 0.00 | 20.22 / 4.83 | 1.274 |
| alarm06 | 50.78 / 7.81 | 20.24 / 5.09 | 4.726 |
| alarm07 | 16.93 / 5.21 | 23.15 / 6.38 | 4.704 |
| police | 22.46 / 4.88 | 25.21 / 7.18 | 4.157 |
| police-legacy | 23.44 / 2.93 | 25.20 / 7.18 | 3.121 |
