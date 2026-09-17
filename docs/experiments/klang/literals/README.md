# Typed Klang literals

Current direction: integrate literals into the [experimental replacement
core](../core/README.md) alongside generic float/double numeric types. The optional
header below remains an isolated, tested prototype; its original promotion
discussion is retained as context.

16 September 2026. Additive prototype for review; `include/klang.h` and existing
models are unchanged. [literals.h](../../../../experiments/klang/literals/literals.h) is a candidate optional header,
using the proposed `klang::literals` namespace and the **existing** `Frequency`.

```cpp
#include "experiments/klang/literals/literals.h"
using namespace klang::literals;

auto frequency = 440_Hz;       // klang::Frequency
auto precise = 440.0_Hz;       // also Frequency
auto shortDecimal = 440._Hz;  // also Frequency
auto cutoff = 2.5_kHz;        // Frequency containing 2500 Hz

// In a model's process():
0.2f * osc(440_Hz) >> out;
```

`Frequency` derives from `param`, which derives from `signal`; the values work
with existing `set(param)` interfaces and retain Klang's float precision and
four-byte representation. Each suffix has integer (`unsigned long long`) and
floating (`long double`) overloads. `_kHz` scales before conversion to float.
There is no allocation or runtime unit tag.

**Use `1_Hz`, `1.0_Hz` or `1._Hz`, not `1.f_Hz`.** C++ parses `f_Hz` as one
suffix, not two successive suffixes. The return type is supplied by the operator;
the `f` is unnecessary. Both tested compilers reject `1.f_Hz`. See the
[literal grammar](https://eel.is/c++draft/lex.ext) and
[literal-operator rules](https://eel.is/c++draft/over.literal).
Use the adjacent spelling `operator""_Hz` for its declaration.

## Compatibility and inference

- No existing constructors, conversions, arithmetic or routing overloads change.
  Ordinary `1`, `1.f` and `1.0` keep their types and overload choices, including
  when `klang::literals` is imported.
- Including the header does not import its suffixes. Callers opt in with a using
  directive in their chosen scope. This also lets callers manage collisions with
  another library defining the same suffix.
- `auto frequency = 440_Hz` retains the exact domain type. An overload taking
  `Frequency` wins over overloads taking `Pitch`, `param`, `signal` or `float`.
- `std::max(frequency, 440_Hz)` and `std::clamp(frequency, 20_Hz, 20_kHz)` deduce
  one consistent type. The otherwise equivalent mixed `Frequency`/plain-number
  `std::max` call is rejected. Copy the result when passing temporaries; these
  standard algorithms return references.
- This is typed construction, not dimensional arithmetic. Existing arithmetic
  may discard the domain type; a `set(param)` interface also erases that type.
  Unary minus is separate from a literal token. Before adding e.g. `_dB`, review
  `-6_dB` and subsequent conversions rather than promising preserved types.
- `Frequency` currently has non-constexpr constructors. These operators are
  inline, not constexpr; compile-time type information does not imply usable
  constant-expression objects. A constexpr extension would be a separate core
  proposal.

The existing `Output&` routing ambiguity (K-002) remains: changing the argument
to `source(440_Hz) >> modifier` does not change the returned type. The tests also
reproduce the existing rejection of `source(440) * 0.2f`: its member operator
takes a non-const reference. `0.2f * source(440_Hz)` works and is used above.
`Controls::set` still restricts arguments to arithmetic types (K-001).

## Validation

C++17, x64 Windows, MSVC 19.51.36257 and clang-cl 22.1.3, optimisation `/O2`:

| Check | MSVC | Clang |
| --- | --- | --- |
| Exact literal types, plain-number overloads, values and standard algorithms | Pass | Pass |
| Existing virtual `set(param)` and exactly-once evaluation | Pass | Pass |
| PD oscillator: plain numbers versus Hz and kHz, including frequency change | Sample-identical | Sample-identical |
| Five expected-rejection cases | Confirmed | Confirmed |
| Constant-construction assembly | Same load/return instructions | Same load/return instructions |
| Unmodified Kleine translation unit, suffixes imported by force-include | Compiles | Four existing errors, also present without header |

[probe.cpp](../../../../experiments/klang/literals/probe.cpp) compares 184,200 frames across two-second renders at
44.1 and 48 kHz. [codegen.cpp](../../../../experiments/klang/literals/codegen.cpp) shows `440.f`, explicit construction,
`440_Hz`, `440.0_Hz` and `.44_kHz` all reduce to loading the same float constant
and returning, with no suffix call or multiplication left in these builds.
This is focused code-generation evidence, not a general performance guarantee.

[rejections.cpp](../../../../experiments/klang/literals/rejections.cpp) retains the invalid `f_Hz` spelling, missing
namespace import, mixed-type `std::max`, typed inline routing, and existing
output/rvalue multiplication cases. Check diagnostics as well as exit codes.

The full Clang application check fails on the same four errors with and without
the header: AlarmGenerator's float/signal conditional, PhoneEffects' output plus
temporary signal, and two Engine::File string initialisations in `kleine.cpp`.
No new error appears. This does not establish a working Clang application build;
the standalone literal/audio probe does build and run with Clang.

### Reproduce

From an x64 Visual Studio developer PowerShell at the repository root:

```powershell
./experiments/klang/literals/check.ps1
./experiments/klang/literals/check.ps1 -Compiler clang-cl.exe -Output build/literals/clang -SkipApplication
```

Use the full path to `clang-cl.exe` if it is not on PATH. Omitting
`-SkipApplication` also attempts the full application check and currently reports
the existing Clang errors above. Compiler logs, objects, executables and assembly
are generated beneath `build/literals`; none are source deliverables. The full
application check compiles its translation unit; it does not relink or render it.

## Next review

Treat unit literals as a P1 language addition. Review promotion to an optional
`<klang/literals.h>` header, or availability through `klang.h`, while keeping
namespace import explicit. Neither requires changes to old source expressions.
Agree suffixes for the remaining domain types and finish the
[Time design](../time-units/README.md) before expanding that API.

Then review **all 53 example `.k` files**, followed by sound models, for clearer
frequency, duration, gain and pitch expressions. Preserve defaults and audio;
do not mechanically replace unitless ratios, counts, phases or values whose
parameter units are different. Catalogue ambiguity cases before/after each
substitution. The examples have not been migrated or individually compiled in
this first prototype.
