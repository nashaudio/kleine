#pragma once

// Andy Farnell, Designing Sound (2010), chapter 26, figures 26.1-26.3.
// Attribution and licensing: ../../../../README.md.
#include <klang/pd.h>
#include <string_view>
#include <vector>

namespace farnell {
using namespace klang::optimised;

// Sixteen-key DTMF dialler, with phase-continuous tones and 200 ms key presses.
struct DTMFTones : Sound {
    pd::osc high, low;
    pd::line envelope;
    pd::hip highpass;
    param duration = 200, level = 0.25f, edge = 1;
    bool filtered = true;
    int frame = 0, release = -1;
    inline static constexpr std::string_view keys = "123A456B789C*0#D";
    inline static constexpr int rows[] = {697, 770, 852, 941};
    inline static constexpr int columns[] = {1209, 1336, 1477, 1633};

    // Farnell's narrow-band detector and its deliberately simple 80 dB threshold.
    struct Detector : Modifier {
        pd::bpf filter;
        pd::env envelope;
        bool active = false, updated = false;
        void set(param hz) override { filter.set(hz, 24); }
        void process() override {
            signal filtered = in >> filter;
            signal level = filtered >> envelope;
            out = float(active);
            updated = envelope.updated;
            if (updated) active = int(envelope.level / 80) == 1;
        }
    };

    // Eight detectors map a single active row and column back to a keypad key.
    struct Decoder : Modifier {
        Detector detectors[8];
        int key = -1, previous = -1;
        bool changed = false;
        void prepare() {
            for (int i = 0; i < 4; ++i) { detectors[i].set(rows[i]); detectors[i + 4].set(columns[i]); }
        }
        void process() override {
            for (auto& detector : detectors) { signal state = in >> detector; }
            changed = false;
            if (detectors[0].updated) {
                int row = -1, column = -1, nr = 0, nc = 0;
                for (int i = 0; i < 4; ++i) {
                    if (detectors[i].active) { row = i; ++nr; }
                    if (detectors[i + 4].active) { column = i; ++nc; }
                }
                key = nr == 1 && nc == 1 ? row * 4 + column : -1;
                changed = key != previous;
                previous = key;
            }
            out = in;
        }
    };

    // Direct equivalent of the list difference/dot-product number match, with inactivity timeout.
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
            bool match = std::equal(received.begin(), received.end(), expected.begin());
            return match;
        }
    };

    void set(param milliseconds = 200, param gain = 0.25f, param filter = 1) override {
        duration = max(milliseconds, 0); level = gain; filtered = filter >= 0.5f;
    }
    void prepare() override { highpass.set(100); }
    void tones(param highHz, param lowHz) { high.set(highHz); low.set(lowHz); }
    void gate(param value) { envelope.set(value, edge); }
    void dial(char key) {
        const auto index = keys.find(key);
        if (index == keys.npos) return;
        tones(columns[index % 4], rows[index / 4]);
        gate(1);
        release = (frame + int(double(float(duration)) * float(fs) / 1000)) / 64 * 64;
    }
    void process() override {
        if (frame == release) { gate(0); release = -1; }
        signal pair = high + low;
        signal amplitude = envelope;
        signal tone = pair * amplitude;
        if (filtered) tone = tone >> highpass;
        tone * level >> out;
        ++frame;
    }
};
}
