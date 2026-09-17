# Current example failures and the Klang plugins branch

Reviewed 16 September 2026. This is source/compile diagnosis, not an audio review.
Production examples, production `klang.h` and the experimental core copy are
unchanged. All branch extractions and diagnostic header changes are under
`build/research/klang-plugins-review/`.

**Subsequent work:** the candidate now has a constrained arithmetic restoration.
The [follow-up review](ARITHMETIC-REVIEW.md) supersedes the unchanged-candidate
status above and adds full-model regression and sound-render evidence. The
branch findings below describe the earlier investigation.

## References checked

The local Klang repository is `F:/klang/git`. Its remote references match the
heads returned by `git ls-remote https://github.com/nashaudio/klang.git`:

- [plugins, 3dc002be](https://github.com/nashaudio/klang/tree/3dc002be66a32166458b9679c11e94e437e819a1),
  27 January 2025: header at `include/klang.h`; examples at the root and in `SE412`.
- [main, 88b849c3](https://github.com/nashaudio/klang/tree/88b849c38997f4c34cadc4b92d1457e8ef43deea),
  23 June 2025: header at `klang.h`; examples under `examples/`.

The branches have diverged: main has 24 unique commits and plugins has 44.
Their dates alone do not establish which individual feature is newer.

**All 53 local example files match Klang main, ignoring line endings.** Plugins
contains counterparts for 34 of them, with `SynTHX.k` mapped to `THX.k`; ten of
those counterparts are identical. Seven of our 13 failing examples have plugins
counterparts. There is no newer replacement collection to import wholesale.

## The 13 failures

| Group | Files | Finding |
| --- | --- | --- |
| Output arithmetic with scalar temporaries | Additive/Inheritance, Additive/Nyquist, Additive/Saw, Filtering/WahWah, Physical/String, Subtractive/Modular, SuperSaw, SynTHX, TB303, Vocoder | All ten compile unchanged with main's header, and with just four arithmetic overloads restored in a scratch copy of our header. |
| Table initialisation | DX7, FM | Local examples are identical to plugins. Both upstream headers expose the one-argument `FUNCTION(type)` macro; our copy replaced it with a two-argument macro. Further errors remain even with the upstream macro/header. |
| Generator passed to a parameter setter | Subtractive/Filter | `filter.set(env, 10)` cannot convert `Envelope` to `param`. It also fails against main's header. No plugins counterpart exists. |

### 1. Four disabled free arithmetic overloads explain ten failures

Our header retains `Generic::Output` member arithmetic taking `TYPE&`, which
does not accept temporary scalar arguments. Upstream main and plugins also have
the complementary free overloads for an output on the left and a float on the
right. All four are commented out in our copy:

```cpp
template<typename SIGNAL>
inline SIGNAL operator*(Output<SIGNAL>& output, float other) {
    return SIGNAL(output) * other;
}
```

Equivalent overloads exist for `+`, `-` and `/`. Examples such as `osc / 3`,
`env * 0.5f` and `filter(1000) * 0.5` rely on these overloads or equivalent support.

A diagnostic copy changed **only** those four commented declarations back into
active declarations. All ten affected original examples then compiled with
MSVC. This isolates the cause without relying on the other upstream differences.
Only the original 13 failures were compiled with this diagnostic copy; it is
not yet an all-model regression pass or an evaluation-count/audio check.

Recommendation: restore this capability in the experimental core first, retaining
the source expressions. Test evaluation count, constness, operand order and
overload ambiguity before promoting the fix, and account for double precision
when generalising the overloads.

### 2. FUNCTION/Table needs its own compatibility repair

Both upstream branches define `FUNCTION(type)` using the namespace-level
`Result<type>`. The current copy defines `FUNCTION(type, size)`, references
`Table<type, size>::Result` even though Result is a namespace-level type, and
contains unmatched closing parentheses. Changing the example calls alone would
not repair that definition.

The upstream headers remove this particular mismatch, but both DX7 and FM still
fail in the focused MSVC checks:

- DX7 then reaches its non-standard `(int[20]) { ... }` array expression
  (C4576). This is an example portability issue beyond the macro mismatch.
- FM still reports C2059/C2143/C2447 at its table initializer. The remaining
  initializer failure needs an isolated MSVC diagnosis; the branch lookup did
  not establish its complete cause.

A Clang check of upstream main also exposes that header's old packed-struct
macro, so a whole-header swap is not an established cross-compiler solution.
Keep Table/macro compatibility and the remaining example portability issues
separate from the numeric-type refactor.

### 3. Envelope-to-param conversion is a core interface gap

`Envelope` provides an evaluated signal, while the filter setter takes `param`.
The existing conversion chain cannot supply that setter argument implicitly.
The compiler rejects the same expression against our header and upstream main.

Recommendation: review the intended evaluated-output-to-parameter contract in
the new core. A fix must evaluate the envelope exactly once; accepting the
expression by reading a stale cached output would change the sound.

## Focused plugins comparison

These are MSVC C++17 `/O2 /fp:precise` baseline-compatible compile checks with
Windows platform macros, separate translation units and root-model instantiation.
Compiler include traces verify the selected header.

| Failing local example | Plugins counterpart | Counterpart + our header | Counterpart + plugins header |
| --- | --- | --- | --- |
| DX7 | Identical | Fails: current FUNCTION macro | Fails: array literal |
| FM | Identical | Fails: current FUNCTION macro | Fails: table initializer |
| WahWah | Adds an unused helper/empty prepare; failing expression unchanged | Fails | Compiles |
| SuperSaw | Different controls/presets; failing expression unchanged | Fails | Compiles |
| SynTHX | THX; different name/defaults and unqualified Mono | Fails | Fails: Mono lookup |
| TB303 | Different filter reference and envelope/control expressions | Fails | Fails: Control power expression |
| Vocoder | UI layout/group differences; failing expression unchanged | Fails | Compiles |

The plugins example changes do not fix any of these seven when paired with our
current header. Keep the existing main-derived examples as the compatibility
suite; selectively bring back the missing core capability.

## Evidence and next step

Scratch evidence:

- `build/research/klang-plugins-review/mapping.json`: all 53 path comparisons.
- `compile-results.json`: upstream-main checks of all 13 failing local examples,
  seven plugins counterparts against both headers, and three header-only checks.
- `restore-results.json`: the isolated four-overload restoration checks.
- `main-to-current.diff`: header differences, including existing local engine,
  buffer and platform fixes that should survive the replacement.

The original [baseline](../../../../../experiments/klang/core/baseline.json) is still authoritative for the untouched
current header. Recommended next step: implement and verify the arithmetic
compatibility repair in the experimental copy, then isolate Table and setter
conversion issues. Migrate example syntax only after their current forms have
been assessed explicitly.
