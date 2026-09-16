#pragma once

namespace validation {
// Exercise the standalone BELL abstractions with their own gains and envelopes.
struct BellStage : farnell::BellStudies::Partials {
    std::string stage;
    explicit BellStage(const std::string& value) : stage(value) {
        if (stage == "a0") { count = 1; oscillator[0].set(440); amplitude[0] = .3f; }
    }
    void trigger() {
        if (stage == "a1") {
            count = 1;
            envelope[0].set(1);
            envelope[0].set(0, 800);
        }
        else if (stage == "partial") { count = 1; gain = 1; partial(0,521,.7f,800); }
        else if (stage == "group") { gain = .3333f; partial(0,521,.7f,800);partial(1,732,.45f,500);partial(2,934,.25f,200); }
        else { gain = stage == "a2" ? .5f : .333f * .3f; Partials::trigger(); }
    }
    void process() override {
        if (stage == "a0") { signal tone = oscillator[0]; out = tone * .3f; }
        else if (stage == "a1") { signal e = envelope[0]; out = e*e; }
        else Partials::process();
    }
};

// Expose the bouncing envelope and control height independently of oscillator phase.
struct BouncingStage : farnell::Bouncing {
    bool envelope;
    explicit BouncingStage(bool value) : envelope(value) {}
    void process() override { Bouncing::process(); out = envelope ? impact.out : line.out; }
};

// Expose Boing's pitch envelope, phase drive and modal contributions for comparison.
struct BoingStage : farnell::Boing {
    std::string stage;
    explicit BoingStage(const std::string& value) : stage(value) {}
    void process() override {
        Boing::process();
        if (stage == "phase") out = phase.out;
        else if (stage == "pitch") { out = pitch.out; for (int i=0;i<6;++i) out *= out; }
        else if (stage == "clamped") out = clamped.out;
        else if (stage == "free") out = free.out;
        else if (stage == "frequency") out = free.frequency;
    }
};

// Separate the creak's stochastic pulse timing from its fixed wood/panel response.
struct CreakingStage : farnell::Creaking {
    void process() override { Creaking::process(); out = stickslip.out; }
};

// Unit-impulse checks for dfbef and the complete material/panel networks.
struct CreakingImpulse : Sound {
    farnell::Creaking::Reflection reflection;
    farnell::Creaking::Wood wood;
    farnell::Creaking::Panel panel;
    std::string stage;
    int frame = 0;
    explicit CreakingImpulse(const std::string& name) : stage(name) {}
    void prepare() override { reflection.prepare();wood.prepare();panel.prepare(); }
    void process() override {
        signal impulse = frame++ == 0 ? 1.0f : 0.0f;
        if (stage == "delay") impulse >> reflection >> out;
        else if (stage == "wood") impulse >> wood >> out;
        else impulse >> panel >> out;
    }
};
}
