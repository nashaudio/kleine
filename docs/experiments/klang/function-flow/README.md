# Stateless function flow and math-name cleanup

Promoted to `include/klang.h` on 17 September 2026; version remains 0.7.8. The
experiment is retained from the accepted pre-promotion header and investigates
automatic promotion of an ordinary
`float function(float)` into sample-by-sample Klang signal flow, while removing
the public `abs`/`sqrt` macros and the non-portable `std::klang` workaround.

The initial contract is deliberately narrow: a `signal` is copied into an exact
`float(float)` function reference and the result is returned as a `signal`.
Conversion to float occurs inside the overload, not during overload selection.
This supports both a named teaching function and an overloaded standard name:

```cpp
in * gain >> hardclip >> out;
in >> std::abs >> out;
in >> std::sqrt >> out;
```

The exact function type supplies context for overloaded standard names. The
left operand remains a Klang `signal`: C++ cannot overload `double >> function`
because neither operand is a class or enumeration. `sqrt(2) >> follower` still
works because the follower is a Klang object. The one core RMS chain whose left
operand is a runtime double now states its existing narrowing explicitly as
`signal(sum * window.inv) >> sqrt`.

Common unary standard maths functions are stateless value functions in
production. Their standard overload sets (`abs`, roots, trigonometric and
hyperbolic functions, exponentials/logarithms, rounding, and error functions)
are imported into `klang` legally, with specific overloads for `signal`,
`Control`, and `ControlMap`. `sqr` and `cube` are also stateless. A pre-existing
function-like `abs`, `sqrt`, `min`, `max`, or `clamp` macro is permanently
undefined before any standard header is included. The old namespace additions
under reserved `std` and the public token-replacement macros are gone.

The pre-existing approximate sine helpers are independently organised as
`fast::sin` and `fast::sinp`; `fast::cos` and `fast::cosp` reuse the same
polynomial with direct quadrant reduction. See the focused [fast maths review](FAST-MATH.md)
for measured error and sourced—but deliberately unimplemented—tan, arc and
hyperbolic candidates.

The existing two-argument `min`/`max` templates retain their first-argument
result-type convention. Changing that convention materially changes existing
models (notably Harrier) and remains a separate arithmetic decision. Non-unary
functions remain ordinary calls, for example `max(in, floor) >> out`; an already
evaluated `max(0, 5)` is a scalar value rather than a function-flow stage.

Arbitrary callable promotion is deliberately excluded. `signal`'s float
conversion and Klang objects' callable setters make an `is_invocable`-style
overload too permissive. Capturing lambdas, `std::function`, stateful transforms,
and functions with configured arguments continue to use an explicit Klang
`Function` or a future tagged adapter. `Generic::Function` itself is unchanged.

## Results

17 September 2026. Candidate SHA-256:
`c197d69f86574a6785fbd6713406ab074d3ab89a55c3094300fb22fd1172797e`.
The promoted LF working header is
`3a846181b171bf5ce54a5daf2046d4c98c548322683d1fada1d63b84dbff3264`;
its source is equivalent to the retained CRLF candidate.

The corpus reports below select the immediately preceding candidate
`0aa8a10f...`. They were not rerun after the `fastsin` namespace-only change,
as requested. Focused MSVC/Clang tests prove that the relocated radian and phase
sine functions are bit-identical under precise and fast floating-point modes;
the added cosine functions have no existing callers.

Post-promotion focused function-flow, fast-trig and 227-check arithmetic
contracts pass on MSVC and Studio Clang. A final Harrier/Mini smoke comparison
also passes: Harrier remains sample-identical and Mini compiles raw and renders
finite/repeatable at both rates. Testing stopped there by request.

- The focused contracts pass in Release and Debug with MSVC 19.51 and Studio
  Clang 14. They cover hostile incoming macros, include order, `using namespace
  std`, direct and flowed `abs`/`sqrt`, an ordinary hardclip function, controls,
  the former `1 - abs(...)` ambiguity, and graph routing.
- The existing arithmetic suite passes all 227 runtime checks under both
  compilers with precise floating point. The former Function-left-scalar probe
  now compiles. MSVC `/fp:fast` still fails the existing 2^24 narrowing check in
  both the accepted and candidate headers; that is not a candidate regression.
- The 80-file compatibility matrix has zero compile regressions; Mini is the
  one deliberately edited model. Baseline results are 72/80 on MSVC and 70/80
  on Clang; the candidate improves these to 73/80 and 71/80 by combining that
  source cleanup with the stateless maths overloads.
  Helicopter still needs its documented host dependency.
- MSVC `/fp:fast` produces twenty sample-identical paired renders at 44.1/48
  kHz across the function-flow and maths users selected for this review. Mini
  and Helicopter add four finite, deterministic unpaired candidate renders.
- Clang precise produces six sample-identical pairs for IIR, Vocoder, and
  Harrier. Clang `-ffast-math` leaves Harrier and Compressor identical; removing
  the old `std::function` optimisation barriers exposes a one-subnormal IIR
  difference and Vocoder residual RMS of about 2.6e-5/2.8e-5. These disappear
  under precise floating point.

Retained evidence is indexed in [validation.json](../../../../experiments/klang/function-flow/validation.json). Inspection
WAVs and plots are under `build/function-flow/models-msvc-math-final/inspection/`
and `build/function-flow/models-clang14-math-final/inspection/`.

## Reproduce

```powershell
python experiments/klang/function-flow/prepare.py
```

Compile `probe.cpp` with the experiment directory before `include/`. Reuse the
existing model runner for comparisons, for example:

```powershell
python tests/klang/check_sounds.py --fast --examples `
  --baseline-header experiments/klang/debug-guard/klang.h `
  --candidate-header include/klang.h `
  --source examples/Vocoder.k --source sounds/Harrier.h `
  --allow-output-changes --inspect --output build/function-flow/recheck
```

`prepare.py` is pinned to the retained pre-promotion debug-guard header.
Generated compiler outputs belong under `build/function-flow/`.
