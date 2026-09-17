// Diagnostic for MSVC /fp:fast eliminating a float narrowing during folding.
// Compare /fp:fast and /fp:precise with either archived or working klang.h.
#include <klang.h>
#include <cstdio>

volatile double input = 16777217.0;
#if defined(_MSC_VER)
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE __attribute__((noinline))
#endif

NOINLINE float builtin(float a, double b) { return a - float(b); }
NOINLINE float wrapped(float a, double b) { return klang::signal(a) - b; }

int main() {
    const double value = input;
    std::printf("literal=%g wrapped=%g builtin=%g runtime=%g\n",
        float(klang::signal(16777216.f) - 16777217.0),
        wrapped(16777216.f, value), builtin(16777216.f, value),
        float(klang::signal(16777216.f) - value));
}
