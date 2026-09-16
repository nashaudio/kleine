#pragma once

// Andy Farnell, Designing Sound (2010), chapter 24, figure 24.4.
// Attribution and licensing: ../../../../README.md.
#include <klang/pd.h>

namespace farnell {
using namespace klang::optimised;

/** @brief Alternating 2500 Hz pedestrian beeps, initially silent, at gain 0.2.
 * @code
 * auto* sound = new farnell::Pedestrians;
 * processor.attach(sound);
 * sound->set(1); // Nonzero starts/restarts; zero stops the metro.
 * @endcode
 * Stopping leaves the gate at its last value, as the PD toggle/metro does.
 */
struct Pedestrians : Sound {
    pd::osc osc;
    pd::metro metro;
    signal gate = 0;
    int count = 0;

    void set(param on) override {
        metro = on;
    }
    void process() override {
        if (metro(100))
            gate = count++ % 2;
        osc(2500) * gate * 0.2f >> out;
    }
};
}
