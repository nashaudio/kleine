"""Prepare the function-flow prototype from the accepted working header."""

from __future__ import annotations

import hashlib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / "experiments/klang/debug-guard/klang.h"
TARGET = Path(__file__).resolve().parent / "klang.h"
EXPECTED_SOURCE_SHA256 = "5936c841df12cbe65955157c5aa64b260e7c81ad17333680009c4aa9ac7aad44"


def replace_once(source: str, old: str, new: str) -> str:
    if source.count(old) != 1:
        raise RuntimeError(f"expected one occurrence, found {source.count(old)}: {old[:80]!r}")
    return source.replace(old, new, 1)


source_bytes = SOURCE.read_bytes()
source_hash = hashlib.sha256(source_bytes).hexdigest()
if source_hash != EXPECTED_SOURCE_SHA256:
    raise RuntimeError(
        f"working header changed: expected {EXPECTED_SOURCE_SHA256}, found {source_hash}"
    )

newline = "\r\n" if b"\r\n" in source_bytes else "\n"
source = source_bytes.decode().replace("\r\n", "\n")

# Function-like platform macros expand even in qualified names such as
# std::abs. Klang owns these spellings after inclusion rather than relying on
# NOMINMAX or on include order.
source = """#ifdef abs
#undef abs
#endif
#ifdef sqrt
#undef sqrt
#endif
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifdef clamp
#undef clamp
#endif

""" + source

source = replace_once(
    source,
    """// provide access to original math functions through std:: prefix
namespace std {
\tnamespace klang {
\t\ttemplate<typename TYPE> TYPE sqrt(TYPE x) { return SQRT(x); }
\t\ttemplate<typename TYPE> TYPE abs(TYPE x) { return ABS(x); }
\t}
};

""",
    "",
)

source = replace_once(
    source,
    """\t/// Stream a literal / constant / scalar type into a signal.
\tinline static signal& operator>>(float input, signal& destination) { // CHECK: should this be signal, rather than float?
\t\tdestination << signal(input);
\t\treturn destination;
\t}
""",
    """\t/// Stream a literal / constant / scalar type into a signal.
\tinline static signal& operator>>(float input, signal& destination) { // CHECK: should this be signal, rather than float?
\t\tdestination << signal(input);
\t\treturn destination;
\t}

\t/// A stateless unary C/C++ function that maps one float sample to another.
\tusing UnaryFunction = float(float);

\t/// Apply an ordinary unary function within a signal-flow expression.
\tinline signal operator>>(signal input, UnaryFunction& function) {
\t\treturn function(float(input));
\t}

""",
)

# C++ cannot overload scalar >> function because neither operand is a class.
# This chain already narrowed its double accumulator through Function's signal
# input; preserve that boundary explicitly now that sqrt is an overload set.
source = replace_once(
    source,
    "\t\t\t\tsum* window.inv >> sqrt >> ar >> out;",
    "\t\t\t\tsignal(sum * window.inv) >> sqrt >> ar >> out;",
)

source = replace_once(
    source,
    """\t\t\t/// sin approximation [-pi/2, pi/2] using odd minimax polynomial (Robin Green)
\t\t\tinline static float polysin(float x) {
\t\t\t\tconst float x2 = x * x;
\t\t\t\treturn (((-0.00018542f * x2 + 0.0083143f) * x2 - 0.16666f) * x2 + 1.0f) * x;
\t\t\t}

\t\t\t/// fast sine (based on V2/Farbrausch; using polysin)
\t\t\tinline static float fastsin(float x)
\t\t\t{
\t\t\t\t// Range reduction to [0, 2pi]
\t\t\t\t//x = fmodf(x, twoPi);
\t\t\t\t//if (x < 0)\t\t\t// support -ve phase (e.g. for FM)
\t\t\t\t//\tx += twoPi;\t\t//
\t\t\t\tx = fast_mod2pi(x);

\t\t\t\t// Range reduction to [-pi/2, pi/2]
\t\t\t\tif (x > 3 / 2.f * pi)\t// 3/2pi ... 2pi
\t\t\t\t\tx -= twoPi;\t// (= translated)
\t\t\t\telse if (x > pi / 2)\t// pi/2 ... 3pi/2
\t\t\t\t\tx = pi - x;\t\t// (= mirrored)

\t\t\t\treturn polysin(x);
\t\t\t}

\t\t\t/// fast sine (using polysin and integer math)
\t\t\tinline static float fastsinp(unsigned int p)
\t\t\t{
\t\t\t\t// Range reduction to [0, 2pi]
\t\t\t\t//x = fmodf(x, twoPi);
\t\t\t\t//if (x < 0)\t\t\t// support -ve phase (e.g. for FM)
\t\t\t\t//\tx += twoPi;\t\t//
\t\t\t\tfloat x = fast_modp(p);

\t\t\t\t// Range reduction to [-pi/2, pi/2]
\t\t\t\tif (x > 3.f / 2.f * pi)\t// 3/2pi ... 2pi
\t\t\t\t\tx -= twoPi;\t\t\t// (= translated)
\t\t\t\telse if (x > pi / 2.f)\t// pi/2 ... 3pi/2
\t\t\t\t\tx = pi - x;\t\t\t// (= mirrored)

\t\t\t\treturn polysin(x);
\t\t\t}

""",
    "",
)

source = replace_once(
    source,
    """\t/// Common audio generators / oscillators.
\tnamespace Generators {
""",
    """\t/** Fast scalar approximations. Angles use radians unless a `p` suffix
\t * identifies an unsigned 32-bit phase covering one complete turn.
\t * Sine uses the retained V2/Farbrausch range reduction and Robin Green
\t * odd polynomial. Cosine reuses that polynomial with direct quadrant
\t * reduction, avoiding an added pi/2 before the existing modulo step.
\t */
\tnamespace fast {
\t\tconstexpr float twoPi = float(2.0 * 3.1415926535897932384626433832795);

\t\tnamespace detail {
\t\t\tinline float sin(float x) {
\t\t\t\tconst float x2 = x * x;
\t\t\t\treturn (((-0.00018542f * x2 + 0.0083143f) * x2 - 0.16666f) * x2 + 1.0f) * x;
\t\t\t}
\t\t}

\t\t/// Approximate sine of a finite angle in radians.
\t\tinline float sin(float x) {
\t\t\tx = fast_mod2pi(x);
\t\t\tif (x > 3 / 2.f * pi)
\t\t\t\tx -= twoPi;
\t\t\telse if (x > pi / 2)
\t\t\t\tx = pi - x;
\t\t\treturn detail::sin(x);
\t\t}

\t\t/// Approximate cosine of a finite angle in radians.
\t\tinline float cos(float x) {
\t\t\tx = fast_mod2pi(x);
\t\t\tx = x < pi ? pi / 2.f - x : x - 3.f / 2.f * pi;
\t\t\treturn detail::sin(x);
\t\t}

\t\t/// Approximate sine from an unsigned 32-bit phase covering one turn.
\t\tinline float sinp(unsigned int phase) {
\t\t\tfloat x = fast_modp(phase);
\t\t\tif (x > 3.f / 2.f * pi)
\t\t\t\tx -= twoPi;
\t\t\telse if (x > pi / 2.f)
\t\t\t\tx = pi - x;
\t\t\treturn detail::sin(x);
\t\t}

\t\t/// Approximate cosine from an unsigned 32-bit phase covering one turn.
\t\tinline float cosp(unsigned int phase) {
\t\t\tfloat x = fast_modp(phase);
\t\t\tx = x < pi ? pi / 2.f - x : x - 3.f / 2.f * pi;
\t\t\treturn detail::sin(x);
\t\t}
\t}

\t/// Common audio generators / oscillators.
\tnamespace Generators {
""",
)

source = replace_once(
    source,
    "\t\t\t\t\tout = fastsinp(position.position + offset.position);",
    "\t\t\t\t\tout = klang::fast::sinp(position.position + offset.position);",
)

source = replace_once(
    source,
    """\t/// Square root function (audio object)
\tinline static Function<float> sqrt(SQRTF);
\t/// Absolute/rectify function (audio object)
\tinline static Function<float> abs(FABS);
\t/// Square function (audio object)
\tinline static Function<float> sqr([](float x) -> float { return x * x; });
\t/// Cube function (audio object)
\tinline static Function<float> cube([](float x) -> float { return x * x * x; });

#define sqrt klang::sqrt // avoid conflict with std::sqrt
#define abs klang::abs   // avoid conflict with std::abs
""",
    """\t// Extend common standard unary maths overload sets for Klang wrapper types.
#define KLANG_UNARY_MATH(function) \\
\tusing std::function; \\
\tinline signal function(signal input) { return std::function(input.value); } \\
\tinline float function(const Control& input) { return float(std::function(float(input))); } \\
\tinline float function(const ControlMap& input) { return float(std::function(float(input))); }

\tKLANG_UNARY_MATH(abs)
\tKLANG_UNARY_MATH(sqrt)
\tKLANG_UNARY_MATH(cbrt)
\tKLANG_UNARY_MATH(sin)
\tKLANG_UNARY_MATH(cos)
\tKLANG_UNARY_MATH(tan)
\tKLANG_UNARY_MATH(asin)
\tKLANG_UNARY_MATH(acos)
\tKLANG_UNARY_MATH(atan)
\tKLANG_UNARY_MATH(sinh)
\tKLANG_UNARY_MATH(cosh)
\tKLANG_UNARY_MATH(tanh)
\tKLANG_UNARY_MATH(asinh)
\tKLANG_UNARY_MATH(acosh)
\tKLANG_UNARY_MATH(atanh)
\tKLANG_UNARY_MATH(exp)
\tKLANG_UNARY_MATH(exp2)
\tKLANG_UNARY_MATH(expm1)
\tKLANG_UNARY_MATH(log)
\tKLANG_UNARY_MATH(log2)
\tKLANG_UNARY_MATH(log10)
\tKLANG_UNARY_MATH(log1p)
\tKLANG_UNARY_MATH(floor)
\tKLANG_UNARY_MATH(ceil)
\tKLANG_UNARY_MATH(trunc)
\tKLANG_UNARY_MATH(round)
\tKLANG_UNARY_MATH(erf)
\tKLANG_UNARY_MATH(erfc)

#undef KLANG_UNARY_MATH

\t/// Stateless square and cube functions preserve Klang signals in audio expressions.
\tinline signal sqr(signal input) { return input * input; }
\tinline signal cube(signal input) { return input * input * input; }
\ttemplate<typename TYPE, std::enable_if_t<std::is_arithmetic_v<TYPE>, int> = 0>
\tinline TYPE sqr(TYPE input) { return input * input; }
\ttemplate<typename TYPE, std::enable_if_t<std::is_arithmetic_v<TYPE>, int> = 0>
\tinline TYPE cube(TYPE input) { return input * input * input; }
\tinline float sqr(const Control& input) { const float value = float(input); return value * value; }
\tinline float cube(const Control& input) { const float value = float(input); return value * value * value; }
\tinline float sqr(const ControlMap& input) { const float value = float(input); return value * value; }
\tinline float cube(const ControlMap& input) { const float value = float(input); return value * value * value; }
""",
)

source = replace_once(
    source,
    """\ttemplate<typename TYPE>
\tstatic GraphPtr& operator>>(TYPE(*function)(TYPE), klang::GraphPtr& graph) {
\t\tgraph->plot(function);
\t\treturn graph;
\t}
""",
    """\tinline static GraphPtr& operator>>(UnaryFunction& function, klang::GraphPtr& graph) {
\t\tgraph->plot(static_cast<float(*)(float)>(function));
\t\treturn graph;
\t}

\ttemplate<typename TYPE>
\tstatic GraphPtr& operator>>(TYPE(*function)(TYPE), klang::GraphPtr& graph) {
\t\tgraph->plot(function);
\t\treturn graph;
\t}
""",
)

source = replace_once(
    source,
    """\ttemplate<typename TYPE>
\tstatic Graph& operator>>(TYPE(*function)(TYPE), Graph& graph) {
\t\tgraph.plot(function);
\t\treturn graph;
\t}
""",
    """\tinline static Graph& operator>>(UnaryFunction& function, Graph& graph) {
\t\tgraph.plot(static_cast<float(*)(float)>(function));
\t\treturn graph;
\t}

\ttemplate<typename TYPE>
\tstatic Graph& operator>>(TYPE(*function)(TYPE), Graph& graph) {
\t\tgraph.plot(function);
\t\treturn graph;
\t}
""",
)

TARGET.write_bytes(source.replace("\n", newline).encode())
print(f"Prepared {TARGET.relative_to(ROOT)} from {source_hash}.")
