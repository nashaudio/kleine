#pragma once
#include "oscillators.k"

namespace farnell::variants {

// Exponential siren sweep and carrier through the horn and building echoes.
struct PoliceExponential : Sound {
    ExponentialOsc sweep, oscillator;
    Police::Horn horn;
    Police::Environment environment;
    Frequency rate = 0.1f;

    void set(param hz) override { rate = hz; }
    void prepare() override { horn.set(); }
    void process() override {
        signal(oscillator(signal(sweep(rate)) * 800 + 300)) >> horn >> environment >> out;
    }
};
}
