# Scheduling measurements

Intel(R) Core(TM) i7-10700 CPU @ 2.90GHz; MSVC 19.51.36257.0; Release `/O2 /fp:precise`; one voice at 48 kHz.

Median of seven repeats, 4,194,304 samples per repeat, 64-sample host buffers. PD oscillator workload. These are processing wall times, excluding startup and I/O.

| Dispatch / timers | ns per sample | Relative to original |
| --- | ---: | ---: |
| original | 6.606 | 1.000 |
| fixed_branch_idle | 6.561 | 0.993 |
| reserved_branch_idle | 6.616 | 1.002 |
| fixed_pointer_idle | 6.674 | 1.010 |
| reserved_pointer_idle | 6.654 | 1.007 |
| fixed_branch_one | 7.601 | 1.151 |
| fixed_pointer_one | 7.626 | 1.154 |
| reserved_branch_one | 7.666 | 1.160 |
| fixed_branch_eight | 9.741 | 1.475 |
| fixed_pointer_eight | 9.694 | 1.467 |
| fixed_branch_changing | 7.702 | 1.166 |

## Sparse coefficient example

| Method | ns per sample | Relative to manual counter |
| --- | ---: | ---: |
| configure_sample | 8.551 | 2.239 |
| configure_counter | 3.819 | 1.000 |
| configure_timer | 3.530 | 0.924 |

Full raw repetitions, 32/64/256/1024 buffer sizes, both DSP workloads, object sizes, aggregate CPU time and source hashes are in [results.json](../../../../experiments/klang/scheduling/results.json). Small differences include code-layout and measurement effects; these results do not establish zero overhead.
