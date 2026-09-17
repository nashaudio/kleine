// Focused compatibility and accuracy checks for klang::fast trigonometry.
#include <klang.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

using namespace klang::optimised;

namespace legacy {
    inline float polynomial(float x) {
        const float x2 = x * x;
        return (((-0.00018542f * x2 + 0.0083143f) * x2 - 0.16666f) * x2 + 1.0f) * x;
    }

    inline float sin(float x) {
        x = klang::fast_mod2pi(x);
        if (x > 3 / 2.f * klang::pi)
            x -= klang::fast::twoPi;
        else if (x > klang::pi / 2)
            x = klang::pi - x;
        return polynomial(x);
    }

    inline float sinp(std::uint32_t phase) {
        float x = klang::fast_modp(phase);
        if (x > 3.f / 2.f * klang::pi)
            x -= klang::fast::twoPi;
        else if (x > klang::pi / 2.f)
            x = klang::pi - x;
        return polynomial(x);
    }
}

static bool bitsEqual(float a, float b) {
    std::uint32_t left, right;
    std::memcpy(&left, &a, sizeof(left));
    std::memcpy(&right, &b, sizeof(right));
    return left == right;
}

int main() {
    if (!bitsEqual(fast::sin(.25f), klang::fast::sin(.25f))) return 5;
    double sinSquares = 0, cosSquares = 0;
    float sinPeak = 0, cosPeak = 0;
    constexpr int count = 1000001;

    for (int i = 0; i < count; ++i) {
        const float x = float(-100.0 * klang::pi.d + 200.0 * klang::pi.d * i / (count - 1));
        const float sine = klang::fast::sin(x);
        const float cosine = klang::fast::cos(x);
        if (!bitsEqual(sine, legacy::sin(x))) return 1;
        const float sinError = sine - std::sin(x);
        const float cosError = cosine - std::cos(x);
        sinPeak = std::max(sinPeak, std::abs(sinError));
        cosPeak = std::max(cosPeak, std::abs(cosError));
        sinSquares += double(sinError) * sinError;
        cosSquares += double(cosError) * cosError;
    }

    std::uint32_t state = 0x12345678u;
    for (int i = 0; i < 1000000; ++i) {
        state = state * 1664525u + 1013904223u;
        if (!bitsEqual(klang::fast::sinp(state), legacy::sinp(state))) return 2;
        const float radians = float(double(state) / 4294967296.0 * klang::fast::twoPi);
        if (std::abs(klang::fast::cosp(state) - std::cos(radians)) > .0002f) return 3;
    }

    if (sinPeak > .0002f || cosPeak > .0002f) return 4;
    std::printf("fast trig pass: sin peak %.9g rms %.9g; cos peak %.9g rms %.9g\n",
        sinPeak, std::sqrt(sinSquares / count), cosPeak, std::sqrt(cosSquares / count));
    return 0;
}
