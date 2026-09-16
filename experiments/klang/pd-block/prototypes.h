#pragma once

// Design probes for farnell/PD-BLOCK.md. These are not public Klang/PD APIs.
#include <klang/pd.h>
#include <stdexcept>

namespace block_trial {
using namespace klang::optimised;

// A local sample clock; calling it consumes exactly one sample position.
struct Block {
    int length = 64, remaining = 0;
    bool enabled = true;
    explicit Block(int samples = 64) { set(samples); }
    void set(param samples) {
        float value = samples;
        if (!std::isfinite(value) || value < 0 || value > 1048576 || std::floor(value) != value)
            throw std::invalid_argument("block size must be an integer from 0 to 1048576");
        length = int(value);
        remaining = 0;
        enabled = length != 0;
    }
    int samples() const { return length; }
    void disable() { enabled = false; }
    bool operator()() {
        if (!enabled) return false;
        bool first = remaining == 0;
        if (first) remaining = length;
        --remaining;
        return first;
    }
    template<class F, class... Args> void operator()(F&& action, Args&&... args) {
        if ((*this)()) std::invoke(std::forward<F>(action), std::forward<Args>(args)...);
    }
};

// A proposed metronome: immediate first count, held state on stop, fractional deadlines.
struct Metro {
    param interval = 100;
    double now = 0, next = 0;
    int count = 0;
    bool running = false;
    explicit Metro(param milliseconds = 100) : interval(milliseconds) {}
    double period() const { return double(float(interval)) * float(fs) / 1000; }
    template<class F> void start(bool enabled, F&& tick) {
        running = enabled;
        if (enabled) {
            tick(count++);
            next = now + period();
        }
    }
    template<class F> void advance(int samples, F&& tick) {
        if (samples <= 0 || interval <= 0) throw std::invalid_argument("invalid metro interval/span");
        const double end = now + samples;
        while (running && next < end) {
            tick(count++);
            next += period();
        }
        now = end;
    }
    template<class F> void operator()(F&& tick) { advance(1, std::forward<F>(tick)); }
    template<class F> void operator()(const Block& span, F&& tick) {
        advance(span.samples(), std::forward<F>(tick));
    }
};

// Inheritance supplies storage only; the derived process chooses how to use it.
struct EquippedSound : Sound { Block blocks; };

// Automatic block hook on the existing buffer entry point; nested sample routes bypass it.
struct BufferSound : EquippedSound {
    virtual event block() { blocks.disable(); }
    void process(klang::buffer audio) override {
        prepare();
        while (!audio.finished()) {
            input(audio);
            if (blocks()) block();
            this->process();
            audio++ = out;
            debug.buffer++;
        }
    }
    using Sound::process;
};

// Alternative policy: keep calling an empty default handler at every internal boundary.
struct AlwaysSound : BufferSound { event block() override {} };

// Automatic hook for every existing dispatch path, at the visible cost of naming sample().
struct SampleSound : EquippedSound {
    virtual event block() { blocks.disable(); }
    virtual void sample() { out = in; }
    void process() final {
        if (blocks()) block();
        sample();
    }
    using Sound::process;
};

// Ordinary sample-timed presentation: no block object or block handler.
struct IdealPedestrians : Sound {
    pd::osc tone;
    Metro metro{100};
    signal gate = 0;
    void start(bool on = true) { metro.start(on, [&](int count) { gate = count % 2; }); }
    void prepare() override { tone.set(2500); }
    void process() override {
        metro([&](int count) { gate = count % 2; });
        tone * gate * 0.2f >> out;
    }
};

// Explicit member and one-shot callback; remains safe under nested signal evaluation.
struct MemberPedestrians : Sound {
    Block blocks;
    pd::osc tone;
    Metro metro{100};
    signal gate = 0;
    void start(bool on = true) { metro.start(on, [&](int count) { gate = count % 2; }); }
    void prepare() override { tone.set(2500); }
    event update() { metro(blocks, [&](int count) { gate = count % 2; }); }
    void process() override {
        blocks(&MemberPedestrians::update, this);
        tone * gate * 0.2f >> out;
    }
};

// Inherited block state and lambda; no block member in the model declaration.
struct EquippedPedestrians : EquippedSound {
    pd::osc tone;
    Metro metro{100};
    signal gate = 0;
    void start(bool on = true) { metro.start(on, [&](int count) { gate = count % 2; }); }
    void prepare() override { tone.set(2500); }
    event update() { metro(blocks, [&](int count) { gate = count % 2; }); }
    void process() override {
        blocks([&] { update(); });
        tone * gate * 0.2f >> out;
    }
};

// Clean process()/block() model syntax; prototype's automatic scope is buffer entry only.
struct EventPedestrians : BufferSound {
    pd::osc tone;
    Metro metro{100};
    signal gate = 0;
    void start(bool on = true) { metro.start(on, [&](int count) { gate = count % 2; }); }
    void prepare() override { tone.set(2500); }
    event block() override { metro(blocks, [&](int count) { gate = count % 2; }); }
    void process() override {
        tone * gate * 0.2f >> out;
    }
};

// Automatic events on both buffer and nested routes, with sample() replacing process().
struct SamplePedestrians : SampleSound {
    pd::osc tone;
    Metro metro{100};
    signal gate = 0;
    void start(bool on = true) { metro.start(on, [&](int count) { gate = count % 2; }); }
    void prepare() override { tone.set(2500); }
    event block() override { metro(blocks, [&](int count) { gate = count % 2; }); }
    void sample() override {
        tone * gate * 0.2f >> out;
    }
};
}
