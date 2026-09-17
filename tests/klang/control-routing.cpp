#include <klang.h>
#include <cstdio>
#include <cstdlib>
#include <type_traits>

static int checks = 0;
static int trace = 0;
void require(bool okay, const char* expression, int line) {
    ++checks;
    if (!okay) {
        std::fprintf(stderr, "line %d: %s\n", line, expression);
        std::exit(1);
    }
}
#define CHECK(expression) require(bool(expression), #expression, __LINE__)

struct Source : klang::Generator {
    int calls = 0;
    klang::signal next = .5f;
    void process() override { ++calls; trace = trace * 10 + 1; out = next; }
};

struct Gate : klang::Modifier {
    int inputs = 0, calls = 0;
    operator int() { return 0; } // Reproduce the former competing conversion.
    void input() override { ++inputs; trace = trace * 10 + 2; }
    void process() override { ++calls; trace = trace * 10 + 3; out = in * 2; }
};

struct StereoGate : klang::Stereo::Modifier {
    int inputs = 0;
    void input() override { ++inputs; }
    void process() override { out = in; }
};

int main() {
    klang::Control control = klang::Dial("Control", 0, 1, 0);

#ifdef SIMPLIFIED_CONTROL_ROUTING
    control = .75;
    CHECK(control.value == .75f);
    control = 2;
    CHECK(control.value == 1);
    unsigned char byte = 0;
    control = byte;
    CHECK(control.value == 0);
    control = klang::signal(.25f);
    CHECK(control.value == .25f);

    klang::Controls controls;
    controls = { klang::Group(
        klang::Dial("0"), klang::Dial("1"), klang::Dial("2"), klang::Dial("3")) };
    controls[3] = .625;
    CHECK(controls[3].value == .625f);

    klang::ControlMap mapped;
    static_assert(std::is_assignable_v<klang::ControlMap&, klang::Control&>);
    mapped = controls[3];
    mapped = .5;
    CHECK(controls[3].value == .5f);
    2.0 >> mapped;
    CHECK(controls[3].value == 1);

    Source source;
    source.next = .4f;
    control = source;
    CHECK(control.value == .4f && source.calls == 1);
    source.next = .6f;
    source >> control;
    CHECK(control.value == .6f && source.calls == 2);
    source.out = .3f;
    const Source& cached = source;
    cached >> control;
    CHECK(control.value == .3f && source.calls == 2);
    source.next = .45f;
    source >> mapped;
    CHECK(controls[3].value == .45f && source.calls == 3);

    klang::signal sample;
    control = .5f;
    control >> sample;
    CHECK(sample == .5f);
    sample = .75f;
    sample >> control;
    CHECK(control.value == .75f);

    Gate gate;
    trace = 0;
    control >> gate;
    CHECK(gate.in == .75f && gate.inputs == 1 && gate.calls == 0 && trace == 2);
    gate >> sample;
    CHECK(sample == 1.5f && gate.calls == 1 && trace == 23);

    const klang::Control& readOnly = control;
    readOnly >> gate;
    CHECK(gate.in == .75f && gate.inputs == 2 && gate.calls == 1);

    StereoGate stereo;
    control >> stereo;
    CHECK(stereo.inputs == 1 && stereo.in.l == .75f && stereo.in.r == .75f);
#else
    // Retained header supports scalar Control reads and explicit set().
    control.set(.75f);
    CHECK(float(control) == .75f);
    CHECK(control.smooth() > 0);
#endif

    std::printf("%d control routing checks passed\n", checks);
}
