# Arithmetic restoration and game-sound comparison

**Follow-up:** the [supplied UE 0.7.9 review](GAME-HEADER-REVIEW.md) establishes
why the actual game sounds compile. It supersedes the unknown-game-header
assumptions below, while preserving these earlier 0.7.8/candidate results.
The [OSM review](../../../../klang/OSM-IMPROVED.md#archived-material) separates verified corrections from remaining
oscillator faults; PD oscillator comparisons do not exercise OSM.

16 September 2026. Changes are confined to the experimental core and test/docs
sources. Production `include/klang.h`, examples, sounds and Farnell models remain
unchanged. The planned generic numeric/64-bit refactor has not started.

## Restoration

The four commented upstream overloads enable `source * .5`, `osc / 3` and the
equivalent `+`/`-` expressions. Restoring them verbatim fixes ten examples but
introduces a compile regression in Harrier:

```cpp
gain = abs(0.5 - speed) * 2 + 0.50;
```

`abs(...)` is a Klang Function wrapper with its own conversion to `float`.
The old free overload competes with built-in multiplication: neither candidate
has the best conversion for both operands. Both MSVC and Clang reject it. The
same error occurs with the main/plugins upstream headers. This is a plausible
reason for disabling the overloads in a game-oriented fork; no authoring history
establishes that as the actual reason.

The candidate instead constrains the four new overloads to non-const Output
objects and arithmetic scalar temporaries. Existing lvalue member arithmetic
keeps its established overload path. Objects already implicitly convertible to
`float`, including Function wrappers, keep their existing arithmetic path and
result types. Each new overload materialises the output once and retains the
upstream float narrowing of the scalar.

The `OutputSignal`/`ScalarOutput` helper aliases affect overload participation
only; they add no runtime state, branch or allocation. They deliberately describe
the current float contract. The future double-backed implementation must revisit
the narrowing and float-conversion constraint rather than reusing them blindly.

## Unchanged-source compilation

MSVC 19.51.36257 and Clang 22.1.3, C++17, Windows x64, `/O2 /fp:precise`.

| Collection | MSVC production | MSVC candidate | Clang production | Clang candidate |
| --- | ---: | ---: | ---: | ---: |
| examples | 40/53 | 50/53 | 40/53 | 50/53 |
| sounds, raw | 1/7 | 2/7 | 1/7 | 2/7 |
| farnell/klang | 20/20 | 20/20 | 18/20 | 18/20 |
| **Total** | **61/80** | **72/80** | **59/80** | **70/80** |

No pass-to-fail regressions. The ten recovered examples are those identified in
[EXAMPLE-REVIEW.md](EXAMPLE-REVIEW.md); Nature's Rain is the eleventh recovered
file. DX7/FM table initialisation and the Subtractive/Filter Envelope conversion
remain unresolved. Clang's existing Farnell failures remain in AlarmGenerator
and PhoneEffects. Kleine builds and links on MSVC with the candidate selected
through include-directory order; `--help` runs successfully.

## Sound headers

Chris reports UE 5.8.1, VS 2026 and compiler settings enforced by Unreal Build
Tool. This review uses standalone VS 2026 MSVC and Clang, not a UE build.
Host helpers and pinned upstream header identities are explicit in the
[test documentation](../../../../tests/klang/README.md).

MSVC results after adding only the labelled test-host dependencies:

| Unchanged sound | Production | Candidate | Upstream main | Upstream plugins |
| --- | --- | --- | --- | --- |
| Bicycle | Scalar arithmetic fails | Runs | Runs | Runs |
| Harrier | Runs | Runs | Function arithmetic fails | Function arithmetic fails |
| Helicopter | Function arithmetic fails | Same | Function arithmetic fails | Same |
| Mini | Function arithmetic fails | Same | Same | Same |
| Motors: Car, ToyBoatEngine, FourStrokeEngine | Scalar arithmetic and Bank routing fail | Bank routing fails | Bank routing fails | Missing DCF blocks comparison |
| Nature: Rain | Scalar arithmetic fails | Runs | Runs | Runs |
| Train, with explicit Reverb2 reconstruction | Scalar arithmetic/control routing fail | Control routing fails | Control routing fails | Missing DCF/control routing |

Without host helpers, only Harrier compiles against production; Harrier and Rain
compile against the candidate. Neither upstream header compiles any of the seven
raw sound headers, largely because the host's Sound names are missing.

On Clang the candidate additionally compiles/renders the reconstructed Train;
its `Control >> gate` expression is accepted there but ambiguous on MSVC.
The upstream main header fails earlier in its packed-struct declarations, so its
Clang sound behaviour cannot be compared. Plugins still runs Bicycle and Rain.

The remaining failures have useful minimal forms:

- Mini/Helicopter: `1 - abs(signal(...))`. This is the existing scalar-left
  counterpart of the function overload conflict. It predates this restoration.
- Motors: `Bank<BPF, 3> >> signal` has no supported reduction from its three
  output channels to mono. That one ToyBoatEngine definition blocks the whole
  Motors header, including Car/FourStrokeEngine; their failure here does not
  establish a separate defect inside each of those objects.
- Train: its source includes `Reverb.k` but instantiates `Reverb2`. Supplying the
  latter explicitly exposes ambiguous control routing on MSVC. Choosing the real
  dependency and defining the routing/reduction contracts remain follow-ups.

These are interface/source issues, not evidence of audible differences. The
tests preserve them as failures rather than changing the model topology.

## Behaviour

Six-second fresh-process renders at 44.1/48 kHz, fixed seed and documented control
changes. No alignment, gain adjustment, resampling or normalisation is applied.

| Candidate sound | Available comparison | Result at both rates |
| --- | --- | --- |
| Harrier | Production, on MSVC and Clang | Sample-identical |
| Bicycle | Main/plugins on MSVC; plugins on Clang | Sample-identical |
| Rain | Main/plugins on MSVC; plugins on Clang | Sample-identical |
| Train reconstruction | Candidate Clang only | Finite, repeatable; no reference parity claim |

All candidate renders repeat exactly in fresh processes. The tests retain
unnormalised levels: Harrier peaks at approximately 11.85/11.15 on MSVC for these
controls, identically in production and candidate. This is existing model output
above unity, not a regression or a level-safe listening asset. Train peaks around
1.07/1.14 on Clang. Other blocked sounds have no runtime result.

The language suite adds 227 candidate runtime checks per compiler, compile-time
type/layout assertions, diagnosed known-failure probes, and the existing typed
literal/oscillator test. It checks exactly-once processing and preserves cached
const reads, relative setter dispatch and routing order. This begins a reusable
language regression suite; it does not establish complete UE, platform, API or
audio acceptance.

## Evidence and remaining work

[arithmetic-review.json](../../../../../experiments/klang/core/arithmetic-review.json) retains compact results, hashes,
compiler versions and render residuals. Full logs and commands are under:

- `build/core-arithmetic/`: the rejected verbatim restoration, including Harrier's
  new failure.
- `build/core-arithmetic-refined/`: final 80-file checks and MSVC Kleine build.
- `build/language-regression/`: language/literal checks on both compilers.
- `build/sound-headers/`: raw/host sound builds, pinned header copies and renders.

Next steps are the existing table, conversion and routing contracts, then the
generic numeric core. The arithmetic candidate is ready for review; the
production header has not been replaced. The suite keeps the original models as
compatibility evidence before any later syntax/literal migration.
