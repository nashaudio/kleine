#pragma once

// Andy Farnell, Designing Sound (2010), chapter 28, figures 28.6-28.10.
// Attribution and licensing: ../../../../README.md.
#include <klang/pd.h>
#include <vector>

namespace farnell {
using namespace klang::optimised;

/** @brief Swept siren through a clipped horn and three building echoes.
 * @code
 * auto* siren = new farnell::Police;
 * processor.attach(siren);
 * siren->set(0.1f); // Sweep Hz; 3 Hz selects the faster performance.
 * @endcode
 * Carrier base/depth, horn and echo parameters remain the patch constants.
 */
struct Police : Sound {
    // Piecewise power oscillator with a square-root falling segment.
    struct LogOsc : Generator {
        pd::phasor ramp;
        void set(param hz) override { ramp.set(hz); }
        void process() override {
            signal phase = ramp;
            signal position = phase * 2;
            signal rise = 1 - min(position, 1), fall = 2 - max(position, 1);
            constant e = 2.71828f;
            signal a = std::pow(float(rise), float(e));
            signal b = std::pow(float(fall), float(e));
            out = ((1 - a) + std::sqrt(float(b))) - 1;
        }
    } sweep, oscillator;

    // Clipped diaphragm and plastic-horn resonance (1500 Hz, Q4).
    struct Horn : Modifier {
        pd::bpf resonance;
        void set(param hz = 1500, param q = 4) override { resonance.set(hz, q); }
        void process() override { std::clamp(float(in), -0.2f, 0.2f) >> resonance >> out; }
    } horn;

    // Parallel 165/121/33 ms building echoes with shared feedback.
    struct Environment : Modifier {
        // One fixed, read-before-write echo path; duration is milliseconds.
        struct Delay {
            std::vector<float> samples;
            int position = 0;
            explicit Delay(float milliseconds) : samples(std::max(1, int((float(fs) / 1000) * milliseconds + 0.5f))) {}
            float read() const { return samples[position]; }
            void write(float value) {
                samples[position] = value;
                if (++position == int(samples.size())) position = 0;
            }
        } a{165}, b{121}, c{33};
        void process() override {
            signal echo = (a.read() + b.read() + c.read()) * 0.1f;
            signal send = in + echo;
            a.write(send);
            b.write(send);
            c.write(send);
            out = in * 0.7f + echo * 0.5f;
        }
    } environment;

    Frequency rate = 0.1f;
    void set(param hz) override { rate = hz; }
    void prepare() override {
        horn.set();
    }
    void process() override {
        signal(oscillator(signal(sweep(rate)) * 800 + 300)) >> horn >> environment >> out;
    }
};
}
