#pragma once

#include "../../farnell/klang/Artificial Sounds/DTMF Tones/dtmftones.k"

namespace validation {
using namespace farnell;
// Discarded gain-0.3 dialler, retained solely to reproduce historical comparisons.
struct DTMFLevel03 : DTMFTones {
    void process() override {
        if (del()) gate(0);
        (high + low) * envelope >> highpass >> out;
        out *= .3f;
    }
};
}
