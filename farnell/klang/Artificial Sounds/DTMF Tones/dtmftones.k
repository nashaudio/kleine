#pragma once

// Andy Farnell, Designing Sound (2010), chapter 26, figures 26.1-26.3.
// Attribution and licensing: ../../../../README.md.
#include <klang/pd.h>
#include <string_view>
#include <vector>

namespace farnell {
using namespace klang::optimised;

/** @brief Sixteen-key DTMF dialler with continuous oscillator phase and 200 ms keys.
 * @code
 * auto* dialler = new farnell::DTMFTones;
 * processor.attach(dialler);
 * dialler->dial('5');
 * @endcode
 * dial(key) uses the fixed keypad table and releases each key after 200 ms.
 */
struct DTMFTones : Sound {
    pd::osc high, low;
    pd::line envelope;
    pd::hip highpass;
    pd::del del{200};
    inline static constexpr std::string_view keys = "123A456B789C*0#D";
    inline static constexpr int rows[] = {697, 770, 852, 941};
    inline static constexpr int columns[] = {1209, 1336, 1477, 1633};

    // One PD bp~/env~ tone detector, using the patch's 80 dB threshold.
    struct Detector : Modifier {
        pd::bpf filter;
        pd::env envelope;
        signal active = 0;
        void set(param hz) override { filter.set(hz, 24); }
        void process() override {
            in >> filter >> envelope >> out;
            if (envelope.updated)
                active = int(out / 80) == 1;
            out = active;
        }
    };

    // Farnell's sand.pd: both inlets trigger AND, then key/-1 and change.
    struct Sand {
        int key, out = 0;
        bool inputs[2] = {};
        explicit Sand(int value = 0) : key(value) {}
        bool send(int inlet, param value) {
            inputs[inlet] = int(float(value)) != 0;
            const int next = inputs[0] && inputs[1] ? key : -1;
            if (next == out) return false;
            out = next;
            return true;
        }
    };

    // Standalone dtmfdec.pd: sixteen AND/select pairs, emitting held keys repeatedly.
    struct Decoder : Modifier {
        Detector detectors[8];
        bool inputs[16][2] = {};
        std::vector<char> messages;
        Decoder() { messages.reserve(16); }
        void prepare() {
            for (int i = 0; i < 4; ++i) {
                detectors[i].set(rows[i]);
                detectors[i + 4].set(columns[i]);
            }
        }
        void send(int key, int inlet, bool value) {
            inputs[key][inlet] = value;
            if (inlet == 0 && inputs[key][0] && inputs[key][1]) messages.push_back(keys[key]);
        }
        void process() override {
            messages.clear();
            // Independent env~ outlets arrive in reverse creation order in this patch.
            for (int i = 7; i >= 0; --i) {
                auto& detector = detectors[i];
                detector.in = in;
                detector.process();
                if (!detector.envelope.updated) continue;
                if (i < 4) {
                    for (int column = 0; column < 4; ++column) {
                        const int key = i * 4 + (i == 3 && column < 2 ? 1 - column : column);
                        send(key, key == 4 ? 1 : 0, detector.active);
                    }
                } else {
                    for (int row = 0; row < 4; ++row) {
                        const int key = row * 4 + i - 4;
                        send(key, key == 4 ? 0 : 1, detector.active);
                    }
                }
            }
            out = in;
        }
    };

    // Phone/call-recogniser decoder: ten sand abstractions, shared change and moses 0.
    struct DigitDecoder : Modifier {
        Detector detectors[7];
        Sand sand[10] = {Sand(0), Sand(1), Sand(2), Sand(3), Sand(4),
                         Sand(5), Sand(6), Sand(7), Sand(8), Sand(9)};
        int previous = 0;
        std::vector<int> messages;
        DigitDecoder() { messages.reserve(10); }
        void prepare() {
            for (int i = 0; i < 7; ++i) detectors[i].set(i < 4 ? rows[i] : columns[i - 4]);
        }
        void send(int key, int inlet, bool value) {
            if (!sand[key].send(inlet, float(value)) || sand[key].out == previous) return;
            previous = sand[key].out;
            if (previous >= 0) messages.push_back(previous);
        }
        void process() override {
            messages.clear();
            for (int i = 6; i >= 0; --i) {
                auto& detector = detectors[i];
                detector.in = in;
                detector.process();
                if (!detector.envelope.updated) continue;
                if (i < 3) {
                    for (int column = 0; column < 3; ++column) {
                        const int key = i * 3 + column + 1;
                        send(key, key == 4 ? 1 : 0, detector.active);
                    }
                } else if (i == 3) send(0, 0, detector.active);
                else {
                    for (int row = 0; row < 3; ++row) {
                        const int key = row * 3 + i - 3;
                        send(key, key == 4 ? 0 : 1, detector.active);
                    }
                    if (i == 5) send(0, 1, detector.active);
                }
            }
            out = in;
        }
    };

    // Reconstructed number matcher with a five-second inactivity reset.
    struct Number {
        std::string_view expected;
        std::vector<char> received;
        int lastFrame = -1;
        explicit Number(std::string_view digits) : expected(digits) { received.reserve(digits.size()); }
        bool digit(char key, int frame) {
            if (lastFrame >= 0 && frame - lastFrame >= int(float(fs) * 5)) received.clear();
            lastFrame = frame;
            if (received.size() == expected.size()) return false;
            received.push_back(key);
            if (received.size() != expected.size()) return false;
            return std::equal(received.begin(), received.end(), expected.begin());
        }
    };

    void prepare() override { highpass.set(100); }
    void tones(param highHz, param lowHz) {
        high.set(highHz);
        low.set(lowHz);
    }
    void gate(param value) { envelope.set(value, 1); }
    void dial(char key) {
        const auto index = keys.find(key);
        if (index == keys.npos) return;
        tones(columns[index % 4], rows[index / 4]);
        gate(1);
        del.bang();
    }
    void process() override {
        if (del()) gate(0);
        (high + low) * envelope >> highpass >> out;
        out *= .25f;
    }
};
}
