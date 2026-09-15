#pragma once

// Andy Farnell, Designing Sound (2010), chapter 25, practical 2.
// Figures 25.2-25.6; attribution/licensing: ../../../../README.md.
#include <klang/pd.h>

namespace farnell {
using namespace klang::optimised;

// Dial, busy, ringback and pulse-dial telephone signalling sounds.
struct PhoneTones : Sound {
    // Clipped telephone wire and handset response, with a pulse-dial variant.
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
    pd::osc carrier[2], cadence;
    pd::lop smoothing;
    int elapsed = 0, pulseElapsed = 0, pulseCount = 0;
    Tone tone = Dial;
    bool busySmoothing = true;
    int lineVariant = -1; // -1 follows the mode; 1 selects the bulk pulse-style handset.

    PhoneTones() {
        controls = {
            klang::Dial("Tone", 0, 4, 0),
            klang::Dial("Dial frequency", 350, 500, 440),
            klang::Dial("Smooth busy", 0, 1, 1),
        };
    }

    // 440 Hz follows the book/local dialtone1; 450 follows the website.
    void set(param value, param dialHz = 440, param smoothBusy = 1) override {
        controls.set(float(value), float(dialHz), float(smoothBusy));
    }

    void prepare() override {
        tone = Tone(std::clamp(int(controls[0]), 0, 4));
        Frequency dialHz = controls[1];
        busySmoothing = float(controls[2]) >= 0.5f;
        carrier[0].set(tone == Busy || tone == Ringback ? 480 : 350);
        carrier[1].set(tone == Busy ? 620 : tone == Ringback ? 440 : dialHz);
        cadence.set(2);
        smoothing.set(100);
        line.set(lineVariant >= 0 ? lineVariant : int(tone == Pulse));
    }

    // Zero means ten pulses; each contact lasts 40 ms in a 100 ms cycle.
    void dial(int digit) { pulseCount = std::clamp(digit == 0 ? 10 : digit, 1, 10); pulseElapsed = 0; }

    void process() override {
        const int rate = int(float(fs));
        signal pair = carrier[0] + carrier[1];
        switch (tone) {
        case Dial:
            pair * 0.125f >> out;
            break;
        case DialLine:
            pair >> line >> out;
            break;
        case Busy: {
            signal gate = cadence;
            gate = std::clamp(float(gate) * 10000, 0.0f, 1.0f);
            if (busySmoothing) gate = gate >> smoothing;
            pair * gate * 0.1f >> out;
            break;
        }
        case Ringback: {
            // Keep the patch's 3 s off/on cadence; the book table says 4/2.
            const int tick = ((elapsed / 64 * 64 + 63) / rate) % 6;
            signal gate = float(tick > 2) >> smoothing;
            pair * gate >> line >> out;
            break;
        }
        case Pulse: {
            const int period = rate / 10, width = rate * 40 / 1000;
            signal contact = float(pulseElapsed < pulseCount * period && pulseElapsed % period < width);
            signal click = contact >> line;
            click * 0.8f >> out;
            if (pulseElapsed < pulseCount * period) ++pulseElapsed;
            break;
        }
        }
        // Wrap on both the six-second cadence and the DSP block grid.
        elapsed = (elapsed + 1) % (6 * rate * 64);
    }
};
}
