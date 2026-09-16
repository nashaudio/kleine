#pragma once

#include "../boing.k"

namespace farnell::variants {
// Struck bar with a fifteenth-power pitch envelope.
struct BoingPower : Boing {
    void process() override {
        signal e = signal(pitch) ^ 4;
        signal modulation = std::pow(float(e), 15.0f);
        signal hz = frequency + vibrato(5) * depth;
        signal modes = signal(phase(-(modulation * 6 + hz / 4))) >> clamped;
        signal resonated = ((phase.out ^ 2) * modes) * noise
                         >> lowpass >> free(modulation * (hz * .1f) + hz);
        (resonated + modes * .2f) >> highpass >> out;
        out *= 4;
    }
};
}
