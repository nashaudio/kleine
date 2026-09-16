#pragma once

// Andy Farnell, Designing Sound (2010), chapter 31, figures 31.3-31.8.
// Rolling, struck-can and uneven-surface studies.
#include "../common.h"

namespace farnell {
/** @brief Rolling tin can: edge contacts and uneven ground excite four body modes.
 * @code
 * auto* can = new farnell::Rolling;
 * processor.attach(can);
 * can->trigger();
 * @endcode
 * set(1) selects the separate struck-can source; set(0) restores rolling.
 * trigger() either pushes the rolling model or strikes the can, at fixed patch gain.
 */
struct Rolling : Sound {
    // Four can modes, with source-specific weights and clipping.
    struct Can : Modifier {
        pd::bpf mode[4];
        param struck = 0;
        void prepare() {
            static constexpr constant hz[] = {359, 426, 1748, 3000};
            for (int i = 0; i < 4; ++i) mode[i].set(hz[i], 123);
        }
        void process() override {
            signal sum = (struck ? .4f : .2f) * (in >> mode[0]) + .3f * (in >> mode[1])
                       + (struck ? .2f : .3f) * (in >> mode[2]) + .2f * (in >> mode[3]);
            const float limit = struck ? .6f : .3f;
            std::clamp(float(sum * 1000), -limit, limit) * .4f >> out;
        }
    } can;

    // Weighted wrapping edge pattern differentiated into contacts.
    struct Edges : Generator {
        pd::phasor phase;
        pd::rzero difference;
        param speed = 0;
        void set(param hz) override { speed = hz; }
        void prepare() { difference.set(.9f); }
        void process() override {
            signal p = phase(speed), sum = 0;
            static constexpr constant ratio[] = {2, 3, 4.2, 5.6};
            static constexpr constant gain[] = {.1, .05, .02, .012};
            for (int i = 0; i < 4; ++i) {
                float v = p * ratio[i];
                sum += (v - std::floor(v)) * gain[i];
            }
            sum >> difference >> out;
        }
    } edges;

    // Noise sampled at irregular contacts, scaled by rolling speed.
    struct Ground : Generator {
        pd::noise noise;
        pd::phasor phase;
        pd::samphold hold;
        pd::rzero difference;
        param speed = 0;
        void set(param value) override { speed = value; }
        void prepare() { difference.set(.99f); }
        void process() override {
            signal n = noise;
            signal contact = max(n, 0) >> hold(n * .001f + phase(speed * 100)) >> difference;
            contact * .05f * speed >> out;
        }
    } ground;

    pd::vline push, impact;
    pd::lop inertia;
    pd::hip highpass;
    param struck = 0;
    void set(param struckCan) override {
        struck = struckCan;
        can.struck = struckCan;
    }
    void trigger() {
        if (struck) {
            impact.set(1);
            impact.set(0, 2);
        } else {
            push.set(0);
            push.set(1, 500);
        }
    }
    void prepare() override {
        inertia.set(.1f);
        highpass.set(200);
        can.prepare();
        edges.prepare();
        ground.prepare();
    }
    void process() override {
        signal excitation;
        if (struck) {
            excitation = (signal(impact) ^ 2) * .05f;
        } else {
            signal speed = pd::cos::lookup(.5f * push - .25f) >> inertia;
            speed *= 5;
            excitation = std::sqrt(std::max(0.0f, float(speed * .1f))) * (edges(speed) + ground(speed * .5f));
        }
        excitation >> highpass >> can >> out;
    }

    // Standalone uneven2 ground study and its three-mode can.
    struct Uneven : Sound {
        pd::noise noise;
        pd::lop smooth[3];
        pd::phasor phase;
        pd::samphold hold;
        pd::rzero difference;
        pd::bpf mode[3];
        pd::hip highpass;
        void prepare() {
            smooth[0].set(20);
            smooth[1].set(20);
            smooth[2].set(90);
            difference.set(.99f);
            highpass.set(400);
            mode[0].set(632,400);
            mode[1].set(759,400);
            mode[2].set(893,400);
        }
        void process() override {
            signal n = noise >> smooth[0] >> smooth[1];
            signal p = phase(max(n, 0) * 3340);
            signal ground = (((p - .5f) ^ 2) * -4 + 1) * .5f >> smooth[2];
            signal click = ground >> hold(p) >> difference;
            signal excitation = (ground + click * 12) * .03f;
            ((excitation >> mode[0]) + (excitation >> mode[1]) + (excitation >> mode[2]))
                * 500 >> highpass >> out;
        }
    };

    // Earlier uneven.pd: random linear gestures shaped into contact curves.
    struct Study : Sound {
        pd::vline position;
        ControlRandom values, times;
        param previous = 0, duration = 0;
        double frame = 0, next = 1e30, messageOffset = 0;
        void trigger() { next = frame + messageOffset; }
        void stop() { next = 1e30; }
        void process() override {
            while (next < frame + 1) {
                const float target = values.next() / 1000.0f;
                position.messageTime(next * 1000 / float(fs));
                position.set(previous);
                position.set(target, duration);
                previous = target;
                duration = times.next() / 23.0f;
                next += std::max(.001, double(float(duration)) * float(fs) / 1000);
            }
            (((signal(position) - .5f) ^ 2) * -4 + 1) * .5f * .1f >> out;
            ++frame;
        }
    };
};
}
