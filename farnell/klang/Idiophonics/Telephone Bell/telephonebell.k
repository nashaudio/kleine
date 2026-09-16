#pragma once

// Andy Farnell, Designing Sound (2010), chapter 29, practical 6.
// Figures 29.7-29.15.
// Attribution/licensing: ../../../../README.md.
#include <klang/pd.h>

namespace farnell {
using namespace klang::optimised;

/** @brief Two alternating inharmonic bells, noise hammer and resonant casing.
 * @code
 * auto* bell = new farnell::TelephoneBell;
 * processor.attach(bell);
 * bell->set(1); // Start the 60 ms striker; set(0) stops, preserving the tail.
 * @endcode
 * strike() fires one alternating bell. Tuning 650/653 Hz, strength 1 and 2000 ms
 * base decay are fixed by the PD messages. Set klang::fs before construction.
 */
struct TelephoneBell : Sound {
    // Finite linear decay helper, not a complete vline~ port.
    struct Decay : Generator {
        double remaining = 0, length = 1;
        void trigger(param ms) { remaining = length = std::max(1.0, double(float(ms)) * float(fs) / 1000); }
        void process() override {
            remaining = std::max(0.0, remaining - 1);
            out = float(remaining / length);
        }
    } hammer;

    // Integer sample delay for explicit casing feedback paths.
    struct Delay {
        std::vector<signal> samples;
        int position = 0;
        void set(param count) {
            samples.assign(std::max(1, int(count)), 0);
            position = 0;
        }
        signal read() const { return samples[position]; }
        void write(signal value) {
            samples[position] = value;
            position = (position + 1) % int(samples.size());
        }
    };

    // Five inharmonic partial groups sharing strike controls.
    struct Bell : Generator {
        // Three partials under a shared squared decay envelope.
        struct Group : Generator {
            pd::osc partial[3];
            param amplitude[3]{};
            Decay envelope;

            void strike(param fundamental, param strength, param decayMs,
                        const constant (&ratio)[3], const constant (&gain)[3]) {
                for (int i = 0; i < 3; ++i) {
                    partial[i].set(fundamental * ratio[i]);
                    amplitude[i] = strength * gain[i];
                }
                envelope.trigger(decayMs);
            }

            void process() override {
                signal mix = 0;
                for (int i = 0; i < 3; ++i) mix += partial[i] * amplitude[i];
                mix * 0.333f * (signal(envelope) ^ 2) >> out;
            }
        } group[5];

        void strike(param fundamental, param strength, param decayMs) {
            // Hum/fundamental, primary, secondary, circular and strike modes.
            static constexpr constant ratio[5][3] = {
                {0.501, 1, 0.7}, {2.002, 3, 9.6}, {2.49, 11, 2.571},
                {3.05, 6.242, 12.49}, {13, 16, 24}
            };
            static constexpr constant gain[5][3] = {
                {0.002, 0.02, 0.001}, {0.008, 0.02, 0.004},
                {0.02, 0.04, 0.02}, {0.005, 0.05, 0.05}, {0.02, 0.03, 0.04}
            };
            static constexpr constant decay[5] = {1.2, 0.9, 0.25, 0.14, 0.07};
            for (int i = 0; i < 5; ++i)
                group[i].strike(fundamental, strength, decayMs * decay[i], ratio[i], gain[i]);
        }

        void process() override {
            out = 0;
            for (auto& part : group) out += part;
        }
    } bell[2];

    // Clipped material modes and the 0.77/0.88 ms casing delay paths.
    struct Casing : Modifier {
        Delay width, length;
        pd::bpf material[3];
        Casing() {
            width.set(int(float(fs) * 0.00077f + 0.5f));
            length.set(int(float(fs) * 0.00088f + 0.5f));
            material[0].set(1243, 12);
            material[1].set(287, 12);
            material[2].set(431, 12);
        }
        void process() override {
            signal across = width.read();
            out = length.read();
            signal resonances = (across >> material[0]) + (across >> material[1]) + (across >> material[2]);
            width.write(in + 0.7f * out);
            length.write(in + std::clamp(float(resonances), -0.3f, 0.3f));
        }
    } casing;

    pd::noise noise;
    pd::metro metro;
    int nextBell = 0;

    void set(param on) override { metro = on; }

    void strike() {
        bell[nextBell].strike(nextBell ? 653 : 650, 1, 2000);
        hammer.trigger(10);
        nextBell ^= 1;
    }

    void process() override {
        if (metro(60)) strike();
        signal direct = bell[0] + bell[1] + (signal(hammer) ^ 4) * noise * 0.1f;
        direct + (direct >> casing) >> out;
    }
};
}
