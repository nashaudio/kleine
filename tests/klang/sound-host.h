// Test-only stand-ins for dependencies supplied by the game host.
// This is not an Unreal build and does not establish FMath equivalence.
#pragma once
#include <klang.h>

#ifdef UPSTREAM_CORE
namespace klang {
    using Sound = Effect;
    using byte = unsigned char;
    namespace Stereo { using Sound = Effect; }
}
#endif

#ifdef HOST_TANH
struct FMath {
    static float Tanh(float x) { return std::tanh(x); }
};
#endif

#ifdef HOST_PHASOR
// The commented definition supplied at the top of sounds/Bicycle.h.
struct Phasor : klang::basic::Saw {
    void process() override {
        Saw::process();
        out = out * 0.5 + 0.5;
    }
};
#endif

#ifdef HOST_REVERB
// Train's game Audio/Reverb.k matches this Reverb2 source apart from line endings.
// Keep the dependency explicit: examples/Reverb.k declares a different object.
#include "../../examples/Delay/Reverb2.k"
#endif
