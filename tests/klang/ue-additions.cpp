// UE additions: frame layout/ownership, host processing, Phasor and setter dispatch.
#include <klang.h>
#include <cstdio>
#include <cstdlib>
#include <memory>

int checks = 0;
void require(bool okay, int line) {
    ++checks;
    if (!okay) { std::fprintf(stderr, "failed at line %d\n", line); std::exit(1); }
}
#define CHECK(x) require(bool(x), __LINE__)

struct StereoGain : klang::Stereo::Effect {
    using Effect::process;
    int prepared = 0, processed = 0;
    void prepare() override { ++prepared; }
    void process() override { ++processed; out = in * 2.f; }
};
struct StereoSource : klang::Stereo::Sound {
    using Sound::process;
    int prepared = 0;
    void prepare() override { ++prepared; }
    void process() override { out = { .25f, -.5f }; }
};

template<int N> void frames() {
    using Buffer = klang::interleaved::buffer<N>;
    Buffer owned(4, .5f);
    for (int i=0; i<4; ++i)
        for (int c=0; c<N; ++c) owned[i][c] = float(i*10+c);
    for (int i=0; i<4; ++i)
        for (int c=0; c<N; ++c) CHECK(owned.data()[i*N+c] == float(i*10+c));
    CHECK(float(owned[4][0]) == 0);
    CHECK(float(owned[-1][0]) == 30);
    const Buffer& read = owned;
    CHECK(float(read[2][0]) == 20);
    CHECK(float(read[1.5f][0]) == 15);
    CHECK(float(read[3.5f][0]) == 15);
    CHECK(float(read[-.5f][0]) == 15);
    {
        Buffer copy(owned);
        CHECK(copy.data() == owned.data());
        copy[2][0] = 99;
        copy++;
        CHECK(copy.offset() == 1 && owned.offset() == 0);
    }
    CHECK(float(owned[2][0]) == 99); // The borrowed copy did not free its owner.
    owned.set(2);
    owned += typename Buffer::signal(3);
    CHECK(float(owned[0][0]) == 5);
    owned *= typename Buffer::signal(2);
    CHECK(float(owned[0][0]) == 10);
    Buffer destination(3, -1);
    destination = owned;
    CHECK(float(destination[2][0]) == 2);
    destination.clear(1);
    CHECK(float(destination[0][0]) == 0 && float(destination[1][0]) == 2);
    destination.rewind(2);
    destination++;
    CHECK(destination.finished());
    Buffer empty(nullptr, 0);
    empty.clear(); empty.rewind();
    CHECK(empty.finished() && empty.offset() == 0);
}

int main() {
    frames<2>(); frames<3>(); frames<4>();
    float host[] = {-999, 1,2,3,4,5,6,7,8, 999};
    klang::Stereo::interleaved::buffer audio(host+1, 4);
    auto gain = std::make_unique<StereoGain>();
    {
        klang::Debug::Session debug(host+1,4,klang::Debug::Buffer::Effect);
        gain->process(audio);
    }
    CHECK(gain->prepared == 1 && gain->processed == 4);
    for(int i=1; i<=8; ++i) CHECK(host[i] == float(i*2));
    CHECK(host[0] == -999 && host[9] == 999 && audio.offset() == 0);
    auto source = std::make_unique<StereoSource>();
    {
        klang::Debug::Session debug(host+1,4,klang::Debug::Buffer::Synth);
        source->process(audio);
    }
    CHECK(source->prepared == 1 && host[1] == .25f && host[2] == -.5f);

    for(int rate : {44100,48000}) {
        klang::fs = rate;
        klang::optimised::Phasor ramp;
        klang::basic::Phasor basic;
        for(float hz : {440.f,-440.f,0.f}) {
            ramp.set(hz, .7f); basic.set(hz, .7f);
            double phase = .7 / (2 * klang::pi.d);
            phase -= std::floor(phase);
            for(int i=0; i<rate; ++i) {
                ramp.process(); basic.process();
                const auto phaseError = [phase](float actual) {
                    const double difference = std::fabs(double(actual) - phase);
                    return std::min(difference, 1 - difference);
                };
                CHECK(phaseError(ramp.out) <= 1.2e-5);
                CHECK(phaseError(basic.out) <= 4e-4);
                CHECK(ramp.out >= 0 && ramp.out < 1);
                CHECK(basic.out >= 0 && basic.out < 1);
                phase += double(hz) / rate;
                phase -= std::floor(phase);
            }
        }
    }
    klang::Filters::DCF filter;
    klang::Modifier& base = filter;
    base(.25f);
    CHECK(filter.r == .25f);
    filter.in = 1; filter.process();
    filter.in = 0; filter.process();
    CHECK(filter.out == -.75f);
    float levels[] = {-.25f,.75f};
    CHECK(klang::buffer(levels,2).level() == .5f);
    std::printf("%d checks passed\n", checks);
}
