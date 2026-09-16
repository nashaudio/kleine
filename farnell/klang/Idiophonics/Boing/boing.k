#pragma once

// Andy Farnell, Designing Sound (2010), chapter 33, figures 33.2-33.3.
#include <klang/pd.h>

namespace farnell {
using namespace klang::optimised;
/** @brief Struck flexible bar mixing clamped modes and noise-excited free modes.
 * @code
 * auto* bar = new farnell::Boing;
 * processor.attach(bar);
 * bar->set(426, 6); // Fundamental Hz and vibrato depth Hz; rate is fixed at 5 Hz.
 * bar->trigger();
 * @endcode
 * set(hz) retains the existing vibrato depth.
 */
struct Boing : Sound {
    // Four clamped modal cosines from phase in cycles, under a fourth-power decay.
    struct Clamped : Modifier {
        pd::vline decay;
        void trigger() {
            decay.set(1);
            decay.set(0, 1500);
        }
        void process() override {
            static constexpr constant ratio[] = {1, 6.267, 17.55, 34.39};
            static constexpr constant gain[] = {.5, .25, .125, .0625};
            signal sum = 0;
            for (int i = 0; i < 4; ++i) sum += pd::cos::lookup(in * ratio[i]) * gain[i];
            sum * (signal(decay) ^ 4) >> out;
        }
    } clamped;
    // Six free-bar vcf~ resonances, with audio-rate frequency modulation.
    struct Free : Modifier {
        pd::vcf mode[6];
        param frequency = 426, resonance = 80;
        void set(param hz) override { frequency = hz; }
        void set(param hz, param q) override {
            frequency = hz;
            resonance = q;
        }
        void process() override {
            static constexpr constant ratio[] = {1, 2.7565, 5.40392, 8.93295, 13.3443, 18.6379};
            out = 0;
            for (int i = 0; i < 6; ++i) out += in >> mode[i](frequency * ratio[i], resonance);
        }
    } free;
    pd::osc vibrato;
    pd::phasor phase;
    pd::noise noise;
    pd::lop lowpass;
    pd::hip highpass;
    pd::vline pitch;
    Frequency frequency = 426;
    param depth = 6;
    void set(param hz) override { frequency = hz; }
    void set(param hz, param vibratoDepth) override {
        frequency = hz;
        depth = vibratoDepth;
    }
    void trigger() {
        pitch.set(1);
        pitch.set(0, 7000);
        clamped.trigger();
        phase.phase(0);
    }
    void prepare() override {
        lowpass.set(4000);
        highpass.set(90);
    }
    void process() override {
        signal e = pitch;
        signal modulation = 3 * (e ^ 64); // env16p actually raises to 64.
        signal hz = frequency + vibrato(5) * depth;
        signal modes = signal(phase(-(modulation * 6 + hz / 4))) >> clamped;
        signal resonated = ((phase.out ^ 2) * modes) * noise
                         >> lowpass >> free(modulation * (hz * .1f) + hz);
        (resonated + modes * .2f) >> highpass >> out;
        out *= 4;
    }
};
}
