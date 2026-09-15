#pragma once

// Andy Farnell, Designing Sound (2010), chapter 28, figures 28.6-28.10.
// Attribution and licensing: ../../../../README.md.
#include <klang/pd.h>
#include <vector>

namespace farnell {
using namespace klang::optimised;

// A swept electronic siren, distorted horn and three-building echo environment.
struct Police : Sound {
    // Piecewise oscillator; legacy selects the book's pre-0.41 pow~ inlet order.
    struct LogOsc : Generator {
        pd::phasor ramp;
        bool legacy = false, graph = false, triangle = false, inverted = false;
        void set(param hz) override { ramp.set(hz); }
        void process() override {
            signal phase = ramp;
            if (triangle) { out = (phase - min(phase * 2, 1)) * 4 + 1; return; }
            signal position = phase * 2;
            signal rise = 1 - min(position, 1), fall = 2 - max(position, 1);
            constant e = 2.71828f;
            signal a = legacy ? std::pow(float(e), float(rise)) : std::pow(float(rise), float(e));
            signal b = legacy ? std::pow(float(e), float(fall)) : std::pow(float(fall), float(e));
            out = graph ? ((1 - a) + b) * 2 - 3 : ((1 - a) + std::sqrt(float(b))) - 1;
            if (inverted) out = -out;
        }
    } sweep, oscillator;

    // Diaphragm clipping followed by the plastic horn's strongest resonance.
    struct Horn : Modifier {
        pd::bpf resonance;
        void set(param hz = 1500, param q = 4) override { resonance.set(hz, q); }
        void process() override { std::clamp(float(in), -0.2f, 0.2f) >> resonance >> out; }
    } horn;

    // Fixed building echoes with shared feedback; explicit routing delays are calibrated by impulse.
    struct Environment : Modifier {
        // Read-before-write storage for one fixed echo path.
        struct Delay {
            std::vector<float> samples;
            int position = 0;
            // The original throw~/catch~ feedback routing adds one DSP block.
            explicit Delay(float milliseconds) : samples(std::max(1, int((float(fs) / 1000) * milliseconds + 0.5f)) + 64) {}
            float read() const { return samples[position]; }
            void write(float value) { samples[position] = value; if (++position == int(samples.size())) position = 0; }
        } a{165}, b{121}, c{33};
        void process() override {
            signal echo = (a.read() + b.read() + c.read()) * 0.1f;
            signal send = in + echo;
            a.write(send); b.write(send); c.write(send);
            out = in * 0.7f + echo * 0.5f;
        }
    } environment;

    Frequency rate = 0.1f, base = 300, depth = 800;
    bool echoes = true;
    void set(param hz = 0.1f, param historical = 0, param ambience = 1) override {
        rate = hz; sweep.legacy = oscillator.legacy = historical >= 0.5f; echoes = ambience >= 0.5f;
    }
    void prepare() override { sweep.set(rate); horn.set(); }
    void process() override {
        signal modulation = sweep;
        oscillator.set(modulation * depth + base);
        signal wave = oscillator;
        signal sound = wave >> horn;
        if (echoes) sound = sound >> environment;
        sound >> out;
    }
};
}
