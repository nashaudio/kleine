#pragma once

#include "../bouncing.k"

namespace farnell::variants {
// Bouncing ball with a fractional-power pitch envelope.
struct BouncingPower : Bouncing {
    void process() override {
        signal height = line;
        if (metro(height * 300)) {
            impact.set(1, 1);
            impact.set(0, height * 200, 1);
        }
        signal e = impact, squared = e ^ 2;
        signal shape = std::pow(std::max(0.0f, float(e)), 2.71828f);
        signal hz = shape * (height * 130) + 80 + (height * 70) * modulation(120);
        squared * carrier(hz) * height * .2f >> out;
    }
};
}
