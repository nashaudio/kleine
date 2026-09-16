#pragma once

// Andy Farnell, Designing Sound (2010), chapter 32, figures 32.3-32.6.
#include "../common.h"

namespace farnell {
/** @brief Door stick-slip impulses filtered by wood modes and panel reflections.
 * @code
 * auto* door = new farnell::Creaking;
 * processor.attach(door);
 * door->set(0.7f); // Force 0..1; source patch slews it over 100 ms.
 * @endcode
 * Vary force over time to perform a creak. Performance contours belong to the
 * caller; the model has no fitted recording contour or added gain control.
 */
struct Creaking : Sound {
    // One dfbef delay with millisecond duration and feedback gain.
    struct Reflection : Modifier {
        SampleDelay delay;
        param milliseconds = 4.52f, amount = .05f;
        void set(param ms) override { set(ms, .05f); }
        void set(param ms, param gain) override {
            milliseconds = ms;
            amount = gain;
        }
        void prepare() {
            delay.set(int(float(milliseconds) * (float(fs) / 1000) + .5f));
        }
        void process() override {
            out = delay.read();
            delay.write(in + out * amount);
        }
    };
    // Eight parallel dfbef reflections and a 125 Hz highpass.
    struct Panel : Modifier {
        Reflection reflection[8];
        pd::hip highpass;
        void prepare() {
            static constexpr constant ms[] = {4.52, 5.06, 6.27, 8, 5.48, 7.14, 10.12, 16};
            for (int i = 0; i < 8; ++i) {
                reflection[i].set(ms[i]);
                reflection[i].prepare();
            }
            highpass.set(125);
        }
        void process() override {
            signal sum = 0;
            for (auto& r : reflection) sum += in >> r;
            sum >> highpass >> out;
        }
    } panel;
    // Six broad wood resonances plus the source's direct path.
    struct Wood : Modifier {
        pd::bpf mode[6];
        void prepare() {
            static constexpr constant hz[] = {62.5,125,250,395,560,790};
            static constexpr constant q[] = {1,1,2,2,3,3};
            for (int i = 0; i < 6; ++i) mode[i].set(hz[i], q[i]);
        }
        void process() override {
            out = in * .2f;
            for (auto& m : mode) out += in >> m;
        }
    } wood;
    // The stickslip subpatch: slew force, time slips, square short pulses.
    struct StickSlip : Generator {
        pd::line line;
        pd::vline pulse;
        ControlRandom random;
        double frame = 0, nextSlip = 1e30, previousSlip = 0;
        bool moving = false;
        void set(param pressure) override {
            line.set(std::clamp(float(pressure), 0.0f, 1.0f), 100, 20);
        }
        void process() override {
            signal force = line;
            if (line.updated && (force > .3f) != moving) {
                moving = force > .3f;
                nextSlip = moving ? frame : 1e30;
            }
            // Pending metro outlet-feedback/timer port: its bang changes the next
            // interval before rearming. Keep that order, without block lookahead.
            while (nextSlip < frame + 1) {
                const float elapsed = float((nextSlip - previousSlip) * 1000 / float(fs));
                const float duration = std::sqrt(std::min(elapsed, 100.0f) / 100);
                pulse.messageTime(nextSlip * 1000 / float(fs));
                pulse.set(std::sqrt(duration + .1f));
                pulse.set(0, duration);
                previousSlip = nextSlip;
                const float inverse = 1 - force;
                const float interval = (random.next() / 1000.0f) * (inverse * 6) + (inverse * 60 + 3);
                nextSlip += double(interval) * float(fs) / 1000;
            }
            (signal(pulse) ^ 2) >> out;
            ++frame;
        }
    } stickslip;

    void set(param pressure) override { stickslip.set(pressure); }
    void prepare() override {
        wood.prepare();
        panel.prepare();
    }
    void process() override {
        stickslip >> wood >> panel >> out;
        out *= .2f;
    }
};
}
