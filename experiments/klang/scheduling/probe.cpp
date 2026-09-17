#include "examples.h"
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <vector>
#include <thread>

static bool countAllocations = false;
static std::size_t allocations = 0;
void* operator new(std::size_t n) {
    if (countAllocations) ++allocations;
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

using namespace scheduling_trial;
static int checks = 0;
void require(bool okay, const char* message) {
    ++checks;
    if (!okay) throw std::runtime_error(message);
}

// Fixed test trace also exercises state read by process() after same-sample callbacks.
template<class Base> struct Trace : Base {
    int frame = 0, prepared = 0, events = 0, value = 0;
    struct Event { int frame, id; } log[8192]{};
    double a = 2.5, b = 5;
    bool registerB = true, immediate = false;
    void add(int id) {
        if (events == 8192) throw std::runtime_error("trace full");
        log[events++] = {frame, id};
        value = value * 3 + id;
        value %= 997;
    }
    void tickA() { add(1); }
    void tickB() { add(2); }
    void prepare() override {
        ++prepared;
        this->every(a, &Trace::tickA, immediate ? First::immediately : First::afterInterval);
        if (registerB) this->every(b, &Trace::tickB);
    }
    void process() override { this->out = float(value); ++frame; }
};

template<class S> void host(S& sound, float* output, int count) {
    static_cast<klang::Sound&>(sound).process(klang::buffer(output, count));
}
template<class S> std::vector<float> render(S& sound, int count, const std::vector<int>& chunks) {
    std::vector<float> audio(count);
    int offset = 0, chunk = 0;
    while (offset < count) {
        int n = std::min(chunks[chunk++ % chunks.size()], count - offset);
        host(sound, audio.data() + offset, n);
        offset += n;
    }
    return audio;
}

template<class Base> void timingCases() {
    for (int rate : {44100, 48000, 96000}) {
        klang::fs = rate;
        auto whole = std::make_unique<Trace<Base>>();
        auto split = std::make_unique<Trace<Base>>();
        whole->reserveTimers();
        split->reserveTimers();
        auto reference = render(*whole, rate / 2, {rate});
        auto actual = render(*split, rate / 2, {1, 3, 17, 79, 100, 257, 1024});
        require(actual == reference, "host partitions changed audio");
        require(whole->events == split->events, "host partitions changed event count");
        for (int i = 0; i < whole->events; ++i) {
            require(whole->log[i].frame == split->log[i].frame && whole->log[i].id == split->log[i].id,
                    "host partitions changed event order");
        }
        require(whole->log[0].frame == int(std::ceil(rate * .0025)), "default ms/first deadline");
        require(whole->log[1].id == 1 && whole->log[2].id == 2 &&
                whole->log[1].frame == whole->log[2].frame, "registration precedence");
        require(split->timerCount() == 2, "prepare registered duplicate timers");
        require(split->prepared > whole->prepared, "test did not repeat prepare");
    }
}

// Timer changes and cancellation from within a handler have defined sequential effects.
struct Mutation : scheduling_trial::Sound {
    int frame = 0, events = 0, log[32]{};
    void a() {
        log[events++] = frame;
        if (events == 1) {
            require(cancel(&Mutation::b), "callback cancellation");
            require(every(samples(7), &Mutation::a) == scheduling_trial::Result::changed, "callback interval change");
            require(every(samples(2), &Mutation::c) == scheduling_trial::Result::busy, "callback registration must be bounded");
        }
    }
    void b() { throw std::runtime_error("cancelled coincident handler ran"); }
    void c() {}
    void prepare() override {
        if (!timerCount()) {
            every(samples(5), &Mutation::a);
            every(samples(5), &Mutation::b);
        }
    }
    void process() override { out = 0; ++frame; }
};

// Virtual member invocation and non-zero base offset must preserve the target object.
struct OtherBase { virtual ~OtherBase() = default; std::uint64_t padding[5]{}; };
struct Multiple : OtherBase, scheduling_trial::Sound {
    int calls = 0;
    virtual void tick() { ++calls; }
    void prepare() override { every(samples(2), &Multiple::tick); }
    void process() override { out = float(calls); }
};
struct MoreDerived : Multiple { void tick() override { calls += 2; } };

// Existing sample/input/prepare behaviour is unchanged without registration.
template<class Base> struct Untimed : Base {
    int preparations = 0, frames = 0;
    void prepare() override { ++preparations; }
    void process() override { this->out = this->in * 0.25f + float(frames++ % 7); }
};

// Stereo dispatch follows the same deadline sequence as mono.
struct StereoTrace : StereoSound {
    int ticks = 0;
    void tick() { ++ticks; }
    void prepare() override { every(samples(3), &StereoTrace::tick, First::immediately); }
    void process() override { out = {in.l + float(ticks), in.r - float(ticks)}; }
};

// A direct queue receiver supports long-duration checks without rendering billions of samples.
struct LongTrace {
    Timers<2>* timers;
    std::uint64_t ticks = 0;
    bool correct = true;
    void tick() {
        ++ticks;
        const auto expected = (ticks * 441 + 9) / 10; // exact rational reference, no floating-point oracle
        correct = correct && timers->position() == expected;
    }
};

// A compact trace of coincident fractional deadlines and the actual callback thread.
struct FractionalTrace {
    int order[8]{}, count = 0;
    std::thread::id thread = std::this_thread::get_id();
    bool sameThread = true;
    void a() { order[count++] = 1; sameThread &= thread == std::this_thread::get_id(); }
    void b() { order[count++] = 2; }
};

// Clock advancement inside a PD block must deliver several short-period messages.
struct FastBlock : scheduling_trial::Sound {
    block_trial::Metro metro{.5f};
    int ticks = 0;
    bool started = false;
    void prepare() override {
        if (!started) { metro.start(true, [&](int) { ++ticks; }); started = true; }
        every(samples(64), &FastBlock::block, First::immediately);
    }
    void block() { metro.advance(64, [&](int) { ++ticks; }); }
    void process() override { out = float(ticks); }
};

// Timer mutation inside an audio span is rejected: the cached span has already been chosen.
struct SampleMutation : scheduling_trial::Sound {
    bool rejected = false;
    void tick() {}
    void prepare() override { every(samples(5), &SampleMutation::tick); }
    void process() override {
        rejected = every(samples(7), &SampleMutation::tick) == scheduling_trial::Result::busy;
        out = 0;
    }
};
struct IdleMutation : SampleMutation { void prepare() override {} };

void raw(const char* name, int rate, const std::vector<float>& audio) {
    char path[256];
    std::snprintf(path, sizeof path, "build/scheduling/%s-%d.f32", name, rate);
    FILE* file = std::fopen(path, "wb");
    if (!file) throw std::runtime_error("cannot write audio fixture");
    std::fwrite(audio.data(), sizeof(float), audio.size(), file);
    std::fclose(file);
}

int main() {
    try {
        timingCases<scheduling_trial::Sound>();
        timingCases<PointerSound>();
        timingCases<CompactSound>();
        klang::fs = 1000;
        {
            auto t = std::make_unique<Trace<scheduling_trial::Sound>>();
            t->a = 10; t->registerB = false;
            float audio[128]{};
            host(*t, audio, 0);
            require(t->events == 0 && t->prepared == 1, "zero buffer emitted event");
            host(*t, audio, 11);
            require(t->events == 1 && t->log[0].frame == 10, "buffer end/exclusive boundary");
            t->a = 7;
            host(*t, audio, 20);
            require(t->events == 3 && t->log[1].frame == 20 && t->log[2].frame == 27,
                    "changed interval must preserve pending tick");
            require(t->every(4, &Trace<scheduling_trial::Sound>::tickA, First::afterInterval, Change::restart) == scheduling_trial::Result::changed,
                    "restart interval result");
            t->a = 4;
            host(*t, audio, 5);
            require(t->log[3].frame == 35, "restart interval deadline");
            t->cancel(&Trace<scheduling_trial::Sound>::tickA);
            require(t->every(4, &Trace<scheduling_trial::Sound>::tickA) == scheduling_trial::Result::changed, "reactivate");
            host(*t, audio, 5);
            require(t->log[4].frame == 40, "reactivate waits interval");
        }
        {
            auto t = std::make_unique<Trace<scheduling_trial::Sound>>();
            t->a = 10; t->registerB = false; t->immediate = true;
            auto audio = render(*t, 21, {1});
            require(t->events == 3 && t->log[0].frame == 0 && t->log[2].frame == 20,
                    "immediate registration/prepare idempotence");
        }
        {
            auto t = std::make_unique<Mutation>();
            render(*t, 25, {25});
            require(t->events == 4 && t->log[0] == 5 && t->log[1] == 10 && t->log[2] == 17 && t->log[3] == 24,
                    "callback rescheduling/cancellation order");
        }
        {
            auto t = std::make_unique<MoreDerived>();
            auto audio = render(*t, 7, {3, 1});
            require(t->calls == 6 && audio[2] == 2 && audio[6] == 6, "member pointer adjustment/virtual dispatch");
        }
        {
            using One = Scheduled<klang::Sound, klang::buffer, Fixed<1>>;
            auto t = std::make_unique<Trace<One>>();
            float audio[1]{};
            host(*t, audio, 1);
            require(t->timerCount() == 1 && t->timerResult() == scheduling_trial::Result::full, "bounded capacity failure");
            require(t->every(0, &Trace<One>::tickA) == scheduling_trial::Result::invalid, "zero interval");
            require(t->every(-1, &Trace<One>::tickA) == scheduling_trial::Result::invalid, "negative interval");
            require(t->every(std::numeric_limits<double>::infinity(), &Trace<One>::tickA) == scheduling_trial::Result::invalid, "infinity");
            require(t->every(std::numeric_limits<double>::quiet_NaN(), &Trace<One>::tickA) == scheduling_trial::Result::invalid, "NaN");
            require(t->every(samples(.5), &Trace<One>::tickA) == scheduling_trial::Result::invalid, "sub-sample interval");
            require(t->every(1, static_cast<void (Trace<One>::*)()>(nullptr)) == scheduling_trial::Result::invalid, "null handler");
            t->every(2.5, &Trace<One>::tickA);
            require(t->timerError() == scheduling_trial::Result::invalid, "registration error must be sticky");
            t->clearTimerError();
            require(t->timerError() == scheduling_trial::Result::unchanged, "clear error");
        }
        {
            auto t = std::make_unique<Trace<CompactSound>>();
            require(t->every(10, &Trace<CompactSound>::tickA) == scheduling_trial::Result::unavailable, "unreserved registration");
            allocations = 0; countAllocations = true;
            require(t->reserveTimers() && t->reserveTimers(), "startup reservation");
            countAllocations = false;
            require(allocations == 1, "reservation must allocate exactly once");
        }
        {
            Timers<2> timers;
            LongTrace receiver{&timers};
            timers.sampleRate(44100);
            timers.every(milliseconds(1), Callback::bind(&receiver, &LongTrace::tick), First::afterInterval, Change::keepNext);
            std::uint64_t rendered = 0;
            timers.run(44100001, [&](int n) { rendered += n; });
            require(receiver.ticks == 1000000 && receiver.correct && rendered == 44100001,
                    "one million deadlines accumulated drift");
        }
        {
            Timers<2> timers;
            timers.sampleRate(1000);
            FractionalTrace receiver;
            timers.every(samples(2.8), Callback::bind(&receiver, &FractionalTrace::a), First::afterInterval, Change::keepNext);
            timers.every(samples(2.2), Callback::bind(&receiver, &FractionalTrace::b), First::afterInterval, Change::keepNext);
            timers.run(4, [](int) {});
            require(receiver.count == 2 && receiver.order[0] == 1 && receiver.order[1] == 2,
                    "same delivered sample must honour registration order");
            require(receiver.sameThread, "handler moved to another thread");
            require(!timers.sampleRate(0), "invalid rate");
        }
        {
            auto t = std::make_unique<Trace<scheduling_trial::Sound>>();
            t->a = 10; t->registerB = false;
            float audio[32]{};
            host(*t, audio, 5);
            klang::fs = 2000;
            host(*t, audio, 11);
            require(t->log[0].frame == 15 && t->timingValid(), "ms pending time across rate change");
            // samples(10) retains its remaining sample count across a rate change.
            Timers<2> timers;
            LongTrace receiver{&timers};
            timers.sampleRate(1000);
            timers.every(samples(10), Callback::bind(&receiver, &LongTrace::tick), First::afterInterval, Change::keepNext);
            timers.run(5, [](int) {});
            require(timers.sampleRate(2000), "sample rate update");
            timers.run(6, [](int) {});
            require(receiver.ticks == 1 && timers.position() == 11, "sample period changed with rate");
        }
        {
            auto t = std::make_unique<SampleMutation>();
            render(*t, 12, {12});
            require(t->rejected, "sample-time mutation must not corrupt cached span");
            auto idle = std::make_unique<IdleMutation>();
            render(*idle, 12, {12});
            require(idle->rejected && idle->timerCount() == 0, "untimed path must reject sample-time registration");
        }
        {
            klang::fs = 48000;
            auto original = std::make_unique<Untimed<klang::Sound>>();
            auto proposed = std::make_unique<Untimed<scheduling_trial::Sound>>();
            auto pointer = std::make_unique<Untimed<PointerSound>>();
            auto compact = std::make_unique<Untimed<CompactSound>>();
            float a[257], b[257], c[257], d[257];
            for (int i = 0; i < 257; ++i) a[i] = b[i] = c[i] = d[i] = std::sin(float(i));
            host(*original, a, 257); host(*proposed, b, 257); host(*pointer, c, 257); host(*compact, d, 257);
            require(!std::memcmp(a, b, sizeof a) && !std::memcmp(a, c, sizeof a) && !std::memcmp(a, d, sizeof a),
                    "untimed behaviour changed");
            require(proposed->preparations == original->preparations, "untimed prepare changed");
        }
        {
            auto t = std::make_unique<StereoTrace>();
            float l[8]{}, r[8]{};
            klang::buffer left(l, 8), right(r, 8);
            static_cast<klang::Stereo::Sound&>(*t).process(klang::Stereo::buffer(left, right));
            require(l[0] == 1 && r[0] == -1 && l[3] == 2 && l[6] == 3, "stereo scheduling");
        }
        {
            auto fixed = std::make_unique<Trace<scheduling_trial::Sound>>();
            auto reserved = std::make_unique<Trace<CompactSound>>();
            reserved->reserveTimers();
            float audio[128]{};
            allocations = 0; countAllocations = true;
            for (int i = 0; i < 100; ++i) {
                fixed->a = reserved->a = 2.5 + (i % 3);
                host(*fixed, audio, 128);
                host(*reserved, audio, 128);
            }
            countAllocations = false;
            require(allocations == 0, "render/prepare/update allocated memory");
        }
        {
            klang::fs = 48000;
            auto t = std::make_unique<FastBlock>();
            auto audio = render(*t, 128, {17, 3, 91});
            require(t->ticks == 6 && audio[0] == 3 && audio[63] == 3 && audio[64] == 6,
                    "multiple control messages inside one PD block");
            auto parent = std::make_unique<Parent>();
            audio = render(*parent, 9601, {257, 13});
            require(audio[4799] == 0 && audio[4800] == 1 && audio[9600] == 2,
                    "parent clock did not schedule nested component");
        }
        {
            auto child = std::make_unique<Trace<scheduling_trial::Sound>>();
            float audio[1]{};
            host(*child, audio, 0); // register through the normal buffer entry
            for (int i = 0; i < 300; ++i) {
                klang::signal value = static_cast<klang::Sound&>(*child);
                (void)value;
            }
            require(child->events == 0 && child->frame == 300,
                    "nested bypass limitation unexpectedly changed");
        }
        for (int rate : {44100, 48000}) {
            klang::fs = rate;
            auto pure = std::make_unique<Alarm>();
            auto block = std::make_unique<BlockAlarm>();
            auto split = std::make_unique<BlockAlarm>();
            const auto a = render(*pure, rate * 4, {64});
            const auto b = render(*block, rate * 4, {64});
            const auto c = render(*split, rate * 4, {1, 3, 100, 257, 1024});
            require(b == c, "explicit PD block changed with host partition");
            require(rate == 48000 ? a == b : a != b, "pure/PD block rate distinction");
            raw("alarm-pure", rate, a);
            raw("alarm-block", rate, b);
        }
        std::printf("{\"checks\":%d,\"steady_state_allocations\":%zu,\"long_run_ticks\":1000000,"
                    "\"sizeof\":{\"timer\":%zu,\"timers8\":%zu,\"original_sound\":%zu,\"fixed_sound\":%zu,"
                    "\"pointer_sound\":%zu,\"reserved_sound\":%zu}}\n",
                    checks, allocations, sizeof(Timer), sizeof(Timers<8>), sizeof(klang::Sound),
                    sizeof(scheduling_trial::Sound), sizeof(PointerSound), sizeof(CompactSound));
    } catch (const std::exception& error) {
        countAllocations = false;
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
