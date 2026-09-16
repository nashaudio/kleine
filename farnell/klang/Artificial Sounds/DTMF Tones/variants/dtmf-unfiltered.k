#pragma once

#include "../dtmftones.k"

namespace farnell::variants {
// Unfiltered DTMF pair with gain 0.125.
struct DTMFUnfiltered : DTMFTones {
    void process() override {
        if (del()) gate(0);
        (high + low) * envelope * .125f >> out;
    }
};
}
