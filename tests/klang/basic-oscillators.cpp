// Basic oscillator phase contracts: direct, readable waveforms with robust wrapping.
#include <klang.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>

static int checks = 0;
void require(bool okay, const char* expression, int line) {
    ++checks;
    if (!okay) {
        std::fprintf(stderr, "line %d: %s\n", line, expression);
        std::exit(1);
    }
}
#define CHECK(expression) require(bool(expression), #expression, __LINE__)

bool near(float value, float expected, float tolerance = 1e-6f) {
    return std::fabs(value - expected) <= tolerance;
}

template<class Oscillator>
float next(Oscillator& oscillator) {
    oscillator.process();
    return oscillator.out;
}

#ifdef HARDENED_BASIC
using TestPhasor = klang::basic::Phasor;
#else
struct TestPhasor : klang::basic::Saw {
    void process() override {
        klang::basic::Saw::process();
        out = out * .5f + .5f;
    }
};
#endif

int main() {
    klang::fs = 48000;

    {
        TestPhasor phasor;
        phasor.set(-12000, 0);
#ifdef HARDENED_BASIC
        CHECK(near(next(phasor), 0));
        CHECK(near(next(phasor), .75f));
        CHECK(near(next(phasor), .5f));
        CHECK(near(next(phasor), .25f));
#else
        CHECK(near(next(phasor), 0));
        CHECK(near(next(phasor), -.25f));
        CHECK(near(next(phasor), -.5f));
        CHECK(near(next(phasor), -.75f));
#endif
    }
    {
        TestPhasor phasor;
        phasor.set(108000, 0); // 2.25 turns per sample
#ifdef HARDENED_BASIC
        CHECK(near(next(phasor), 0));
        CHECK(near(next(phasor), .25f));
        CHECK(near(next(phasor), .5f));
#else
        CHECK(near(next(phasor), 0));
        CHECK(near(next(phasor), 0));
        CHECK(near(next(phasor), 0));
#endif
    }
    {
        TestPhasor phasor;
        phasor.set(0, +klang::signal(.25f));
#ifdef HARDENED_BASIC
        CHECK(near(next(phasor), .25f));
        phasor.reset();
        CHECK(near(next(phasor), .25f));
#else
        CHECK(near(next(phasor), 0));
        phasor.reset();
        CHECK(near(next(phasor), 0));
#endif
    }
    {
        TestPhasor phasor;
        phasor.set(0, 10 * 2 * klang::pi.f + klang::pi.f / 2);
#ifdef HARDENED_BASIC
        CHECK(near(next(phasor), .25f, 2e-6f));
#else
        CHECK(next(phasor) > 10);
#endif
    }
#ifdef HARDENED_BASIC
    {
        klang::Phase phase = .5f;
        phase += klang::increment(-3.f, 2.f);
        CHECK(near(phase, 1.5f));
        phase += klang::increment(6.5f, 2.f);
        CHECK(near(phase, 0));
        CHECK(near(phase + klang::increment(-4.25f, 2.f), 1.75f));
    }
    {
        const klang::relative quarter = +klang::signal(.25f);
        klang::basic::Sine sine;
        klang::basic::Saw saw;
        klang::basic::Triangle triangle;
        klang::basic::Square square;
        klang::basic::Pulse pulse;
        sine.set(0, quarter);
        saw.set(0, quarter);
        triangle.set(0, quarter);
        square.set(0, quarter);
        pulse.set(0, quarter);
        CHECK(near(next(sine), 1));
        CHECK(near(next(saw), -.5f));
        CHECK(near(next(triangle), 0));
        CHECK(near(next(square), -1));
        CHECK(near(next(pulse), -1));
    }
    {
        klang::optimised::Phasor phasor;
        phasor.set(-12000, +klang::signal(.25f));
        CHECK(near(next(phasor), .25f));
        CHECK(near(next(phasor), 0));
        CHECK(near(next(phasor), .75f));
        phasor.set(108000, 0);
        phasor.set(+klang::signal(0));
        CHECK(near(next(phasor), 0));
        CHECK(near(next(phasor), .25f));
        phasor.reset();
        CHECK(near(next(phasor), 0));
    }
#endif

    std::printf("%d basic oscillator checks passed\n", checks);
}
