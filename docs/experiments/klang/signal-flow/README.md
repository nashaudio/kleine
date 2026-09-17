# Inline signal-flow ambiguity (K-002)

Chris has prioritised ambiguity resolution. This reproducer isolates the failure
encountered while simplifying Boing's `phase(...) >> clamped` expression. It uses
the current, unchanged `include/klang.h` and no PD primitives.

With MSVC 19.51, `source(2) >> destination` produces C2593: the member routing
operator on `Generic::Output<signal>` competes with the mutable and const free
`klang::operator>>` templates. `Generator::operator()` returns that generic output
reference, exposing the competing overloads.

The default build uses `signal(source(2)) >> destination` and checks both the
result and one evaluation of each object. This inline conversion avoids a named
temporary, but remains a workaround, not the intended final syntax.

[Retained results](../../../../experiments/klang/signal-flow/results.json) record the compiler, source hashes, competing
overloads and successful one-evaluation check.

From an x64 Visual Studio developer prompt:

```text
mkdir build\signal-flow
cl /nologo /std:c++17 /EHsc /O2 /wd4244 /wd4305 /DNOMINMAX /Iinclude experiments\klang\signal-flow\inline-route.cpp /Fobuild\signal-flow\inline-route.obj /Febuild\signal-flow\inline-route.exe
build\signal-flow\inline-route.exe
cl /nologo /std:c++17 /EHsc /O2 /wd4244 /wd4305 /DNOMINMAX /DREPRODUCE_AMBIGUITY /Iinclude /c experiments\klang\signal-flow\inline-route.cpp /Fobuild\signal-flow\expected-failure.obj
```

The final command is expected to fail. Compiler logs belong under
`build/signal-flow/`. A future fix should be prototyped here and cover const versus
mutable sources, stored and temporary results, modifier chains, discarded
expressions, and ordinary stream extraction (K-015), with explicit checks of
evaluation count and order. Core promotion remains subject to review.
