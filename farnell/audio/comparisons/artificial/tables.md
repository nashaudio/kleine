# Artificial Sounds measurements

PD 0.55.2, block 64; raw residuals have no alignment or gain fitting. Component waveforms and encoded detector states are diagnostics, not listening fixtures. Both sample rates construct fresh models; live sample-rate changes are not established.

| Case | 48 kHz residual (dB) | 44.1 kHz residual (dB) | 48 kHz level delta (dB) |
| --- | ---: | ---: | ---: |
| pedestrians | identical | identical | 0.000000 |
| dtmf | -300.00 | -300.00 | 0.000000 |
| alarm01 | identical | identical | 0.000000 |
| alarm02 | -140.39 | -141.25 | 0.000000 |
| alarm03 | -139.09 | -139.60 | 0.000000 |
| alarm04 | identical | identical | 0.000000 |
| alarm05 | -143.76 | -144.66 | 0.000000 |
| alarm06 | -300.00 | -300.00 | 0.000000 |
| alarm07 | -300.00 | -300.00 | 0.000000 |
| police | -113.91 | -116.05 | -0.000013 |
| police-legacy | -109.91 | -111.72 | -0.000015 |
| police-environment | -300.00 | -300.00 | 0.000000 |
| police-horn | -112.11 | -113.93 | -0.000013 |
| police-logosc | -184.76 | -176.61 | 0.000000 |
| police-logosc-legacy | -169.86 | -170.39 | -0.000000 |
| police-graph | -176.64 | -179.28 | 0.000000 |
| police-graph-legacy | -179.45 | -170.42 | -0.000000 |
| police-triangle | identical | identical | 0.000000 |
| dtmf-bulk | -300.00 | -300.00 | 0.000000 |
| dtmf-study | identical | identical | 0.000000 |
| alarm15 | -300.00 | -300.00 | 0.000000 |
| alarm-bank | -138.85 | -138.64 | 0.000000 |
| ringback-bulk | -101.65 | -101.79 | 0.000012 |
| police00 | -179.45 | -170.42 | -0.000000 |
| police01 | -139.74 | -138.71 | -0.000000 |
| police1 | -171.66 | -171.06 | -0.000000 |
| police3 | -109.91 | -111.72 | -0.000015 |
| police-osc-bulk | -169.86 | -170.39 | -0.000000 |
| police-compare-log | -174.87 | -170.25 | 0.000000 |
| dtmf-detector | identical | identical | 0.000000 |
| dtmf-decoder | identical | identical | 0.000000 |
| phone-effects | -101.96 | -103.01 | 0.000012 |
| call-recogniser | -300.00 | -300.00 | 0.000000 |
| phone-match | -102.35 | -103.26 | 0.000011 |
| call-match | -300.00 | -300.00 | 0.000000 |
| pedestrians-controls | identical | identical | 0.000000 |
| alarm01-controls | identical | identical | 0.000000 |
| alarm05-controls | -143.80 | -144.67 | 0.000000 |
| dtmf-retrigger | -300.00 | -300.00 | 0.000000 |

Process CPU/RSS include startup and I/O and are sampled lower bounds. Kleine DSP wall time covers Processor::process, including buffer preparation. These single runs do not establish relative PD/Klang DSP efficiency.

| Case (48 kHz) | PD / Kleine CPU (ms/audio s) | PD / Kleine peak RSS (MiB) | Kleine DSP wall (ms/audio s) |
| --- | ---: | ---: | ---: |
| pedestrians | 152.34 / 3.91 | 24.98 / 4.94 | 0.377 |
| dtmf | 109.38 / 0.00 | 25.02 / 5.24 | 0.838 |
| alarm01 | 171.88 / 0.00 | 24.95 / 4.95 | 0.465 |
| alarm02 | 152.34 / 0.00 | 25.02 / 4.80 | 0.910 |
| alarm03 | 160.16 / 0.00 | 24.94 / 4.80 | 1.067 |
| alarm04 | 156.25 / 11.72 | 25.02 / 4.80 | 2.199 |
| alarm05 | 140.62 / 3.91 | 24.91 / 5.18 | 1.519 |
| alarm06 | 148.44 / 3.91 | 25.05 / 4.87 | 5.140 |
| alarm07 | 49.48 / 5.21 | 27.29 / 6.37 | 5.145 |
| police | 47.85 / 4.88 | 29.34 / 7.15 | 4.618 |
| police-legacy | 50.78 / 3.91 | 29.24 / 7.15 | 3.442 |
