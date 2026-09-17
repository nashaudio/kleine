# Time unit spelling probe

The replacement-header work now lives in [core](../core/README.md), following
Chris's decision to develop shared generic numeric types with `signal64` and
`param64`. This original probe remains float-backed; double-backed Time and its
conversions are part of the replacement design, not implemented here yet.

Follow-up: the [typed frequency literal prototype](../literals/README.md) tests
the same mechanism using existing Klang types, including compatibility, overload
selection, generated code and audio. Time remains a separate API design.

Design discussion, 16 September 2026. This is a C++17 syntax/representation
experiment, not a promoted API. The scheduler and core Klang are unchanged.

[syntax.cpp](../../../../experiments/klang/time-units/syntax.cpp) compares:

1. A concrete `Time` alias to a millisecond specialisation of an internal
   `Value<Unit>` template, with `Time::s`, `Time::ms`, `Time::samples` aliases.
   Bare `Time` works as a variable, data member or function parameter. `Seconds`
   and `Samples` can be equivalent public aliases for the arrow-conversion
   vocabulary discussed with Chris.
2. A public `Time<Unit>` template, defaulting to milliseconds. With the tested
   constructor, `Time local;`, `Time local(100);`, and `Time local = 100;` deduce
   the default specialisation in C++17. An ordinary function parameter or
   non-static model member still needs `Time<>` or an explicit unit. A default
   template argument does not make `Time::s` valid; `Time<>::s` is valid.
3. Optional `_ms`, `_s` and `_samples` literals returning those typed values.
   They work for integer and floating literals; numeric conversion retains
   existing `param` float precision.

All three native-unit value types in candidate 1 derive from the actual
`klang::param` and occupy four bytes on MSVC x64. There is no per-value unit tag
or allocation. Units are encoded in the C++ type. `Time::s(1)` holds `1` second;
`Time::ms(1)` holds `1` millisecond. Choosing a public default unit does not
require the scheduler to use the same internal clock representation.

Recommendation for discussion: default bare `Time` to milliseconds to match the
proposed bare-number `every` contract, preserve native units in explicit types,
and retain integer/double sample deadlines inside the scheduler. Seconds versus
milliseconds is a scale choice, not a cure for long-running float-clock drift.
Sample counts must preserve their sample basis until a conversion to physical
time is explicitly requested; that conversion depends on the sample rate.

The probe checks spelling, type identity, size, literals and a typed conversion
function. It does **not** implement arrow conversions, unit-aware arithmetic,
cross-unit construction, timer overloads, range policy, or dimensional safety.
Inherited `param`/`signal` arithmetic can discard units. Existing `set(param)`
APIs still need conversion to their documented unit before type erasure. Typed
timer overloads must preserve the unit before converting a value to `param`.

## Validation

MSVC 19.51.36257, `/std:c++17 /O2`, x64:

- Positive executable passes type/size assertions and value checks.
- `REJECT_MEMBER` fails with C3000: deduction is disallowed for non-static members.
- `REJECT_PARAMETER` fails with C2955: template argument list required.
- `REJECT_SCOPE` fails with C2955: template argument list required before scope lookup.

The C++ [class template deduction rules](https://eel.is/c++draft/dcl.type.class.deduct)
describe where a deduced class placeholder may appear; the actual checks here
use C++17 rather than assuming the current draft's newer facilities apply.

**Clang check:** the complete positive probe also compiles and runs with the
installed LLVM `clang-cl` 22.1.3 (x64 Windows/MSVC target), `/std:c++17 /O2`.
The same type/size/value checks pass. The expected-rejection cases above were
checked with MSVC, not repeated with Clang.

## Why `100_ms` works

User-defined literals are a standard **C++11** feature. A namespace-scope
`operator""_ms(unsigned long long)` handles `100_ms`; an overload taking
`long double` handles `100.0_ms`. These are ordinary functions returning typed
values, with no allocation in this implementation. The compiler recognises the
literal suffix and selects the corresponding operator. They apply to literal
tokens, not variable names; use a constructor such as `Time::ms(value)` for a
runtime value. Keep the suffixes in a literals namespace and import it where
needed. Application-defined suffixes use a leading underscore.

[Clang's support table](https://clang.llvm.org/cxx_status.html#cxx11) lists
user-defined literals since Clang 3.1. They are also documented by
[Microsoft](https://learn.microsoft.com/en-us/cpp/cpp/user-defined-literals-cpp)
and listed in [GCC's C++11 support table](https://gcc.gnu.org/projects/cxx-status.html#cxx11).
The probe's `if constexpr` and class-template deduction require C++17; the
literal syntax itself does not.

From an x64 Visual Studio developer command prompt, at the repository root:

```text
mkdir build\time-units
cl /nologo /std:c++17 /EHsc /O2 /DNOMINMAX /wd4244 /wd4305 /Iinclude experiments\klang\time-units\syntax.cpp /Fobuild\time-units\syntax.obj /Febuild\time-units\syntax.exe
build\time-units\syntax.exe
```

To reproduce an expected rejection, compile with `/c /DREJECT_MEMBER`,
`/c /DREJECT_PARAMETER`, or `/c /DREJECT_SCOPE`, with the object under
`build/time-units`. No original production source or generated scheduling
measurement has been changed by this probe.
