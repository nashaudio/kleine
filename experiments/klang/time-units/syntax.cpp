// C++17 spelling/representation probe, not a public unit API or arithmetic implementation.
#include <klang.h>
#include <type_traits>
#include <cstdio>

namespace time_trial {
enum class Unit { ms, s, samples };

// Store each value in its declared units; the unit exists only in the type.
template<Unit unit> struct Value : klang::param {
    Value(float value = 0) : klang::param(value) {}
    using ms = Value<Unit::ms>;
    using s = Value<Unit::s>;
    using samples = Value<Unit::samples>;
};

using Time = Value<Unit::ms>;
using Seconds = Time::s;
using Samples = Time::samples;

namespace literals {
inline Time operator""_ms(unsigned long long value) { return float(value); }
inline Time operator""_ms(long double value) { return float(value); }
inline Seconds operator""_s(unsigned long long value) { return float(value); }
inline Seconds operator""_s(long double value) { return float(value); }
inline Samples operator""_samples(unsigned long long value) { return float(value); }
inline Samples operator""_samples(long double value) { return float(value); }
}

// Bare Time works in declarations, parameters and model members, as well as locals.
struct Model {
    Time duration = 100;
    Time::s seconds = .1f;
    Time::ms milliseconds = 100;
    Time::samples samples = 64;
};
float milliseconds(Time value) { return float(value); }

// Unit selection happens before conversion to an untyped param can discard it.
template<Unit unit> double asMilliseconds(Value<unit> value, double rate) {
    if constexpr (unit == Unit::s) return double(float(value)) * 1000;
    else if constexpr (unit == Unit::samples) return double(float(value)) * 1000 / rate;
    else return double(float(value));
}
}

namespace template_trial {
enum Unit { ms, s, samples };

// Public template alternative: C++17 deduction may supply the default for locals.
template<Unit unit = ms> struct Time : klang::param {
    Time(float value = 0) : klang::param(value) {}
    using s = Time<template_trial::s>;
};
struct Model {
    Time<> duration = 100;
    Time<samples> block = 64;
#ifdef REJECT_MEMBER
    Time unqualifiedMember = 100;
#endif
};
#ifdef REJECT_PARAMETER
void parameter(Time value) {}
#endif
#ifdef REJECT_SCOPE
using Nested = Time::s;
#endif
}

int main() {
    using namespace time_trial;
    using namespace time_trial::literals;
    Time duration;
    Time::s seconds;
    Time::ms milliseconds;
    Time::samples samples;
    Model model;
    auto a = 100_ms;
    auto b = 0.1_s;
    auto c = 64_samples;
    static_assert(std::is_same_v<decltype(a), Time>);
    static_assert(std::is_same_v<decltype(b), Time::s>);
    static_assert(std::is_same_v<decltype(c), Time::samples>);
    static_assert(std::is_base_of_v<klang::param, Time>);
    static_assert(sizeof(Time) == sizeof(float));
    static_assert(sizeof(Seconds) == sizeof(float));
    static_assert(sizeof(Samples) == sizeof(float));
    static_assert(std::is_same_v<Time, Time::ms>);

    template_trial::Time local;
    template_trial::Time initialised(100);
    template_trial::Time copied = 100;
    template_trial::Time<template_trial::samples> explicitSamples(64);
    static_assert(std::is_same_v<decltype(local), template_trial::Time<template_trial::ms>>);
    static_assert(std::is_same_v<decltype(initialised), decltype(local)>);
    static_assert(std::is_same_v<decltype(copied), decltype(local)>);
    static_assert(std::is_same_v<template_trial::Time<>::s, template_trial::Time<template_trial::s>>);

    if (asMilliseconds(a, 48000) != 100 || asMilliseconds(c, 48000) != 64.0 * 1000 / 48000)
        return 1;
    // Native-unit floats retain the existing param precision/semantics.
    if (float(b) != .1f || time_trial::milliseconds(model.duration) != 100)
        return 2;
    std::printf("Nested aliases, defaulted-template local deduction and literals pass; each unit type is %zu bytes.\n", sizeof(Time));
}
