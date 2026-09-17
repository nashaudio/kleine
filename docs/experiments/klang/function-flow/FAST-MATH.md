# Fast scalar maths review

17 September 2026. The `fast::sin`/`fast::cos` refactor described below is
promoted. Other sourced approximations remain deferred. This review does not
claim that a mathematically cheap expression is faster than the platform
library without measurement on the target compiler/CPU.

## Implemented

The retained V2/Farbrausch sine implementation is moved without arithmetic
changes from `fastsin`/`fastsinp` into `klang::fast::sin` and
`klang::fast::sinp`. The optimised Sine oscillator calls the renamed phase
function. Focused tests compare one million radian inputs and one million phase
words against a literal copy of the previous implementation; both MSVC and
Studio Clang are bit-identical under precise and fast floating-point modes.

`fast::cos` and `fast::cosp` reuse the same odd polynomial. After the existing
modulo reduction, cosine maps `[0, pi)` with `pi/2 - x` and `[pi, 2pi)` with
`x - 3pi/2`, placing the polynomial input directly in `[-pi/2, pi/2]`. This is
the usual quarter-cycle sine/cosine relationship without adding pi/2 before
the modulo operation. The official CMSIS-DSP cosine likewise selects cosine by
adding a quarter cycle before reading its sine table, while Robin Green's
range-reduction discussion treats sine/cosine reconstruction together.

Across one million samples spanning -100pi to +100pi, precise builds measured:

| Compiler | sin peak / RMS error | cos peak / RMS error |
| --- | --- | --- |
| Studio Clang 14 | 2.06493e-5 / 5.28522e-6 | 2.06502e-5 / 5.28805e-6 |
| MSVC 19.51 | 2.06493e-5 / 5.28522e-6 | 2.06502e-5 / 5.28805e-6 |

Fast-math builds remain below 3e-5 peak error in the same test. These functions
retain the old finite-float range-reduction assumptions; exceptional and very
large inputs require a separate contract.

## Sourced candidates, not implemented

- **Tangent:** Robin Green's 2020 *Even Faster Math Functions* presents full
  pi/4 range reduction, reconstruction around poles, and a degree-13 minimax
  polynomial. The slides do not state reusable source-code terms, so the
  coefficients were not copied. Cephes supplies a freely usable, copyrighted
  single-precision `tanf` with documented peak relative error 3.3e-7 over
  +/-4096, but it is an accuracy-oriented port rather than obviously cheaper
  than a modern libm call.
- **Arctangent/atan2:** Cephes `atanf` uses two range transformations and a
  polynomial, documenting peak relative error 1.9e-7 over [-10,10]. Jim
  Shima's public-domain self-normalising atan2 is much cheaper and explicitly
  intended for DSP, but is a separate two-input accuracy/zero-handling design.
- **Hyperbolic functions:** Paul Mineiro's BSD-licensed fastapprox repository
  contains scalar/vector cosh, sinh and tanh approximations, as well as trig,
  exponential and logarithmic functions. A port would need its copyright and
  licence notice plus independent audio-domain error, saturation, NaN and
  performance tests. Cephes also supplies sourced single-precision hyperbolic
  and inverse-hyperbolic implementations with documented errors, but they are
  not positioned as minimal approximations.
- **Table alternative:** ARM CMSIS-DSP provides Apache-2.0 sine and cosine using
  a 512-entry table plus linear interpolation. It is well sourced and portable
  to embedded targets but has a different memory/performance trade-off from the
  existing polynomial.

Sources:

- Robin Green, *Faster Math Functions* (GDC 2002):
  https://basesandframes.wordpress.com/wp-content/uploads/2016/05/rgreenfastermath_gdc02.pdf
- Robin Green, *Even Faster Math Functions* (GDC 2020):
  https://media.gdcvault.com/gdc2020/presentations/Even_Faster_Math_Green_Robin.pdf
- ARM CMSIS-DSP fast maths and source:
  https://arm-software.github.io/CMSIS-DSP/main/group__groupFastMath.html
- Cephes archive, licence note and single-precision documentation:
  https://www.netlib.org/cephes/
- Paul Mineiro, fastapprox:
  https://github.com/pmineiro/fastapprox
- Jim Shima, public-domain self-normalising atan2:
  https://dspguru.com/dsp/tricks/fixed-point-atan2-with-self-normalization/

No additional approximation should be integrated merely because its operation
count looks low. Each needs an explicit input domain, peak/RMS/relative error,
pole and exceptional-value behavior, licence notice, Debug/Release timings and
comparison with the target standard library.
