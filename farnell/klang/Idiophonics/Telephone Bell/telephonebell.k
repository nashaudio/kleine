#pragma once

// Andy Farnell, Designing Sound (2010), chapter 29, practical 6.
// Figures 29.7-29.15; BELL/striker.pd equals the website telephonebell.pd.
// Attribution/licensing: ../../../../README.md.
#include <klang/pd.h>

namespace farnell {
using namespace klang::optimised;

// Two alternating inharmonic bells with a noise hammer and resonant casing.
struct TelephoneBell : Sound {
    // Finite linear decay used by the struck partials and hammer (vline~ ramp).
    struct Decay : Generator {
        double remaining = 0, length = 1;
        void trigger(param ms) { remaining = length = std::max(1.0, double(float(ms)) * float(fs) / 1000); }
        void process() override {
            remaining = std::max(0.0, remaining - 1);
            out = float(remaining / length);
        }
    } hammer;

    // Read-before-write integer delay for the casing's explicit feedback paths.
    struct Delay {
        std::vector<signal> samples;
        int position = 0;
        void set(param count) { samples.assign(std::max(1, int(count)), 0); position = 0; }
        signal read() const { return samples[position]; }
        void write(signal value) { samples[position] = value; position = (position + 1) % int(samples.size()); }
    } casingRoute;

    // Five partial groups sharing one bell's tuning, strike and decay controls.
    struct Bell : Generator {
        // Three inharmonic partials sharing a squared decay envelope.
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
                signal shape = envelope;
                mix * 0.333f * shape * shape >> out;
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

    // Clipped material resonances and delays, preserving Pd's 64-sample schedule.
    struct Casing : Modifier {
        Delay width, length, materialReturn;
        pd::bpf material[3];
        Casing() {
            width.set(std::max(64, int(float(fs) * 0.00077f + 0.5f)));
            length.set(int(float(fs) * 0.00088f + 0.5f));
            materialReturn.set(64);
            material[0].set(1243, 12);
            material[1].set(287, 12);
            material[2].set(431, 12);
        }
        void process() override {
            signal across = width.read();
            out = length.read();
            signal resonances = (across >> material[0]) + (across >> material[1]) + (across >> material[2]);
            width.write(in + 0.7f * out);
            length.write(in + materialReturn.read());
            materialReturn.write(std::clamp(float(resonances), -0.3f, 0.3f));
        }
    } casing;

    pd::noise noise;
    bool ringing = false;
    int nextBell = 0, untilStrike = 0;
    param casingMix = 1;

    TelephoneBell() {
        casingRoute.set(128); // Two enclosing send~/receive~ block delays.
        controls = {
            klang::Dial("Ring", 0, 1, 0),
            klang::Dial("Fundamental", 100, 1000, 650),
            klang::Dial("Detune", 0, 30, 3),
            klang::Dial("Strength", 0, 2, 1),
            klang::Dial("Decay (ms)", 10, 4000, 2000),
            klang::Dial("Strike interval (ms)", 10, 200, 60)
        };
    }

    void ring(bool enabled) {
        controls.set(enabled ? 1.0f : 0.0f, float(controls[1]), float(controls[2]),
                     float(controls[3]), float(controls[4]), float(controls[5]));
    }

    void prepare() override {
        const bool enabled = float(controls[0]) >= 0.5f;
        if (enabled && !ringing) untilStrike = 0;
        ringing = enabled;
    }

    void strike() {
        Frequency fundamental = controls[1];
        param detune = controls[2];
        bell[nextBell].strike(fundamental + nextBell * detune, controls[3], controls[4]);
        hammer.trigger(10);
        nextBell ^= 1;
    }

    void process() override {
        if (ringing) {
            if (!untilStrike) {
                strike();
                untilStrike = std::max(1, int(double(float(controls[5])) * float(fs) / 1000));
            }
            --untilStrike;
        }
        signal envelope = hammer;
        signal click = noise * envelope * envelope * envelope * envelope * 0.1f;
        signal direct = bell[0] + bell[1] + click;
        signal resonated = casingRoute.read() >> casing;
        casingRoute.write(direct);
        direct + casingMix * resonated >> out;
    }
};
}
