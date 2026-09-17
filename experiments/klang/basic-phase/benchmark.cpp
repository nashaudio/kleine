#include <klang.h>
#include <chrono>
#include <cstdio>

template<class Phasor>
double run(const char* name, int samples) {
    Phasor phasor;
    phasor.set(440, 0);
    double sum = 0;
    const auto start = std::chrono::steady_clock::now();
    for (int sample = 0; sample < samples; ++sample) {
        phasor.process();
        sum += phasor.out;
    }
    const auto end = std::chrono::steady_clock::now();
    const double ns = std::chrono::duration<double, std::nano>(end - start).count() / samples;
    std::printf("%s %.3f ns/sample checksum %.9g\n", name, ns, sum);
    return ns;
}

int main() {
    klang::fs = 48000;
    constexpr int samples = 50000000;
    run<klang::basic::Phasor>("basic", samples);
    run<klang::optimised::Phasor>("optimised", samples);
}
