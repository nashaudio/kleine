// Function-flow and math-name contracts.
#define abs(value) platform_abs_macro(value)
#define sqrt(value) platform_sqrt_macro(value)
#define min(a, b) platform_min_macro(a, b)
#define max(a, b) platform_max_macro(a, b)
#define clamp(a, b, c) platform_clamp_macro(a, b, c)
#include <klang.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <type_traits>

#ifdef abs
#error Klang must neutralise a pre-existing abs macro
#endif
#ifdef sqrt
#error Klang must neutralise a pre-existing sqrt macro
#endif
#ifdef min
#error Klang must neutralise a pre-existing min macro
#endif
#ifdef max
#error Klang must neutralise a pre-existing max macro
#endif
#ifdef clamp
#error Klang must neutralise a pre-existing clamp macro
#endif

using namespace std;
using namespace klang::optimised;

static int calls = 0;

float hardclip(float input) {
    ++calls;
    if (input > 1.f) return 1.f;
    if (input < -1.f) return -1.f;
    return input;
}

static bool close(float a, float b) {
    return std::abs(a - b) < 1e-6f;
}

int main() {
    static_assert(std::is_same_v<decltype(klang::abs(klang::signal(-1))), klang::signal>);
    static_assert(std::is_same_v<decltype(klang::sqrt(klang::signal(4))), klang::signal>);
    static_assert(std::is_same_v<decltype(klang::tanh(klang::signal(1))), klang::signal>);
    static_assert(std::is_same_v<decltype(klang::sin(klang::signal(1))), klang::signal>);
    static_assert(std::is_same_v<decltype(abs(-1.f)), float>);
    static_assert(std::is_same_v<decltype(sqrt(4.f)), float>);

    signal input = .75f;
    signal out;

    input * 2.f >> hardclip >> out;
    if (!close(out, 1.f) || calls != 1) return 1;
    input >> hardclip >> out;
    if (!close(out, .75f) || calls != 2) return 2;

    signal negative = -.5f;
    negative >> std::abs >> out;
    if (!close(out, .5f)) return 3;
    negative >> abs >> out;
    if (!close(out, .5f)) return 4;

    signal four = 4.f;
    four >> std::sqrt >> out;
    if (!close(out, 2.f)) return 5;
    four >> sqrt >> out;
    if (!close(out, 2.f)) return 6;

    signal angle = pi / 2;
    angle >> sin >> out;
    if (!close(out, 1.f)) return 23;
    signal unity = 1.f;
    unity >> tanh >> out;
    if (!close(out, std::tanh(1.f))) return 24;
    if (!close(cos(signal(0)), 1.f) || !close(exp(signal(0)), 1.f)) return 25;

    if (!close(abs(negative), .5f)) return 7;
    if (!close(sqrt(four), 2.f)) return 8;
    if (!close(abs(-2.f), 2.f) || !close(sqrt(9.f), 3.f)) return 9;
    if (std::abs(-2.0) != 2.0 || std::sqrt(9.0) != 3.0) return 10;

    Control control = Dial("Value", -4, 4, -2);
    if (!close(abs(control), 2.f)) return 11;
    control.set(4);
    if (!close(sqrt(control), 2.f)) return 12;

    signal arithmetic = 1 - abs(signal(-.5f));
    if (!close(arithmetic, .5f)) return 13;
    static_assert(std::is_same_v<decltype(abs(signal(-.5f)) * 2), signal>);

    if (min(2, 3) != 2 || max(2.0, 3.0) != 3.0 || std::clamp(2.f, -1.f, 1.f) != 1.f) return 22;

    auto functionGraph = std::make_unique<Graph>();
    (*functionGraph)(0, 4);
    std::sqrt >> *functionGraph;
    if (functionGraph->getData().count != 1) return 14;

    GraphPtr functionGraphPtr;
    hardclip >> functionGraphPtr(0, 2);
    if (functionGraphPtr.getData().count != 1) return 15;

    std::printf("function-flow contracts pass\n");
    return 0;
}
