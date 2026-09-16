#pragma once

#include "../bouncing.k"

namespace farnell::variants {
// Bouncing ball with exponential pitch modulation.
struct BouncingExponential : Bouncing {
    void process() override {
        signal height = line;
        if (metro(height * 300)) {
            impact.set(1, 1);
            impact.set(0, height * 200, 1);
        }
        signal e = impact, squared = e ^ 2;
        signal shape = std::pow(2.71828f, float(e));
        signal hz = shape * (height * 130) + 80 + (height * 70) * modulation(120);
        squared * carrier(hz) * height * .2f >> out;
    }
};
}
