# Final UE 0.7.9 integration details

Integrated into `include/klang.h` on 17 September 2026. This closes the warning,
lookup-table, follower-expression and version decisions left after the initial
UE header review. The working Klang version is now 0.7.10 after the subsequent
oscillator, routing and generator-parameter promotions.

## Warning policy

`KLANG_STRICT` defaults to `0`. On MSVC this scopes and suppresses warnings
4587, 4263, 4264 and 4996 around Klang's header. Define it before inclusion to
restore those diagnostics:

```cpp
#define KLANG_STRICT 1
#include <klang.h>
```

The default header compiles with those four warnings individually enabled and
promoted to errors. The strict build emits C4263/C4264/C4587 as expected; the
same `/WX` command therefore fails, proving the warnings are reactivated rather
than globally disabled by the header.

## Callable tables

The malformed `FUNCTION` macro is removed. `Table<T,N>` accepts either a
value-returning callable or a callable writing through `Table<T,N>::Result`:

```cpp
Table<float, 128> sine = [](float x) { return sin(x * twoPi / 128); };
Table<int, 4> doubled = [](int x, Table<int, 4>::Result& y) { y = 2 * x; };
```

Capturing lambdas are supported. DX7 and FM use ordinary value-returning
lambdas and compile on MSVC and Studio Clang. Their MSVC renders remain
sample-identical at 44.1/48 kHz; Clang now compiles and renders both where the
retained pre-change header failed.

## Follower expression

The UE expression `sqrt((in * in) >> ar) >> out` and Klang's explicit flow
`(in * in) >> ar >> sqrt >> out` are bit-identical across 48,000 changing input
samples on both compilers. Klang retains the explicit flow because it exposes
the stateful attack/release stage and evaluation order. This is an RMS-like
path: `sqrt(in * in)` alone is `abs(in)`, but the intervening smoother means
`sqrt(AR(in * in))` is not equivalent to `AR(abs(in))`.

## Final validation

- All 7 retained sound roots and all 53 examples compile under MSVC and Studio
  Clang after the subsequent constrained generator-to-param integration.
- The full model runner reports no compile regressions.
- The language suite passes, including callable tables and 48,000 follower
  equivalence checks.
- The 1,105,295-check UE suite passes in Release and Debug on both compilers.
- `Sound` and `Stereo::Sound` remain Effects, preserving modulation, sidechain
  and reference-input use.

Evidence is under `build/ue-details-final/`.
