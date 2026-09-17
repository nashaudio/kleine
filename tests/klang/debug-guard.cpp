// Host debug opt-in is independent of optimisation and _DEBUG.
#include <klang.h>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <thread>
#include <type_traits>

static_assert(HAS_KLANG_DEBUG == EXPECT_CAPTURE);
static_assert(std::is_base_of_v<klang::Effect,klang::Sound>);
void require(bool condition, int line) {
    if (!condition) { std::fprintf(stderr,"Failed at line %d\n",line); std::exit(1); }
}
#define CHECK(x) require(bool(x),__LINE__)

struct Sound : klang::Sound {
    using klang::Sound::process;
    int prepared = 0;
    void prepare() override { ++prepared; }
    void process() override {
        (in >> klang::debug) >> out;
        out *= .5f;
    }
};
struct Stereo : klang::Stereo::Sound {
    using klang::Stereo::Sound::process;
    void process() override {
        in.l >> klang::debug;
        out = in * .5f;
    }
};

int main() {
    auto sound=std::make_unique<Sound>();
    float samples[16];
    for (int i=0;i<16;++i) samples[i]=float(i)*.125f;
    {
        klang::Debug::Session capture(nullptr,16,klang::Debug::Buffer::Effect);
        sound->process(klang::buffer(samples,16));
        CHECK(sound->prepared==1);
        for (int i=0;i<16;++i) CHECK(samples[i]==float(i)*.0625f);
#if HAS_KLANG_DEBUG
        CHECK(capture.hasAudio());
        const float* audio=capture.getAudio();
        CHECK(audio);
        for (int i=0;i<16;++i) CHECK(audio[i]==float(i)*.125f);
#else
        CHECK(!capture.hasAudio() && !capture.getAudio());
#endif
    }
    auto stereo=std::make_unique<Stereo>();
    float frames[8]={1,2,3,4,5,6,7,8};
    {
        klang::Debug::Session capture(nullptr,4,klang::Debug::Buffer::Effect);
        stereo->process(klang::Stereo::interleaved::buffer(frames,4));
        for(int i=0;i<8;++i) CHECK(frames[i]==float(i+1)*.5f);
#if HAS_KLANG_DEBUG
        CHECK(capture.hasAudio());
        const float* audio=capture.getAudio();
        for(int i=0;i<4;++i) CHECK(audio[i]==float(2*i+1));
#endif
    }
    klang::debug.print("capture %d",7);
    klang::debug.printOnce("once");
    CHECK(klang::debug.hasText()==bool(HAS_KLANG_DEBUG));
    char text[256]={};
    CHECK((klang::debug.getText(text)>0)==bool(HAS_KLANG_DEBUG));
    int profiled=0;
    klang::Debug::profile([&] { ++profiled; });
    CHECK(profiled==int(HAS_KLANG_DEBUG));
#if !HAS_KLANG_DEBUG
    // Normal rendering may run indefinitely without a Debug::Session.
    for(int n=0;n<2000;++n) sound->process(klang::buffer(samples,16));
    klang::debug.in=123; // Verify that all supported input routes bypass this field.
    auto write=[](float sample) {
        klang::Input& base=klang::debug;
        for(int n=0;n<40000;++n) {
            klang::signal value=sample;
            value >> klang::debug;
            const klang::signal& tapped=static_cast<const klang::Debug&>(klang::debug);
            CHECK(tapped==sample);
            klang::debug << value;
            base.input(value);
            base << value;
            klang::debug.input();
            klang::debug.buffer += value;
            klang::debug.buffer++;
            klang::debug.print("discard %d",n);
        }
    };
    std::thread a(write,.5f),b(write,.75f); a.join(); b.join();
    CHECK(klang::debug.in==123 && !klang::debug.buffer.active);
    const klang::Debug& debug=klang::debug;
    const klang::signal& silence=debug;
    CHECK(silence==0);
#endif
    std::printf("capture=%d passed; sizes Debug=%zu Buffer=%zu Session=%zu Sound=%zu\n",
                int(HAS_KLANG_DEBUG),sizeof(klang::Debug),sizeof(klang::Debug::Buffer),
                sizeof(klang::Debug::Session),sizeof(klang::Sound));
}
