#pragma once

// Andy Farnell, Designing Sound (2010), chapter 29 bell studies.
#include "telephonebell.k"
#include "../common.h"

namespace farnell {
/** @brief Earlier BELL constructions, each preserving its distinct source patch.
 * @code
 * auto* bell = new farnell::BellStudies::Telephone;
 * processor.attach(bell);
 * bell->trigger();
 * @endcode
 * These are separate studies, not presets of the final TelephoneBell model.
 */
struct BellStudies {
    // Three decaying partials for A2, group and testgroup references.
    struct Partials : Sound {
        pd::osc oscillator[3];
        pd::vline envelope[3];
        param amplitude[3]{}, gain = .3333f;
        int count = 3;
        void partial(int i, param hz, param amp, param ms, double at = -1) {
            oscillator[i].set(hz);
            amplitude[i] = amp;
            if (at >= 0) envelope[i].messageTime(at * 1000 / float(fs));
            envelope[i].set(1);
            envelope[i].set(0, ms);
        }
        void trigger() {
            partial(0,521,.7f,800);
            partial(1,732,.45f,800);
            partial(2,934,.25f,800);
        }
        void process() override {
            out = 0;
            for (int i = 0; i < count; ++i) {
                out += oscillator[i] * amplitude[i] * (signal(envelope[i]) ^ 2);
            }
            out *= gain;
        }
    };

    // A3 overtone groups with the source's double-burst telephone cadence.
    struct Ratios : Sound {
        Partials group[3];
        double messageOffset = 0;
        double frame = 0, nextBurst = 1e30, nextStrike = 1e30;
        int beat = 0, strikeCount = 0;
        bool ringing = false;
        void set(param on) override {
            ringing = on != 0;
            nextBurst = ringing ? frame + messageOffset : 1e30;
        }
        void strike(double at) {
            static constexpr constant ratio[3][3] = {{1,3.3,1.0/6},{5,6.5,7.5},{11,12,15}};
            static constexpr constant amp[3][3] = {{.05,.05,.05},{.3,.2,.1},{.3,.2,.1}};
            static constexpr constant ms[] = {600,150,75};
            for (int i = 0; i < 3; ++i) {
                if (i == 1) continue; // The middle group has no output connection upstream.
                group[i].gain = .5f;
                for (int j = 0; j < 3; ++j) group[i].partial(j,865 * ratio[i][j],amp[i][j],ms[i],at);
            }
        }
        void process() override {
            while (std::min(nextBurst,nextStrike) < frame + 1) {
                if (nextBurst <= nextStrike) {
                    const int step = beat++ % 15;
                    if (step == 0 || step == 4) {
                        nextStrike = nextBurst + .012 * float(fs);
                        strikeCount = 1;
                    }
                    nextBurst += .3 * float(fs);
                } else {
                    if (strikeCount % 2) strike(nextStrike); // Only ringer outlet 1 is connected upstream.
                    nextStrike = ++strikeCount > 13 ? 1e30 : nextStrike + .06 * float(fs);
                }
            }
            (group[0] + group[2]) * .33333f >> out; // Middle A2 group is disconnected in A3-bell-ratio.pd.
            ++frame;
        }
    };

    // A4 bell: mixed envelope powers, noise strike and delayed body.
    struct Telephone : Sound {
        pd::osc oscillator[13];
        pd::line envelope[5];
        pd::noise noise;
        pd::bpf material[3];
        pd::hip highpass;
        SampleDelay body;
        Frequency fundamental = 650;
        void set(param hz) override { fundamental = hz; }
        void prepare() override {
            static constexpr constant ratio[] = {.5001,1,2.002,9.6,3,2.4999,11,2.571,3,6.242,12.483,16,24};
            for (int i = 0; i < 13; ++i) oscillator[i].set(fundamental * ratio[i]);
            material[0].set(1243,12);
            material[1].set(287,12);
            material[2].set(431,12);
            body.set(int(float(fs) * .059f + .5f));
            highpass.set(10);
        }
        void trigger() {
            const float duration[] = {720,540,300,600.0f/7,600.0f/9};
            for (int i = 0; i < 5; ++i) {
                envelope[i].set(1);
                envelope[i].set(0,duration[i]);
            }
        }
        void process() override {
            signal hum = (.1f * oscillator[0] + oscillator[1]) * .02f * envelope[0];
            signal primary = (.2f * oscillator[2] + .5f * oscillator[3] + .1f * oscillator[4])
                           * .04f * envelope[1];
            signal secondary = (.1f * oscillator[5] + .2f * oscillator[6] + .1f * oscillator[7])
                             * .2f * (signal(envelope[2]) ^ 4);
            signal circular = (.1f * oscillator[8] + oscillator[9] + oscillator[10])
                            * .05f * (signal(envelope[3]) ^ 2);
            signal strike = (oscillator[11] + oscillator[12] + noise) * .03f * (signal(envelope[4]) ^ 4);
            signal delayed = std::clamp(float(body.read()),-.2f,.5f);
            signal feedback = ((delayed >> material[0]) + (delayed >> material[1]) + (delayed >> material[2])) * .4f;
            body.write(strike + feedback);
            std::clamp(float((hum+primary+secondary+circular+strike)*.9f + feedback*4),-.9f,.9f) >> highpass >> out;
        }
    };
};
}
