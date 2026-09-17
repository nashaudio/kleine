#include <klang.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>

static int checks = 0;
void require(bool okay, int line) {
    ++checks;
    if (!okay) {
        std::fprintf(stderr, "failed at line %d\n", line);
        std::exit(1);
    }
}
#define CHECK(expression) require(bool(expression), __LINE__)

#ifdef COMPARE_COUPLED_FOLLOWER
struct CoupledFollower : klang::Envelope::Follower {
    void process() override { sqrt((in * in) >> ar) >> out; }
};
#endif

int main() {
    klang::fs = 48000;
    klang::Envelope::Follower flowed;
#ifdef COMPARE_COUPLED_FOLLOWER
    CoupledFollower coupled;
#endif
    flowed.set(.003f, .075f);
#ifdef COMPARE_COUPLED_FOLLOWER
    coupled.set(.003f, .075f);
#endif

    for (int sample = 0; sample < 48000; ++sample) {
        const klang::signal input = std::sin(sample * .031f) *
            (sample < 16000 ? .25f : sample < 32000 ? 1.f : .05f);
        flowed.in = input;
#ifdef COMPARE_COUPLED_FOLLOWER
        coupled.in = input;
#endif
        flowed.process();
#ifdef COMPARE_COUPLED_FOLLOWER
        coupled.process();
        CHECK(flowed.out == coupled.out);
#else
        CHECK(std::isfinite(float(flowed.out)));
#endif
    }
    std::printf("%d follower-flow checks passed\n", checks);
}
