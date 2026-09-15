#pragma once

// Andy Farnell, Designing Sound (2010), chapter 24, figure 24.4.
// Attribution and licensing: ../../../../README.md.
#include <klang/pd.h>

namespace farnell {
using namespace klang::optimised;

// Alternating pedestrian-crossing beeps with the patch's initially silent half-cycle.
struct Pedestrians : Sound {
    pd::osc oscillator;
    Frequency frequency = 2500;
    param interval = 100, level = 0.2f;
    bool running = false, gate = false;
    int count = 0, frame = 0;
    double nextTick = 0;

    void set(param hz, param milliseconds = 100, param gain = 0.2f) override {
        frequency = hz; interval = max(milliseconds, 1); level = gain;
    }
    void start(bool enabled = true) {
        running = enabled;
        if (enabled) {
            gate = (count++ % 2) != 0;
            nextTick = frame + double(float(interval)) * float(fs) / 1000;
        }
    }
    void prepare() override { oscillator.set(frequency); }
    void process() override {
        // metro -> sig~ takes effect in the block containing the logical event.
        if (running && frame / 64 >= int(nextTick / 64)) {
            gate = (count++ % 2) != 0;
            nextTick += double(float(interval)) * float(fs) / 1000;
        }
        signal tone = oscillator;
        tone * (gate ? level : 0) >> out;
        ++frame;
    }
};
}
