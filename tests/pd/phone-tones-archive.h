#pragma once
#include "../../farnell/klang/Artificial Sounds/Phone Tones/phonetones.k"

// Historical comparison fixtures; not candidates for the retained sound library.
namespace validation {
using namespace farnell;
// Original collection's 350/440 Hz dial tone.
struct Dial440 : PhoneTones {
    void process() override {
        (osc[0](350) + osc[1](440)) * .125f >> out;
    }
};

// Original collection's unsmoothed busy gate.
struct BusyUnsmoothed : PhoneTones {
    BusyUnsmoothed() : PhoneTones(Busy) {}
    void process() override {
        signal amplitude = std::clamp(float(signal(cadence(2))) * 10000, 0.0f, 1.0f);
        (osc[0](480) + osc[1](620)) * amplitude * .1f >> out;
    }
};

// Rejected duller ringback, retained solely to reproduce earlier comparisons.
struct RingbackHandset : PhoneTones {
    RingbackHandset() : PhoneTones(Ringback) {}
    void prepare() override {
        smoothing.set(100);
        line.set(1);
    }
};
}
