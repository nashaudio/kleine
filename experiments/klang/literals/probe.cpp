#include "literals.h"
#include <klang/pd.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <type_traits>

using namespace klang::literals;

static_assert(std::is_same_v<decltype(1_Hz), klang::Frequency>);
static_assert(std::is_same_v<decltype(1.0_Hz), klang::Frequency>);
static_assert(std::is_same_v<decltype(1._Hz), klang::Frequency>);
static_assert(std::is_same_v<decltype(.5_Hz), klang::Frequency>);
static_assert(std::is_same_v<decltype(1e3_Hz), klang::Frequency>);
static_assert(std::is_same_v<decltype(48'000_Hz), klang::Frequency>);
static_assert(std::is_same_v<decltype(1_kHz), klang::Frequency>);
static_assert(std::is_same_v<decltype(1.0_kHz), klang::Frequency>);
static_assert(std::is_convertible_v<decltype(1_Hz), klang::param>);
static_assert(std::is_convertible_v<decltype(1_Hz), klang::signal>);
static_assert(sizeof(decltype(1_Hz)) == sizeof(float));

// Importing literals leaves ordinary literal types and overload choices intact.
static_assert(std::is_same_v<decltype(1), int>);
static_assert(std::is_same_v<decltype(1.f), float>);
static_assert(std::is_same_v<decltype(1.0), double>);
template<int n> using Choice = std::integral_constant<int, n>;
Choice<0> choose(int);
Choice<1> choose(float);
Choice<2> choose(double);
Choice<3> choose(klang::Frequency);
Choice<4> choose(klang::Pitch);
Choice<5> choose(klang::param);
Choice<6> choose(klang::signal);
static_assert(decltype(choose(1))::value == 0);
static_assert(decltype(choose(1.f))::value == 1);
static_assert(decltype(choose(1.0))::value == 2);
static_assert(decltype(choose(1_Hz))::value == 3);
static_assert(decltype(choose(1.0_Hz))::value == 3);
static_assert(decltype(choose(1._Hz))::value == 3);

// Exact type identity also resolves standard template deduction.
static_assert(std::is_same_v<
    decltype(std::max(klang::Frequency(220), 440_Hz)), const klang::Frequency&>);
static_assert(std::is_same_v<decltype(true ? 1_Hz : 2.0_Hz), klang::Frequency>);

void require(bool okay, const char* message) {
    if (!okay) {
        std::fprintf(stderr, "%s\n", message);
        std::exit(1);
    }
}

// Exercise existing virtual param dispatch and exactly-once sample evaluation.
struct Source : klang::Generator {
    klang::param value = 0;
    int calls = 0;
    void set(klang::param next) override { value = next; }
    void process() override {
        ++calls;
        out = value;
    }
};

// Remain usable in ordinary expressions with an existing modifier.
struct Double : klang::Modifier {
    int calls = 0;
    void process() override {
        ++calls;
        out = in * 2;
    }
};

int main() {
    require(float(1_Hz) == 1 && float(1.0_Hz) == 1 && float(1._Hz) == 1,
            "Equivalent literal spellings differ");
    require(float(1_kHz) == 1000 && float(1.2345_kHz) == 1234.5f,
            "Kilohertz conversion failed");
    auto upper = std::max(klang::Frequency(220), 440_Hz);
    auto limited = std::clamp(upper, 20_Hz, 20_kHz);
    require(float(limited) == 440, "Typed standard algorithms failed");

    klang::param parameter = 440_Hz;
    klang::signal sample = 440_Hz;
    require(float(parameter) == 440 && float(sample) == 440, "Base conversion failed");
    Source source;
    Double twice;
    // K-002's existing explicit materialisation is still needed for this route.
    klang::signal result = klang::signal(source(2_Hz)) >> twice;
    require(float(result) == 4 && source.calls == 1 && twice.calls == 1,
            "Setter dispatch or evaluation count changed");

    int compared = 0;
    for (int rate : {44100, 48000}) {
        klang::fs = rate;
        pd::osc plain, hertz, kilohertz;
        for (int frame = 0; frame < rate * 2; ++frame) {
            // Exercise a frequency change through normal inline set(param) calls.
            const bool changed = frame >= rate;
            const float value = changed ? 1234.5f : 440.f;
            klang::signal a, b, c;
            0.2f * plain(value) >> a;
            0.2f * hertz(changed ? 1234.5_Hz : 440_Hz) >> b;
            0.2f * kilohertz(changed ? 1.2345_kHz : .44_kHz) >> c;
            require(std::memcmp(&a.value, &b.value, sizeof(float)) == 0 &&
                    std::memcmp(&a.value, &c.value, sizeof(float)) == 0,
                    "Literal substitution changed oscillator output");
            ++compared;
        }
    }
    std::printf("Types, overloads, values, dispatch and routing pass; %d frames "
                "sample-identical for plain/Hz/kHz at 44.1/48 kHz.\n", compared);
}
