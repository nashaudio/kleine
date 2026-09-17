# Basic and optimised phase hardening

Integrated into `include/klang.h` on 17 September 2026. The Basic oscillators
remain direct, un-band-limited waveform definitions intended for control use,
teaching and readable reference implementations.

`klang::Phase` now wraps negative and multi-cycle radian or wavetable steps.
Its `wrapped(offset)` helper applies relative phase consistently in Basic Sine,
Saw/Phasor, Triangle, Square and Pulse. The ordinary in-range path performs two
comparisons; `fmod` is used only when wrapping is required.

`optimised::Phasor` is independent of `basic::Phasor`. It reuses the existing
`Generators::Fast::Phase` and signed `Fast::Increment`, so its sample path is an
integer phase addition and conversion to `[0,1)`. It intentionally remains an
aliased control ramp rather than using the band-limited OSM.

## Validation

- `tests/klang/basic-oscillators.cpp`: 24 checks under MSVC and Studio Clang,
  covering negative frequency, positive/negative multi-turn increments,
  absolute and relative phase, reset, arbitrary cycle sizes, every Basic
  waveform and the dedicated optimised Phasor.
- The UE integration test uses an independent double-precision phase oracle.
  All 1,105,295 checks pass in MSVC/Clang Release and Debug.
- The four examples importing `klang::basic` remain sample-identical at
  44.1/48 kHz under both compilers.
- Bicycle and ToyBoatEngine remain sample-identical. Car/FourStrokeEngine were
  subsequently diagnosed as a legacy model defect unrelated to Phasor: their
  delay taps jump randomly at audio rate. They are retained for reference but
  excluded from the production acceptance corpus; see the Motors review.

Release `/fp:fast` microbenchmark, 50 million 440 Hz samples at 48 kHz:

| Compiler | Basic | Optimised |
| --- | ---: | ---: |
| MSVC 19.51 | 2.99 ns/sample | 1.26 ns/sample |
| Studio Clang 14 | 1.19 ns/sample | 0.33 ns/sample |

These are focused same-process timings, not whole-model CPU measurements. Build
the retained [benchmark](../../../../experiments/klang/basic-phase/benchmark.cpp) with optimisation and fast floating
point; generated executables belong under `build/basic-phase-final/`.
