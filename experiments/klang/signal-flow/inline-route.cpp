// K-002: isolate inline-generator routing ambiguity on the unchanged Klang core.
#include <klang.h>
#include <cstdio>
using namespace klang::optimised;

// A generator that counts evaluations and emits the selected value.
struct Source : Generator {
    param value = 0;
    int calls = 0;
    void set(param v) override { value = v; }
    void process() override {
        ++calls;
        out = value;
    }
};

// A modifier that counts evaluations and doubles its input.
struct Destination : Modifier {
    int calls = 0;
    void process() override {
        ++calls;
        out = in * 2;
    }
};

int main() {
    Source source;
    Destination destination;
#ifdef REPRODUCE_AMBIGUITY
    signal result = source(2) >> destination; // Expected MSVC C2593.
#else
    signal result = signal(source(2)) >> destination;
#endif
    if (result != 4 || source.calls != 1 || destination.calls != 1) return 1;
    std::puts("Inline conversion: result 4; generator and modifier each evaluated once.");
}
