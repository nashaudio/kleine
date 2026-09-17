# Initial UE header integration

The selected additions and subsequent reviewed follow-ups are promoted to
`include/klang.h`, subsequently versioned 0.7.10. The initial prototype in this directory
remains pinned to its historical first-integration snapshot.

`python experiments/klang/ue-integration/prepare.py` reconstructs the prototype
from the accepted OSM header archived in `osm-improved.zip`. It writes only this
directory's `klang.h`. `interleaved.inc` contains the adapted buffer addition.
It does not merge future edits to the working header: do not copy a regenerated
prototype over subsequent production work without reviewing the diff.

See [integration and validation](../../../klang/UE-INTEGRATION.md) and the
[reusable model inspection commands](../../../tests/klang/README.md#render-compare-and-inspect-examplessounds).

Focused contracts:

```powershell
python tests/klang/check_ue_additions.py --header experiments/klang/ue-integration/klang.h --output build/ue-integration/contracts
```

The runner tests frame layout/indexing, borrowed copies, frame arithmetic,
interpolation, interleaved Effect/Sound processing, Phasor equivalence to Basic
Saw, DCF setter dispatch, absolute-level measurement and linking two translation
units. It uses `/fp:fast /MT` (`/MTd` Debug). Supply `--debug`, or
`--compiler build/toolchains/klang14/klang.exe --driver gnu`, as appropriate.

Like the UE source, Phasor inherits the existing Basic oscillator limitations:
negative phase increments do not wrap backwards, relative phase is unused by
Basic Saw, and increments at least one turn per sample are not advanced. Tests
check fidelity for negative frequencies but restrict the 0..1 range contract to
ordinary nonnegative frequencies and valid initial phase. This is not the fixed
OSM and does not claim the OSM's signed/multi-cycle capability.

Interleaved buffers use `signals<N>` frames; this requires at least two channels
with the existing channel type layout. Use the existing mono `klang::buffer`
for one channel. Borrowed host storage is bounded by the supplied frame count;
only owned integer indexing wraps at the rounded allocation capacity. The owner
must outlive borrowed copies. Stereo Effect/Sound gain a host-buffer processing
overload; the separate Synth note renderer has not gained an interleaved API.
