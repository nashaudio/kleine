#pragma once

// Andy Farnell, Designing Sound (2010), chapter 25, practical 2.
// Figures 25.2-25.6; attribution/licensing: ../../../../README.md.
#include <klang/pd.h>

namespace farnell {
using namespace klang::optimised;

/** @brief Dial, busy, ringback and pulse-dial telephone signals.
 * @code
 * auto* sound = new farnell::PhoneTones(farnell::PhoneTones::Ringback);
 * processor.attach(sound);
 * @endcode
 * The constructor chooses the tone. Pulse mode accepts dial(count).
 */
struct PhoneTones : Sound {
    // Clipped telephone wire/handset; set(1) selects the pulse-dial line.
    struct Line : Modifier {
        pd::bpf wire, speaker;
        pd::hip highpass[2];
        bool pulse = false;

        void set(param pulseDial) override {
            pulse = pulseDial >= 0.5f;
            wire.set(2000, 12);
            speaker.set(400, pulse ? 7 : 3);
            for (auto& filter : highpass) filter.set(90);
        }

        void process() override {
            signal line = std::clamp(float(in), -0.9f, 0.9f) >> wire;
            signal handset = line * (pulse ? 1.2f : 0.5f) >> speaker;
            if (!pulse) handset += std::clamp(float(line), -0.4f, 0.4f) * 0.15f;
            handset >> highpass[0] >> highpass[1] >> out;
        }
    } line;

    enum Tone { Dial, DialLine, Busy, Ringback, Pulse };
    pd::osc osc[2], cadence;
    pd::metro metro;
    pd::lop smoothing;
    int count = 0, pulseElapsed = 0, pulseCount = 0;
    signal gate = 0;
    const Tone tone;
    explicit PhoneTones(Tone selection = Dial) : tone(selection) {
        if (tone == Ringback) metro = 1; // loadbang -> metro 1000.
    }

    void prepare() override {
        smoothing.set(100);
        line.set(tone == Pulse);
    }

    // The patch supplies a pulse count; each contact lasts 40 ms in a 100 ms cycle.
    void dial(int digit) {
        pulseCount = std::max(0, digit);
        pulseElapsed = 0;
    }

    void process() override {
        const int rate = int(float(fs));
        switch (tone) {
        case Dial:
            (osc[0](350) + osc[1](450)) * 0.125f >> out;
            break;
        case DialLine:
            (osc[0](350) + osc[1](450)) >> line >> out;
            break;
        case Busy: {
            signal amplitude = std::clamp(float(signal(cadence(2))) * 10000, 0.0f, 1.0f);
            amplitude = amplitude >> smoothing;
            (osc[0](480) + osc[1](620)) * amplitude * 0.1f >> out;
            break;
        }
        case Ringback: {
            // Keep the patch's 3 s off/on cadence; the book table says 4/2.
            if (metro(1000)) gate = count++ % 6 > 2;
            (osc[0](480) + osc[1](440)) * (gate >> smoothing) >> line >> out;
            break;
        }
        case Pulse: {
            const int period = rate / 10, width = rate * 40 / 1000;
            signal contact = float(pulseElapsed < pulseCount * period && pulseElapsed % period < width);
            0.8f * (contact >> line) >> out;
            if (pulseElapsed < pulseCount * period) ++pulseElapsed;
            break;
        }
        }
    }
};
}
