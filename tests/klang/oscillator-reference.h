// Independent periodic antiderivatives for oscillator checks.
#pragma once
#include <klang.h>
#include <cmath>
#include <cstdint>

// Keep the independent oracle precise even when the candidate header uses fast math.
#pragma float_control(precise, on, push)

namespace oscillator_reference {
namespace fast = klang::Generators::Fast;
constexpr double turn = 4294967296.0;
double primitive(double t, double duty, bool pulse) {
    const double cycles = std::floor(t), p = t - cycles;
    if (pulse) return cycles * (2 * duty - 1) + 2 * std::min(p, duty) - p;
    if (duty == 0) return p - p * p;
    if (duty == 1) return p * p - p;
    if (p < duty) return p * p / duty - p;
    const double down = p - duty;
    return down - down * down / (1 - duty);
}

double point(double p, double duty, bool pulse) {
    if (pulse) return p < duty ? 1 : -1;
    return p < duty ? 2 * p / duty - 1 : 1 - 2 * (p - duty) / (1 - duty);
}

// Quantisation is part of the retained 32-bit phase-counter contract.
std::int64_t step(float hz, int rate) {
    return static_cast<std::int64_t>(double(hz) / rate * turn);
}
std::uint32_t position(float radians) {
    double cycles = double(radians) / fast::twoPi;
    cycles -= std::floor(cycles);
    return static_cast<std::uint32_t>(cycles * turn);
}

}
#pragma float_control(pop)
