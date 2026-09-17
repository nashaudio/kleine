// Language contracts: values, dispatch, sequencing and evaluation count.
#include <klang.h>
#include <cstdio>
#include <cstdlib>
#include <type_traits>

using klang::param;
using klang::signal;

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
    int calls = 0, absoluteSets = 0, relativeSets = 0, id = 1;
    param value = 12;
    void set(param next) override { ++absoluteSets; value = next; }
    void set(klang::relative next) override { ++relativeSets; value += next; }
    void process() override {
        ++calls;
        trace = trace * 10 + id;
        out = value;
    }
};

struct Stage : klang::Modifier {
    int calls = 0, pairSets = 0;
    param first = 0, second = 0;
    void set(param a, param b) override { ++pairSets; first = a; second = b; }
    void process() override {
        ++calls;
        trace = trace * 10 + 3;
        out = in * 2 + 1;
    }
};

struct StereoSource : klang::Stereo::Generator {
    int calls = 0;
    void process() override { ++calls; out = {12, 24}; }
};

// Capture a result before inspecting state: a passing value alone can conceal
// duplicate processing, reversed evaluation order, or a read of stale output.
template<class Expression>
void once(Expression expression, float expected) {
    Source source;
    trace = 0;
    signal result = expression(source);
    CHECK(result == expected);
    CHECK(source.calls == 1);
    CHECK(trace == 1);
}

template<class T>
void lvalues(T& value) {
    once([&](auto& source) { return source + value; }, 15);
    once([&](auto& source) { return source - value; }, 9);
    once([&](auto& source) { return source * value; }, 36);
    once([&](auto& source) { return source / value; }, 4);
}

int main() {
    static_assert(sizeof(signal) == sizeof(float));
    static_assert(sizeof(param) == sizeof(float));
    static_assert(std::is_base_of_v<signal, param>);
    static_assert(std::is_same_v<decltype(+signal(3)), klang::relative>);
    static_assert(std::is_same_v<decltype(signal(1) * 2.0), signal>);

    int integer = 3;
    float single = 3;
    double twice = 3;
    const int constInteger = 3;
    const float constSingle = 3;
    const double constTwice = 3;
    signal sample = 3;
    param parameter = 3;
    const signal constSample = 3;
    const param constParameter = 3;
    lvalues(integer); lvalues(single); lvalues(twice);
    lvalues(constInteger); lvalues(constSingle); lvalues(constTwice);
    lvalues(sample); lvalues(parameter); lvalues(constSample); lvalues(constParameter);

    once([](auto& source) { return 3 + source; }, 15);
    once([](auto& source) { return 3.f - source; }, -9);
    once([](auto& source) { return 3.0 * source; }, 36);
    once([](auto& source) { return 3 / source; }, .25f);

    // These function/scalar forms are used by Harrier, Helicopter and Mini.
    // Restoring unconstrained Output/float overloads makes them ambiguous.
#if defined(GAME_HEADER) || defined(STATELESS_MATH_FUNCTIONS)
    // The supplied 0.7.9 header and stateless math prototype return signal.
    static_assert(std::is_same_v<decltype(abs(signal(-.5)) * 2), signal>);
    static_assert(std::is_same_v<decltype(abs(signal(-.5)) * 2.0), signal>);
#else
    static_assert(std::is_same_v<decltype(abs(signal(-.5)) * 2), float>);
    static_assert(std::is_same_v<decltype(abs(signal(-.5)) * 2.0), double>);
#endif
    CHECK(abs(signal(-.5)) * 2 == 1);
    CHECK(abs(signal(-.5)) + 2 == 2.5f);
    CHECK(abs(signal(-.5)) / 2 == .25f);

#ifdef STATELESS_MATH_FUNCTIONS
    {
        signal result;
        double scalar = 1.25;
        scalar >> result;
        CHECK(result == 1.25f);
        2 >> result;
        CHECK(result == 2);
        char character = 3;
        character >> result;
        CHECK(result == 3);
        unsigned int natural = 4;
        natural >> result;
        CHECK(result == 4);
        long double extended = 5.5;
        extended >> result;
        CHECK(result == 5.5f);
        klang::constant fixed = { 6.25 };
        fixed >> result;
        CHECK(result == 6.25f);

        signal sample = .5f;
        0.25 * klang::tanh(sample * 3) / klang::tanh(3) >> result;
        CHECK(result == float(0.25 * std::tanh(1.5f) / std::tanh(3.0)));
    }
#endif

    // Existing double scalar arithmetic narrows BEFORE the operation.
    CHECK(signal(16777216.f) - 16777217.0 == 0);

    {
        Source source;
        trace = 0;
        auto& pending = source(7);
        CHECK(source.absoluteSets == 1 && source.calls == 0);
        signal result = pending;
        CHECK(result == 7 && source.calls == 1);
        result = source(+signal(2));
        CHECK(result == 9 && source.relativeSets == 1 && source.calls == 2);
        const Source& cached = source;
        result = cached;
        CHECK(result == 9 && source.calls == 2);
        CHECK(source.output() == 9 && source.calls == 2);
    }
    {
        Source left, right;
        right.id = 2;
        right.value = 3;
        trace = 0;
        signal result = left - right;
        CHECK(result == 9);
        CHECK(left.calls == 1 && right.calls == 1 && trace == 12);
    }
    {
        Source source;
        Stage stage;
        signal result;
        trace = 0;
        source >> stage >> result;
        CHECK(result == 25);
        CHECK(source.calls == 1 && stage.calls == 1 && trace == 13);
        stage << signal(4);
        CHECK(stage.calls == 1); // feedback input does not process the stage
        result = stage;
        CHECK(result == 9 && stage.calls == 2);
    }

#ifdef RESTORED_ARITHMETIC
    // Every operator with integer, float and double temporaries, including
    // inline calls whose static result type is Generic::Output<signal>&.
#define TEMPORARIES(value) \
    once([](auto& s) { return s + value; }, 15); \
    once([](auto& s) { return s - value; }, 9); \
    once([](auto& s) { return s * value; }, 36); \
    once([](auto& s) { return s / value; }, 4); \
    once([](auto& s) { return s(12) + value; }, 15); \
    once([](auto& s) { return s(12) - value; }, 9); \
    once([](auto& s) { return s(12) * value; }, 36); \
    once([](auto& s) { return s(12) / value; }, 4)
    TEMPORARIES(3); TEMPORARIES(3.f); TEMPORARIES(3.0);
#undef TEMPORARIES
    static_assert(std::is_same_v<decltype(std::declval<Source&>() * .5), signal>);
    once([](auto& s) { return s(16777216.f) - 16777217.0; }, 0);
    StereoSource stereo;
    auto result = stereo / 3;
    CHECK(result.l == 4 && result.r == 8 && stereo.calls == 1);
    result = stereo * 2.f;
    CHECK(result.l == 24 && result.r == 48 && stereo.calls == 2);
    result = stereo + 3.0;
    CHECK(result.l == 15 && result.r == 27 && stereo.calls == 3);
    result = stereo - 3;
    CHECK(result.l == 9 && result.r == 21 && stereo.calls == 4);
    Source source;
    Stage stage;
    signal routed;
    trace = 0;
    source(12) * .5 >> stage >> routed;
    CHECK(routed == 13 && source.calls == 1 && stage.calls == 1 && trace == 13);
#endif
#ifdef CHANNEL_REDUCTIONS
    {
        signal mono = 2;
        klang::signals<3> channels(mono);
        CHECK(channels[0] == 2 && channels[1] == 2 && channels[2] == 2);
        CHECK(channels.sum() == 6);
        CHECK(channels.average() == 2);
        CHECK(channels.mono() == 2);

        Source source;
        source.value = 7;
        klang::signals<3> generated(source);
        CHECK(generated[0] == 7 && generated[1] == 7 && generated[2] == 7);
        CHECK(source.calls == 1);
    }
    {
        klang::Bank<Stage, 3> bank;
        klang::signals<3> channels = {2.f, 3.f, 4.f};
        signal result;
        trace = 0;
        channels >> bank >> result;
        CHECK(result == 21); // (2*2+1) + (3*2+1) + (4*2+1)
        CHECK(bank.sum() == 21);
        CHECK(bank.mono() == 21);
        CHECK(bank[0].calls == 1 && bank[1].calls == 1 && bank[2].calls == 1);
        CHECK(trace == 333);
    }
#endif
#ifdef GENERATOR_PARAM
    {
        Source source;
        source.value = .5f;
        param evaluated = source;
        CHECK(evaluated == .5f && source.calls == 1);
        source.out = .25f;
        const Source& cached = source;
        param snapshot = cached;
        CHECK(snapshot == .25f && source.calls == 1);

        Source target, parameter;
        parameter.value = .75f;
        target(parameter);
        CHECK(parameter.calls == 1 && target.absoluteSets == 1 && target.value == .75f);

        Stage stage;
        parameter.value = .6f;
        stage(parameter, 10);
        CHECK(parameter.calls == 2 && stage.pairSets == 1);
        CHECK(stage.first == .6f && stage.second == 10);
    }
#endif
#ifdef GAME_HEADER
    {
        klang::Bank<Stage, 3> bank;
        klang::signals<3> channels;
        for (int n = 0; n < 3; ++n) channels[n] = 2;
        signal result;
        trace = 0;
        channels >> bank >> result;
        CHECK(result == 15); // Three channels summed; no averaging.
        CHECK(bank[0].calls == 1 && bank[1].calls == 1 && bank[2].calls == 1);
        CHECK(trace == 333);
    }
    {
        struct Gate : klang::Modifier {
            int inputs = 0;
            operator int() { return 0; }
            void input() override { ++inputs; }
        } gate;
        klang::Control control = klang::Dial("Control", 0, 1, 0);
        control.set(.5f);
        control >> gate;
        CHECK(gate.inputs == 1 && gate.in == .5f);
    }
#endif
    std::printf("%d runtime checks passed\n", checks);
}
