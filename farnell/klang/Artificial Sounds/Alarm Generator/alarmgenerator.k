#pragma once

// Andy Farnell, Designing Sound (2010), chapter 27, figures 27.1-27.7.
// Attribution and licensing: ../../../../README.md.
#include <klang/pd.h>

namespace farnell {
using namespace klang::optimised;

/** @brief Seven alarm studies, from gated tones to a four-voice programmed sequence.
 * @code
 * auto* alarm = new farnell::AlarmGenerator(7);
 * processor.attach(alarm);
 * alarm->set(380, 2, 349, 0, 0, 0, 1);
 * alarm->trigger();
 * @endcode
 * The programme is durationMs, cycles, four frequenciesHz and spectral colour.
 * Studies 1/5 use set(on); the other fixed studies run continuously.
 * Bank is the earlier three-voice study: set() takes eight normalised controls,
 * gate rate (*4 Hz), scan rate (*24 Hz), then three frequency (*1000 Hz)/colour pairs.
 */
struct AlarmGenerator : Sound {
    // Cosine-waveshaped oscillator; colour blends even and odd harmonics.
    struct Voice : Generator {
        pd::osc osc;
        pd::cos shaper;
        param spectrum = 1;
        void set(param hz, param colour = 1) override {
            frequency = hz;
            spectrum = colour;
        }
        void process() override {
            spectrum * 0.25f + osc(frequency) >> shaper >> out;
        }
        Frequency frequency = 0;
    } voice[4];

    // Earlier three-voice alarm bank with eight normalised controls.
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
            cadence.set(gateRate * 4);
            phase.set(scanRate * 24);
            voice[0].set(a * 1000, ta);
            voice[1].set(b * 1000, tb);
            voice[2].set(c * 1000, tc);
        }
        void prepare() override {
            smoothing.set(100);
            dc.set(50);
        }
        void process() override {
            signal position = phase;
            signal gate = std::clamp(float(signal(cadence)) * 10000, 0.0f, 1.0f) >> smoothing;
            out = 0;
            for (int i = 0; i < 3; ++i) {
                signal segment = std::clamp(float(position) * 3, float(i), float(i + 1)) - i;
                signal amplitude = segment * 0.5f - 0.25f >> window[i];
                out += voice[i] * amplitude;
            }
            out * gate * 0.2f >> dc >> out;
        }
    };

    pd::osc carrier[3], cadence;
    pd::phasor phase;
    pd::line timebase;
    pd::wrap wrapping;
    pd::cos window[4];
    pd::lop smoothing[3];
    pd::hip dc;
    pd::metro metro;
    int study = 7, frame = 0, resetAt = -1, count = 0, step = -1;
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
        frequencies[0] = a;
        frequencies[1] = b;
        frequencies[2] = c;
        frequencies[3] = d;
        spectrum = colour;
    }
    void trigger() {
        timebase.set(0);
        timebase.set(1, duration);
        resetAt = frame + int(std::ceil(double(float(duration)) * float(fs) / 1000));
    }
    void set(param on) override { metro = on; }
    void prepare() override {
        for (auto& filter : smoothing) filter.set(70);
        dc.set(50);
    }
    void process() override {
        out = 0;
        if (study == 1 || study == 5) {
            if (metro(study == 1 ? 300 : 500))
                step = count++ % (study == 1 ? 2 : 3);
            if (study == 1) {
                out = float(step == 1) * carrier[0](800) * 0.2f;
            } else {
                static constexpr constant hz[] = {723, 932, 1012};
                for (int i = 0; i < 3; ++i) {
                    out += carrier[i](hz[i]) * (float(step == i) >> smoothing[i]);
                }
                out *= 0.2f;
            }
        } else if (study == 2 || study == 3) {
            signal amplitude = std::clamp(float(signal(cadence(2))) * 1000000, 0.0f, 1.0f) >> smoothing[0];
            out = (study == 2 ? amplitude : 1 - amplitude) * carrier[0](study == 3 ? 600 : 800);
            if (study == 3) out += carrier[1](800) * amplitude;
            out *= 0.2f;
        } else if (study == 4) {
            signal position = phase(4);
            static constexpr constant hz[] = {723, 932, 1012};
            for (int i = 0; i < 3; ++i) {
                signal segment = std::clamp(float(position) * 3, float(i), float(i + 1)) - i;
                signal amplitude = segment * 0.5f - 0.25f >> window[i];
                out += carrier[i](hz[i]) * amplitude;
            }
            out *= 0.25f;
        } else {
            if (frame == resetAt) {
                timebase.set(0);
                resetAt = -1;
            }
            signal position = timebase * cycles >> wrapping;
            for (int i = 0; i < 4; ++i) {
                signal segment = std::clamp(float(position) * 4, float(i), float(i + 1)) - i;
                signal amplitude = segment * 0.5f - 0.25f >> window[i];
                out += voice[i](frequencies[i], spectrum) * amplitude;
            }
            out * 0.25f >> dc >> out;
            out *= outputGain;
        }
        ++frame;
    }
};
}
