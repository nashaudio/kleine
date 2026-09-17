#pragma once

#include <klang/pd.h>

namespace farnell {
using namespace klang::optimised;

// Control random arithmetic adapted from Pure Data 0.55-2, src/x_misc.c. Copyright (c) 1997-2024 Miller Puckette.
// Redistribution terms: docs/licenses/Pure-Data-BSD.txt (repository root).

// Read-before-write sample delay; a helper, not a named-buffer PD port.
struct SampleDelay {
    std::vector<signal> buffer;
    int position = 0;
    void set(int samples) {
        samples = std::max(1, samples);
        if (int(buffer.size()) != samples) {
            buffer.assign(samples, 0);
            position = 0;
        }
    }
    signal read() const { return buffer[position]; }
    void write(signal x) {
        buffer[position] = x;
        position = (position + 1) % int(buffer.size());
    }
    signal tick(signal x) {
        signal y = read();
        write(x);
        return y;
    }
};

// Seeded PD control-random helper; next(range) requires a positive range.
struct ControlRandom {
    uint32_t state = 1;
    void seed(uint32_t value) { state = value; }
    int next(int range = 1000) {
        state = state * 472940017u + 832416023u;
        return std::min(range - 1, int(double(range) * double(state) / 4294967296.0));
    }
};
}
