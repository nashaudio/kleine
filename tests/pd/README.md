# Isolated PD primitive comparisons

Run `python tools/check_pd_primitives.py` after building Kleine Release and installing `tools/requirements.txt`. Override executables with `--pd` / `--kleine`, and rates with `--rates`. Reference patches are kept here; generated wrappers, impulses and WAVs go under `build/primitives/`. [results.json](results.json) retains the measured evidence and source hashes.

Each patch exposes just the primitive and its required input/control. The FM patches deliberately combine two oscillators to test the effect of using one as a modulator.

| Patch | Klang implementation | Input / cases |
| --- | --- | --- |
| [osc.pd](osc.pd) | `pd::osc` | 300/440 Hz; phase 0/0.25 cycles; negative 440 Hz |
| [osc-fm.pd](osc-fm.pd) | `pd::osc` | 300 Hz modulator; carrier `1000 + 2000 * modulator` Hz, crossing negative frequencies |
| [hip.pd](hip.pd) | `pd::hip` | Unit impulse; 0, 90 and 2000 Hz; modern and legacy 90 Hz normalisation |
| [noise.pd](noise.pd) | `pd::noise` | Seed 404933; sample-for-sample comparison |
| [lop.pd](lop.pd) | `pd::lop` | Unit impulse, 100 Hz |
| [bp.pd](bp.pd) | `pd::bpf` (existing class name for `bp~`) | Unit impulse, 2000 Hz/Q12 and 400 Hz/Q7 |
| [vline-decay.pd](vline-decay.pd) | `TelephoneBell::Decay` | Immediate 1, followed by a linear 10 ms decay; this is not a complete `vline~` port |
| [phasor.pd](phasor.pd) | `pd::phasor` | Positive/negative 440 Hz, initial phase 0.25 cycles |
| [cos.pd](cos.pd) | `pd::cos` | Table lookup over positive/negative cycle values |
| [wrap.pd](wrap.pd) | `pd::wrap` | Finite negative, integer and positive inputs |
| [line.pd](line.pd) | `pd::line` | Block-rounded ramps, retarget, stop and immediate values |
| [env.pd](env.pd) | `pd::env` | Gated oscillator; default 1024-sample Hann window and 512-sample hop, including control-delivery timing |

The [Klang test sound](primitive.h) is reached through Kleine's `pd-<case>` render names, for example `--render pd-osc-fm build/fm.wav 1 48000`. The normal render path still uses the same Kleine `Processor` as the models. The FM case also exercises inline `osc(frequency)` dispatch with `param` arguments.

On 15 September 2026, all **40 cases passed**: 20 at 48 kHz and 20 at 44.1 kHz, with a 64-sample PD block. Oscillator, highpass, explicitly seeded noise, phasor, cosine lookup, wrapping and ramp cases have identical decoded float samples. The remaining residuals are below −102 dB in these tests. Recorded `−300 dB` is a reporting floor for exact zero residual; use `identical_samples` for that distinction. No level fitting or sample alignment is used. The regression limits are 0.002 dB level difference and residual below −70 dB; these are fixture limits, not universal perceptual acceptance criteria.

The version experiments run **PD 0.55.2 in compatibility modes 0.55 and 0.43**, not two different installed releases. They compare oscillator output, FM and highpass normalisation. [osc-fm-offset.pd](osc-fm-offset.pd) subtracts the measured legacy modulator DC difference to test its contribution to FM phase drift. See the [level investigation](../../farnell/audio/comparisons/reference-levels.md) for interpretation.

`pd::osc` uses the 0.55-2 2048-point cosine table, float interpolation and IEEE-754 phase arithmetic with 64-sample phase maintenance. `set(hz)` preserves phase; `set(hz, cycles)` / `phase(cycles)` set PD-style phase in cycles. `pd::hip::legacy` opts into pre-0.44 normalisation, default false. `pd::noise::seed(uint32_t)` preserves all 32 seed bits; default construction follows PD's per-instance seed sequence. Filter ports flush denormals per sample rather than at the end of a PD block, so bit identity is not promised for every extreme input or state.

`pd::line` is explicitly restricted to block 64 and control changes at block boundaries. `pd::env` currently implements only the default 1024/512 configuration. Its fixture encodes PD's envelope control value as samples (100 dB denotes unity); this is diagnostic data, not a listening WAV or a dBFS signal. Its largest absolute difference is about 0.0000076 dB. Neither port claims arbitrary PD block/window/hop support.

[number-match.pd](number-match.pd) is a separate reference helper for the [Artificial Sounds trial](../../farnell/audio/artificial-sounds.md), not a PD primitive. It reconstructs sequential equality and inactivity reset for the two supplied demos whose external `list-emath`/`list-dotprod` abstractions are missing. Its expected-number, wrong-number and timeout behaviour is covered by the model recipes; original upstream patches are preserved.

The [source manifest](sources.json) identifies inspected upstream revisions. Port attribution is Miller Puckette and contributors under the retained [Pure Data BSD licence](../../licenses/Pure-Data-BSD.txt). The model-only decay helper remains nested in the bell. `pd.h` is now UTF-8; its previous Windows-1252 comments were converted without changing their wording.
