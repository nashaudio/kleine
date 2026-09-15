#pragma once

// Andy Farnell, Designing Sound (2010), chapter 27, figures 27.1-27.7.
// Attribution and licensing: ../../../../README.md.
#include <klang/pd.h>

namespace farnell {
using namespace klang::optimised;

// The seven alarm studies, from gated sine tones to a programmable four-tone sequence.
struct AlarmGenerator : Sound {
    // An oscillator whose cosine waveshaper blends even and odd harmonics.
    struct Voice : Generator {
        pd::osc oscillator;
        pd::cos shaper;
        param spectrum = 1;
        void set(param hz, param colour = 1) override { oscillator.set(hz); spectrum = colour; }
        void process() override {
            signal wave = oscillator;
            wave + spectrum * 0.25f >> shaper >> out;
        }
    } voice[4];

    // Earlier eight-control bank: three shaped tones under a separate gated cadence.
    struct Bank : Sound {
        Voice voice[3];
        pd::phasor phase;
        pd::osc cadence;
        pd::lop smoothing;
        pd::hip dc;
        pd::cos window[3];
        Bank() { cadence.set(0.5f); }
        void set(param gateRate, param scanRate, param a, param ta,
                 param b, param tb, param c, param tc) override {
            cadence.set(gateRate * 4); phase.set(scanRate * 24);
            voice[0].set(a * 1000, ta); voice[1].set(b * 1000, tb); voice[2].set(c * 1000, tc);
        }
        void prepare() override { smoothing.set(100); dc.set(50); }
        void process() override {
            signal position = phase, lfo = cadence, sum = 0;
            signal gate = std::clamp(float(lfo) * 10000, 0.0f, 1.0f) >> smoothing;
            for (int i = 0; i < 3; ++i) {
                signal segment = std::clamp(float(position) * 3, float(i), float(i + 1)) - i;
                signal amplitude = segment * 0.5f - 0.25f >> window[i];
                signal wave = voice[i];
                sum += wave * amplitude;
            }
            sum * gate * 0.2f >> dc >> out;
        }
    };

    pd::osc carrier[3], cadence;
    pd::phasor phase;
    pd::line timebase;
    pd::wrap wrapping;
    pd::cos window[4];
    pd::lop smoothing[3];
    pd::hip dc;
    int study = 7, frame = 0, resetAt = -1, step = 0;
    double nextTick = 0;
    bool running = false, hasTick = false;
    param duration = 380, cycles = 2, outputGain = 0.5f;
    Frequency frequencies[4] = {349, 0, 0, 0};
    param spectrum = 1;

    explicit AlarmGenerator(int example = 7) : study(example) {
        if (study == 6) outputGain = 0.2f;
    }
    // A programme is the same seven-number list used by the PD patch.
    void set(param milliseconds, param repetitions, param a, param b,
             param c, param d, param colour) override {
        duration = max(milliseconds, 0);
        cycles = repetitions;
        frequencies[0] = a; frequencies[1] = b; frequencies[2] = c; frequencies[3] = d;
        spectrum = colour;
    }
    void trigger() {
        timebase.set(0);
        timebase.set(1, duration);
        resetAt = (frame + int(double(float(duration)) * float(fs) / 1000)) / 64 * 64;
    }
    void start(bool enabled = true) {
        running = enabled;
        if (enabled) {
            hasTick = true;
            step = (step + 1) % (study == 1 ? 2 : 3);
            nextTick = frame + (study == 1 ? 300.0 : 500.0) * float(fs) / 1000;
        }
    }
    void prepare() override {
        for (auto& filter : smoothing) filter.set(70);
        dc.set(50);
        cadence.set(2); phase.set(4);
        for (int i = 0; i < 4; ++i) voice[i].set(frequencies[i], spectrum);
        carrier[0].set(study == 3 ? 600 : study >= 4 ? 723 : 800);
        carrier[1].set(study >= 4 ? 932 : 800);
        carrier[2].set(1012);
    }
    void process() override {
        signal result = 0;
        if (study == 1 || study == 5) {
            if (running && frame / 64 >= int(nextTick / 64)) {
                hasTick = true;
                step = (step + 1) % (study == 1 ? 2 : 3);
                nextTick += (study == 1 ? 300.0 : 500.0) * float(fs) / 1000;
            }
            if (study == 1) {
                signal wave = carrier[0];
                result = wave * (hasTick && step == 0 ? 1 : 0) * 0.2f;
            } else {
                for (int i = 0; i < 3; ++i) {
                    signal amplitude = float(hasTick && (step + 2) % 3 == i) >> smoothing[i];
                    signal wave = carrier[i];
                    result += wave * amplitude;
                }
                result *= 0.2f;
            }
        } else if (study == 2 || study == 3) {
            signal lfo = cadence;
            signal amplitude = std::clamp(float(lfo) * 1000000, 0.0f, 1.0f) >> smoothing[0];
            signal wave = carrier[0];
            result = wave * (study == 2 ? amplitude : 1 - amplitude);
            if (study == 3) { signal other = carrier[1]; result += other * amplitude; }
            result *= 0.2f;
        } else if (study == 4) {
            signal position = phase;
            for (int i = 0; i < 3; ++i) {
                signal segment = std::clamp(float(position) * 3, float(i), float(i + 1)) - i;
                signal amplitude = segment * 0.5f - 0.25f >> window[i];
                signal wave = carrier[i];
                result += wave * amplitude;
            }
            result *= 0.25f;
        } else {
            if (frame == resetAt) { timebase.set(0); resetAt = -1; }
            signal ramp = timebase;
            signal position = ramp * cycles >> wrapping;
            for (int i = 0; i < 4; ++i) {
                signal segment = std::clamp(float(position) * 4, float(i), float(i + 1)) - i;
                signal amplitude = segment * 0.5f - 0.25f >> window[i];
                signal wave = voice[i];
                result += wave * amplitude;
            }
            result = result * 0.25f >> dc;
            result *= outputGain;
        }
        result >> out;
        ++frame;
    }
};
}
