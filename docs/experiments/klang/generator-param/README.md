# Generator-to-param snapshots and inline setter forwarding

Integrated into `include/klang.h` on 17 September 2026.

A mutable Klang generator already processes when converted to `signal`; a const
generator returns its cached output. `param` now preserves that contract through
a constrained constructor for signal-producing objects:

```cpp
param evaluated = generator;       // Processes exactly once.
param cached = constGenerator;     // Reads the cached output.
```

Arithmetic, `signal`/`param`, relative and `Control` construction retain their
existing overloads. The new constructor excludes those categories instead of
loosening conversion globally.

Generic Generator and Modifier inline configuration now perfect-forwards its
arguments. This avoids copying a generator before converting it to a setter
parameter and supports the intended inline form:

```cpp
osc >> filter(env, 10) >> out;
```

## Validation

- Mutable and const generator-to-param behavior, Generator inline setters and
  Modifier inline setters pass five focused runtime checks under MSVC and Clang.
- Existing absolute/relative setter dispatch remains green.
- All 7 retained sound roots and all 53 examples compile under both compilers with no
  compile regressions.
- `examples/Subtractive/Filter.k` compiles and renders finite/repeatably at
  44.1/48 kHz on both compilers.
- The 1,105,295-check UE suite passes in Release and Debug on both compilers.

Evidence is under `build/forwarding-final/`.
