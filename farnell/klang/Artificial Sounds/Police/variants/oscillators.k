#pragma once
#include "../police.k"

namespace farnell::variants {

// Piecewise exponential oscillator with a square-root falling segment.
struct ExponentialOsc : Police::LogOsc {
    void process() override {
        signal position = signal(ramp) * 2;
        signal rise = 1 - min(position, 1), fall = 2 - max(position, 1);
        constant e = 2.71828f;
        signal a = std::pow(float(e), float(rise));
        signal b = std::pow(float(e), float(fall));
        out = ((1 - a) + std::sqrt(float(b))) - 1;
    }
};

// Symmetric piecewise power oscillator.
struct PowerOsc : Police::LogOsc {
    void process() override {
        signal position = signal(ramp) * 2;
        signal rise = 1 - min(position, 1), fall = 2 - max(position, 1);
        constant e = 2.71828f;
        signal a = std::pow(float(rise), float(e));
        signal b = std::pow(float(fall), float(e));
        out = ((1 - a) + b) * 2 - 3;
    }
};

// Symmetric piecewise exponential oscillator.
struct SymmetricExponentialOsc : Police::LogOsc {
    void process() override {
        signal position = signal(ramp) * 2;
        signal rise = 1 - min(position, 1), fall = 2 - max(position, 1);
        constant e = 2.71828f;
        signal a = std::pow(float(e), float(rise));
        signal b = std::pow(float(e), float(fall));
        out = ((1 - a) + b) * 2 - 3;
    }
};

// Inverted symmetric exponential oscillator.
struct InvertedExponentialOsc : SymmetricExponentialOsc {
    void process() override {
        SymmetricExponentialOsc::process();
        out = -out;
    }
};

// Triangle oscillator formed from a folded phasor.
struct TriangleOsc : Police::LogOsc {
    void process() override {
        signal phase = ramp;
        out = (phase - min(phase * 2, 1)) * 4 + 1;
    }
};
}
