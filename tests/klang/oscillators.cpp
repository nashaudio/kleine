// Measure oscillator contracts independently of historical sample identity.
// One second settles, one second measures; requested frequencies are integral
// so the latter window contains whole cycles at either test sample rate.
#include <klang.h>
#include <cstdio>
#include <cmath>
#include <memory>

// Diagnostic variants: snapshot phase before tick advances it. These exercise
// the existing OSM equations without changing either reference header.
#ifndef REPAIRED_OSM
template<class Oscillator, bool IsPulse = false>
struct Sequenced : Oscillator {
    void process() override {
        const float phase = this->osm.offset;
        const auto transition = this->osm.tick();
        if constexpr (IsPulse) this->out = this->osm.pulse(phase, transition);
        else this->out = this->osm.saw(phase - this->osm.col, transition);
    }
};

// Diagnostic for 0.7.9's clamped slope width: use that same width for the
// state-machine breakpoint, instead of leaving Saw's near-zero breakpoint.
struct WidthMatchedSaw : Sequenced<klang::optimised::Saw> {
    using Sequenced<klang::optimised::Saw>::set;
    void set(klang::param hz, klang::param phase) override {
        Sequenced<klang::optimised::Saw>::set(hz, phase, hz / klang::fs);
    }
};
#endif

template<class Oscillator>
void measure(const char* name, int rate, int hz, bool halfDuty = false) {
    klang::fs = rate;
    auto osc = std::make_unique<Oscillator>();
    if constexpr (std::is_base_of_v<klang::Generators::Fast::Osm, Oscillator>) {
        if (halfDuty) osc->set(hz, 0, .5f);
        else osc->set(hz, 0);
    } else osc->set(hz, 0);
    double total = 0, squares = 0, lo = 1e30, hi = -1e30;
    double real[3] = {}, imag[3] = {};
    double firstCrossing = 0, lastCrossing = 0, previous = 0;
    int crossings = 0, nonfinite = 0;
    for (int frame = 0; frame < rate * 2; ++frame) {
        osc->process();
        double value = float(osc->out);
        if (frame >= rate) {
            if (!std::isfinite(value)) ++nonfinite;
            else {
                total += value;
                squares += value * value;
                lo = std::min(lo, value);
                hi = std::max(hi, value);
                for (int h = 0; h < 3; ++h) {
                    const double angle = 2.0 * klang::pi.d * hz * (h + 1) * (frame - rate) / rate;
                    real[h] += value * std::cos(angle);
                    imag[h] += value * std::sin(angle);
                }
                if (previous <= 0 && value > 0) {
                    const double crossing = frame - 1 - previous / (value - previous);
                    if (!crossings) firstCrossing = crossing;
                    lastCrossing = crossing;
                    ++crossings;
                }
            }
        }
        previous = value;
    }
    double amplitude[3];
    for (int h = 0; h < 3; ++h) amplitude[h] = 2.0 * std::hypot(real[h], imag[h]) / rate;
    const double measured = crossings > 1 ? (crossings - 1) * double(rate) / (lastCrossing - firstCrossing) : 0;
    std::printf("%s,%d,%d,%.9g,%d,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g\n",
        name, rate, hz, float(osc->frequency), nonfinite, measured, lo, hi,
        total / rate, std::pow(squares / rate, .5), amplitude[0], amplitude[1], amplitude[2]);
}

int main(int argc, char** argv) {
    if (argc == 2 && std::strcmp(argv[1], "phase") == 0) {
        std::puts("requested_turns,reported_cycles,position");
        for (double turns : {0., .25, .5, .75, 1., -.25}) {
            klang::Generators::Fast::Phase phase;
            phase = klang::Phase(turns * 2.0 * klang::pi.d);
            std::printf("%.9g,%.9g,%u\n", turns, float(phase), phase.position);
        }
        return 0;
    }
    std::puts("wave,rate,requested_hz,cached_hz,nonfinite,rising_crossing_hz,min,max,mean,rms,h1,h2,h3");
    for (int rate : {44100, 48000}) {
        for (int hz : {100, 440, 1000}) {
            measure<klang::optimised::Sine>("sine", rate, hz);
            measure<klang::optimised::Saw>("saw", rate, hz);
            measure<klang::optimised::Triangle>("triangle", rate, hz);
            measure<klang::optimised::Square>("square-default", rate, hz);
            measure<klang::optimised::Square>("square-half-duty", rate, hz, true);
            measure<klang::optimised::Pulse>("pulse", rate, hz);
#ifndef REPAIRED_OSM
            measure<Sequenced<klang::optimised::Saw>>("sequenced-saw", rate, hz);
            measure<Sequenced<klang::optimised::Triangle>>("sequenced-triangle", rate, hz);
            measure<Sequenced<klang::optimised::Square, true>>("sequenced-square-half-duty", rate, hz, true);
            measure<Sequenced<klang::optimised::Pulse, true>>("sequenced-pulse", rate, hz);
            measure<WidthMatchedSaw>("sequenced-saw-width-matched", rate, hz);
#endif
        }
    }
}
