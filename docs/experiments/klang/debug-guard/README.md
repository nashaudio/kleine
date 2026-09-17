# Optional Klang debug capture

Integrated into `include/klang.h` on 17 September 2026; version remains 0.7.8.
MSVC and Studio Clang 14 passed all 16 configurations. All 12 paired model
renders were sample-identical. The Studio-enabled Debug definition matches the
previous header after preprocessing and whitespace removal. Retained evidence
is in [validation.json](../../../../experiments/klang/debug-guard/validation.json).

`HAS_KLANG_DEBUG` controls the existing `debug` capture mechanism independently
of Release/Debug compilation. It defaults to 1 when Studio's wrappers define
`KLANG=1`, otherwise 0. Explicit `HAS_KLANG_DEBUG=0` or `=1` takes precedence.
Define it before including `klang.h`, consistently throughout a module.

```cpp
#define HAS_KLANG_DEBUG 0 // Optional explicit production-host setting.
#include <klang.h>
```

Models retain their debug statements. Disabled behaviour:

- `value >> debug` and `debug << value` accept the value without capturing it.
  Inline chains such as `(in >> debug) >> follower` pass the value through.
  A single thread-local signal holds that transient value; shared `debug.in`
  is not written, including through an `Input&` reference.
- `debug.print`, `printOnce`, and capture sessions do nothing; their queries
  report no captured audio/text. `PROFILE(...)` does not evaluate its arguments;
  direct `Debug::profile(...)` returns zero without invoking its callable.
- Effect/Sound/Note loops do not advance the debug cursor. Direct debug-buffer
  appends discard data; direct cursor increments do not advance.
- Ordinary C++ argument evaluation still occurs for function calls. Feeding a
  generator into `debug` can still process that generator, preserving signal
  flow semantics. This switch is not a general removal of surrounding DSP.

Enabled behaviour retains the previous Debug definition after preprocessing,
including its members, bases, virtual interface and capture lifecycle. Sound
remains an Effect. This does not redesign Studio's debug architecture, harden
enabled capture against missing sessions, or certify binary compatibility with
every existing DLL. Raw public-field manipulation and the separate Graph/Console
APIs are not disabled by this switch. Existing buffer allocation is retained.

## Reproduce / validate

`prepare.py` builds this prototype from the reviewed UE-integration snapshot;
it does not modify production or incorporate subsequent working-header edits.

```powershell
python experiments/klang/debug-guard/prepare.py
python tests/klang/check_debug_guard.py --header experiments/klang/debug-guard/klang.h
```

The runner checks default-off, Studio-on, explicit-off-in-Studio and explicit-on
outside Studio in Release and Debug. Run with MSVC or
`--compiler build/toolchains/klang14/klang.exe --driver gnu` (Studio DLL directory
on PATH). It tests mono/stereo capture and audio, transparent inline taps,
console/profiling, long sessionless disabled rendering, and concurrent disabled
writes through both Debug and its Input base. Results record object sizes and
source/header hashes under `build/debug-guard/`.

Model regression checks use unchanged Compressor, both PingPong examples,
RM, Tremolo and Flanger at 44.1/48 kHz. Reports and original-level WAVs are in
`build/debug-guard/models/inspection/index.html`.
