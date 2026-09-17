#pragma once
#include "scheduler.h"
#include "../pd-block/prototypes.h"

namespace scheduling_trial {

// A sample-timed alarm: the callback owns the counter, process() owns the audio.
struct Alarm : Sound {
    pd::osc osc;
    klang::signal gate = 0;
    int count = 0;
    void prepare() override { every(300, &Alarm::tick, First::immediately); }
    void tick() { gate = count++ % 2; }
    void process() override { osc(800) * gate * 0.2f >> out; }
};

// An explicit PD control window; all messages for [blockStart, blockEnd) run first.
// Metro here is the earlier experimental clock, not a replacement for pd::metro.
struct BlockAlarm : Sound {
    pd::osc osc;
    block_trial::Metro metro{300};
    klang::signal gate = 0;
    bool started = false;
    void prepare() override {
        if (!started) {
            metro.start(true, [&](int count) { gate = count % 2; });
            started = true;
        }
        every(samples(64), &BlockAlarm::block, First::immediately);
    }
    void block() { metro.advance(64, [&](int count) { gate = count % 2; }); }
    void process() override { osc(800) * gate * 0.2f >> out; }
};

// A slower configuration stage, called once per 5 ms instead of once per sample.
struct SparseFilter : Effect {
    pd::lop filter;
    klang::param cutoff = 1000;
    void prepare() override { every(5, &SparseFilter::configure, First::immediately); }
    void configure() { filter.set(cutoff); }
    void process() override { in >> filter >> out; }
};

// A parent schedules child configuration on the same clock, leaving sample routing ordinary.
struct Parent : Sound {
    // Held control value consumed through normal nested Klang evaluation.
    struct Child : klang::Sound {
        int ticks = 0;
        void tick() { ++ticks; }
        void process() override { out = float(ticks); }
    } child;
    void prepare() override { every(100, child, &Child::tick); }
    void process() override { child >> out; }
};

} // namespace scheduling_trial
