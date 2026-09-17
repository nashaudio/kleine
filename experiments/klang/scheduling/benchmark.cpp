#include "scheduler.h"
#include <klang/pd.h>
#include <cstdio>
#include <chrono>
#include <vector>
#include <memory>
#include <algorithm>
#include <windows.h>

namespace trial = scheduling_trial;
using CompactPointer = trial::Scheduled<klang::Sound, klang::buffer, trial::Reserved<8>, trial::Dispatch::functionPointer>;

// Identical sample work for each dispatcher. Timer callbacks only update a held value.
template<class Base, int Count, bool Changing = false> struct Work : Base {
    float state = .1f, gain = .75f;
    int calls = 0, preparations = 0;
    pd::osc osc;
    int dsp = 0;
    void a() { gain = .5f + float(++calls % 100) * .005f; }
    void b() { a(); }
    void c() { a(); }
    void d() { a(); }
    void e() { a(); }
    void f() { a(); }
    void g() { a(); }
    void h() { a(); }
    void prepare() override {
        if constexpr (Count > 0) {
            const double interval = Changing ? 1 + (++preparations % 3) * .1 : 1;
            this->every(interval, &Work::a);
            if constexpr (Count == 8) {
                this->every(1.1, &Work::b); this->every(1.2, &Work::c);
                this->every(1.3, &Work::d); this->every(1.4, &Work::e);
                this->every(1.5, &Work::f); this->every(1.6, &Work::g);
                this->every(1.7, &Work::h);
            }
        }
    }
    void process() override {
        if (dsp) this->out = osc(731) * gain;
        else {
            state = state * .99f + .001f;
            this->out = state * gain;
        }
    }
};

// Recompute an expensive coefficient either per sample, with a manual counter, or a timer.
template<class Base, int Mode> struct Configuration : Base {
    float state = .25f, coefficient = .9f;
    volatile float cutoff = 731;
    int remaining = 0;
    __declspec(noinline) void configure() { coefficient = std::exp(-6.2831853f * cutoff / 48000.f); }
    void prepare() override {
        if constexpr (Mode == 2) this->every(10, &Configuration::configure, trial::First::immediately);
    }
    void process() override {
        if constexpr (Mode == 0) configure();
        if constexpr (Mode == 1) {
            if (remaining == 0) { configure(); remaining = 480; }
            --remaining;
        }
        state = coefficient * state + (1 - coefficient) * .25f;
        this->out = state;
    }
};

// Opaque virtual entry prevents the harness from turning models into constant expressions.
__declspec(noinline) double measure(klang::Sound& sound, int block, int frames) {
    float output[1024]{};
    const auto start = std::chrono::steady_clock::now();
    for (int offset = 0; offset < frames; offset += block)
        sound.process(klang::buffer(output, std::min(block, frames - offset)));
    const auto end = std::chrono::steady_clock::now();
    static volatile float sink;
    sink = output[0];
    return std::chrono::duration<double, std::nano>(end - start).count() / frames;
}

struct Case {
    const char* name;
    int dsp;
    std::unique_ptr<klang::Sound> sound;
};

double processCpuSeconds() {
    FILETIME created, exited, kernel, user;
    if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user)) return -1;
    const auto ticks = [](FILETIME value) {
        return (std::uint64_t(value.dwHighDateTime) << 32) | value.dwLowDateTime;
    };
    return double(ticks(kernel) + ticks(user)) * 1e-7;
}

template<class Base, int Count, bool Changing = false>
void add(std::vector<Case>& cases, const char* name, int dsp) {
    auto model = std::make_unique<Work<Base, Count, Changing>>();
    model->dsp = dsp;
    if constexpr (Count > 0 && !std::is_same_v<Base, klang::Sound>) model->reserveTimers();
    cases.push_back({name, dsp, std::move(model)});
}

int main() {
    const bool affinity = SetThreadAffinityMask(GetCurrentThread(), DWORD_PTR(1) << 2) != 0;
    klang::fs = 48000;
    std::vector<Case> cases;
    for (int dsp : {0, 1}) {
        add<klang::Sound, 0>(cases, "original", dsp);
        add<trial::Sound, 0>(cases, "fixed_branch_idle", dsp);
        add<trial::CompactSound, 0>(cases, "reserved_branch_idle", dsp);
        add<trial::PointerSound, 0>(cases, "fixed_pointer_idle", dsp);
        add<CompactPointer, 0>(cases, "reserved_pointer_idle", dsp);
        add<trial::Sound, 1>(cases, "fixed_branch_one", dsp);
        add<trial::PointerSound, 1>(cases, "fixed_pointer_one", dsp);
        add<trial::CompactSound, 1>(cases, "reserved_branch_one", dsp);
        add<trial::Sound, 8>(cases, "fixed_branch_eight", dsp);
        add<trial::PointerSound, 8>(cases, "fixed_pointer_eight", dsp);
        add<trial::Sound, 1, true>(cases, "fixed_branch_changing", dsp);
    }
    cases.push_back({"configure_sample", 2, std::make_unique<Configuration<klang::Sound, 0>>()});
    cases.push_back({"configure_counter", 2, std::make_unique<Configuration<klang::Sound, 1>>()});
    cases.push_back({"configure_timer", 2, std::make_unique<Configuration<trial::Sound, 2>>()});
    const double cpuStart = processCpuSeconds();
    std::uint64_t totalFrames = 0;
    std::printf("case,dsp,buffer,round,ns_per_sample\n");
    for (int block : {32, 64, 256, 1024}) {
        for (auto& item : cases) {
            measure(*item.sound, block, 262144);
            totalFrames += 262144;
        }
        for (int round = 0; round < 7; ++round) {
            // Rotate order to reduce frequency/temperature bias toward one implementation.
            for (int i = 0; i < int(cases.size()); ++i) {
                auto& item = cases[(i + round * 7) % cases.size()];
                const double ns = measure(*item.sound, block, 4194304);
                totalFrames += 4194304;
                std::printf("%s,%d,%d,%d,%.9f\n", item.name, item.dsp, block, round, ns);
            }
        }
    }
    const double cpuEnd = processCpuSeconds();
    const double cpuSeconds = cpuStart < 0 || cpuEnd < 0 ? -1 : cpuEnd - cpuStart;
    std::fprintf(stderr, "{\"process_cpu_seconds\":%.6f,\"rendered_audio_seconds\":%.6f,\"affinity_cpu_2\":%s}\n",
                 cpuSeconds, double(totalFrames) / 48000,
                 affinity ? "true" : "false");
}
