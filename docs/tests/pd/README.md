# Isolated PD primitive comparisons

The shared control/audio [line API](line.md) adds 20 control fixtures and C++ mode/initializer checks. [Current topology review](../../farnell/reviews/KLANG-TOPOLOGY-REVIEW.md) supersedes old model-routing parity claims below; primitive fixtures remain independent.

The complete [vline ramp/message port](vline.md) replaces Gesture. Its 50 isolated
cases cover the queue, interruptions, fractional timing, cold inlets and stop;
PD host clock synchronisation is explicit in the driver. The pure models remain
sample-timed. See also the [54 model comparisons](../../../tests/pd/vline-model-results.json).

Run `python tools/check_pd_primitives.py` after building Kleine Release and installing `tools/requirements.txt`. Override executables with `--pd` / `--kleine`, and rates with `--rates`. Reference patches are kept here; generated wrappers, impulses and WAVs go under `build/primitives/`. [results.json](../../../tests/pd/results.json) retains the measured evidence and source hashes.

Each patch exposes just the primitive and its required input/control. The FM patches deliberately combine two oscillators to test the effect of using one as a modulator.

The separate [metro port and contract](metro.md) adds 16 count/timing fixtures using `tools/check_pd_metro.py`. Its sample-timed polling API deliberately leaves PD block delivery separate; these tests do not claim raw `metro -> sig~` sample parity at 44.1 kHz.

The [del/delay port](del.md) adds 52 control-clock cases: bang, hot/cold floats,
retrigger, stop, tempo units and pending tempo changes. The DTMF dialler uses it
for its 200 ms release. [DTMF message tests](dtmf-messages.md) separately compare
the actual Sand abstraction and both decoder graphs, including overlaps and
held-key repetition; all 56 shared-input cases pass at 48/44.1 kHz.

| Patch | Klang implementation | Input / cases |
| --- | --- | --- |
| [osc.pd](../../../tests/pd/osc.pd) | `pd::osc` | 300/440 Hz; phase 0/0.25 cycles; negative 440 Hz |
| [osc-fm.pd](../../../tests/pd/osc-fm.pd) | `pd::osc` | 300 Hz modulator; carrier `1000 + 2000 * modulator` Hz, crossing negative frequencies |
| [hip.pd](../../../tests/pd/hip.pd) | `pd::hip` | Unit impulse; 0, 90 and 2000 Hz; modern and legacy 90 Hz normalisation |
| [noise.pd](../../../tests/pd/noise.pd) | `pd::noise` | Seed 404933; sample-for-sample comparison |
| [lop.pd](../../../tests/pd/lop.pd) | `pd::lop` | Unit impulse, 0.1/20/100 Hz |
| [bp.pd](../../../tests/pd/bp.pd) | `pd::bpf` (existing class name for `bp~`) | Unit impulse: 2000 Hz/Q12, 400 Hz/Q7, 359 Hz/Q123, 632 Hz/Q400 |
| [vline-decay.pd](../../../tests/pd/vline-decay.pd) | `TelephoneBell::Decay` | Immediate 1, followed by a linear 10 ms decay; this is not a complete `vline~` port |
| [vline.pd](../../../tests/pd/vline.pd) | `pd::vline` | Complete float/list, cold-inlet, queue and stop contract; [separate runner and results](vline.md) |
| [phasor.pd](../../../tests/pd/phasor.pd) | `pd::phasor` | Positive/negative 440 Hz, initial phase 0.25 cycles |
| [cos.pd](../../../tests/pd/cos.pd) | `pd::cos` | Table lookup over positive/negative cycle values |
| [wrap.pd](../../../tests/pd/wrap.pd) | `pd::wrap` | Finite negative, integer and positive inputs |
| [line.pd](../../../tests/pd/line.pd) | `pd::line` | Block-rounded ramps, retarget, stop and immediate values |
| [env.pd](../../../tests/pd/env.pd) | `pd::env` | Gated oscillator; default 1024-sample Hann window and 512-sample hop, including control-delivery timing |
| [vcf.pd](../../../tests/pd/vcf.pd) | `pd::vcf` | Both quadrature outputs, 997 Hz input, 13 Hz modulation driving frequency through -3000 to 15000 Hz; Q=80 and zero-Q bypass |
| [samphold.pd](../../../tests/pd/samphold.pd) | `pd::samphold` | Noise seed 404933, 137 Hz phasor trigger; capture on decreasing input |
| [rzero.pd](../../../tests/pd/rzero.pd) | `pd::rzero` | Noise seed 404933, coefficient 0.99 |

The [Klang test sound](../../../tests/pd/primitive.h) is reached through Kleine's `pd-<case>` render names, for example `--render pd-osc-fm build/fm.wav 1 48000`. The normal render path still uses the same Kleine `Processor` as the models. The FM case also exercises inline `osc(frequency)` dispatch with `param` arguments.

On 15 September 2026, all **58 cases passed**: 29 at 48 kHz and 29 at 44.1 kHz, with a 64-sample PD block. Oscillator, highpass, explicitly seeded noise, phasor, cosine lookup, wrapping and ramp cases have identical decoded float samples. The remaining residuals are below −102 dB in these tests. Recorded `−300 dB` is a reporting floor for exact zero residual; use `identical_samples` for that distinction. No level fitting or sample alignment is used. The regression limits are 0.002 dB level difference and residual below −70 dB; these are fixture limits, not universal perceptual acceptance criteria.

The version experiments run **PD 0.55.2 in compatibility modes 0.55 and 0.43**, not two different installed releases. They compare oscillator output, FM and highpass normalisation. [osc-fm-offset.pd](../../../tests/pd/osc-fm-offset.pd) subtracts the measured legacy modulator DC difference to test its contribution to FM phase drift. See the [level investigation](../../farnell/audio/comparisons/reference-levels.md) for interpretation.

`pd::osc` uses the 0.55-2 2048-point cosine table, float interpolation and IEEE-754 phase arithmetic with 64-sample phase maintenance. `set(hz)` preserves phase; `set(hz, cycles)` / `phase(cycles)` set PD-style phase in cycles. `pd::hip::legacy` opts into pre-0.44 normalisation, default false. `pd::noise::seed(uint32_t)` preserves all 32 seed bits; default construction follows PD's per-instance seed sequence. Filter ports flush denormals per sample rather than at the end of a PD block, so bit identity is not promised for every extreme input or state.

`pd::line` is explicitly restricted to block 64 and control changes at block boundaries. `pd::env` currently implements only the default 1024/512 configuration. Its fixture encodes PD's envelope control value as samples (100 dB denotes unity); this is diagnostic data, not a listening WAV or a dBFS signal. Neither port claims arbitrary PD block/window/hop support.

`pd::env` holds a completed calculation privately, then publishes `out` and
`updated` together on the next block's first sample. The public `level` member
remains removed. This restores its original output timing while keeping the
simplified Detector. The [focused checks](../../../tests/pd/env-output-results.json) compare raw
samples directly at 48/44.1 kHz (maximum error 0.0000076 dB), plus eight event
cases covering silence, constant input, gating/release and boundary impulses.
Event frames match exactly, including repeated equal outputs; `out` holds when
`updated` is false. There is no comparison offset or delivery projection.

After creating `build/env-publication`, compile the event driver in an MSVC shell:

```text
cl /nologo /std:c++17 /EHsc /O2 /fp:precise /DNOMINMAX /Iinclude tests/pd/env-publication.cpp /Febuild/env-publication/env.exe /Fobuild/env-publication/env.obj
python tools/check_pd_env.py
```

The earlier simplification exposed `out` one sample early and compensated in the
tests. That behaviour and the compensation have both been removed. PhoneEffects
and diagnostics now consume the publication immediately; no extra sample is held
in those callers. Decoder timestamps are logged at the actual sample, without +1.

[number-match.pd](../../../tests/pd/number-match.pd) is a separate reference helper for the [Artificial Sounds trial](../../farnell/audio/artificial-sounds.md), not a PD primitive. It reconstructs sequential equality and inactivity reset for the two supplied demos whose external `list-emath`/`list-dotprod` abstractions are missing. Its expected-number, wrong-number and timeout behaviour is covered by the model recipes; original upstream patches are preserved.

The [source manifest](../../../tests/pd/sources.json) identifies inspected upstream revisions. Port attribution is Miller Puckette and contributors under the retained [Pure Data BSD licence](../../licenses/Pure-Data-BSD.txt). The model-only decay helper remains nested in the bell. `pd.h` is now UTF-8; its previous Windows-1252 comments were converted without changing their wording.

The earlier [Idiophonics series](../../farnell/audio/idiophonics.md) added finite gesture, modal and delay/component fixtures in [idiophonics.h](../../../tests/pd/idiophonics.h), plus [rapid-control/Harrier smoke checks](../../../tests/pd/idiophonics-smoke.cpp). Its Creaking delay and panel parity describes the implementation before the topology review. Gesture has since been replaced by the complete `pd::vline` queue/message port; named-buffer delay helpers remain separate work.

`pd::vcf` now follows PD 0.55-2 table interpolation, real/imaginary state updates and Q-dependent gain. Existing `lpf`/`bpf` member aliases remain for source compatibility; they denote the real/imaginary outputs, not conventional lowpass/bandpass filter specifications. The previous polynomial approximation changed high-frequency behavior; existing Harrier timbre may therefore change. Tests establish the stated fixed-rate inputs, not every Q, reset or live rate-change case.
