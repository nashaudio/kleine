# Control assignment and routing

Integrated into `include/klang.h` on 17 September 2026.

The previous `Control` mixed four implicit conversions with unconstrained
member and global routing templates. In particular, `Control >> Modifier` was
ambiguous under MSVC, while direct value assignment was unavailable.

The promoted contract separates the operations:

```cpp
controls[3] = 0.5f;          // Value assignment, clamped to the control range.
source >> controls[3];       // Evaluate source once and write the value.
controls[3] >> destination;  // Feed the cached control value forward.

meter = controls[4];         // Bind a ControlMap to a UI control.
```

- Arithmetic and signal-convertible value assignments are constrained
  separately. `Control` retains its implicit trivial copy assignment and its
  existing size/layout contract.
- The unconstrained member `Control::operator>>` overloads are removed. The
  existing global destination-driven router is sufficient and preserves
  mutable-source processing versus const cached reads.
- `ControlMap` binding and value assignment are distinct overloads: assigning
  a `Control` binds the map, while assigning a scalar or signal writes through
  an existing mapping. This avoids
  the null-map defect reproduced while developing the first forwarding form.

## Validation

- `tests/klang/control-routing.cpp` passes 17 runtime checks under MSVC and
  Studio Clang: clamping, `controls[i] = value`, mapped writes, binding,
  mutable/const source evaluation, mono/stereo routing and chain order.
- The former `Control >> Modifier` ambiguity probe compiles on both compilers.
- The 62-model example/sound corpus has no compile regressions. Train's host
  fixture now compiles under MSVC as well as Clang.
- Compressor remains sample-identical at 44.1/48 kHz on both compilers.

Generated reports are under `build/control-routing-final/`; focused Compressor
evidence is under `build/control-routing/compressor-*`.
