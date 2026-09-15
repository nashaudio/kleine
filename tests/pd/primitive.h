#pragma once

#include <klang/pd.h>
#include "../../farnell/klang/Idiophonics/Telephone Bell/telephonebell.k"

namespace validation {
using namespace klang::optimised;

// One isolated primitive, with an impulse or deterministic control signal.
struct Primitive : Sound {
    std::string name;
    pd::osc oscillator, modulator;
    pd::hip highpass;
    pd::lop lowpass;
    pd::bpf bandpass;
    pd::noise noise;
    pd::phasor phase;
    pd::cos cosine;
    pd::wrap wrapping;
    pd::line ramp;
    pd::env meter;
    farnell::TelephoneBell::Decay envelope;
    int frame = 0;

    explicit Primitive(const std::string& name) : name(name) {
        noise.seed(404933);
        if (name == "osc") oscillator.set(440);
        else if (name == "osc-300") oscillator.set(300);
        else if (name == "osc-phase") oscillator.set(440, 0.25f);
        else if (name == "osc-negative") oscillator.set(-440);
        else if (name == "osc-fm") modulator.set(300);
        else if (name == "hip") highpass.set(90);
        else if (name == "hip-zero") highpass.set(0);
        else if (name == "hip-high") highpass.set(2000);
        else if (name == "hip-legacy") { highpass.set(90); highpass.legacy = true; }
        else if (name == "lop") lowpass.set(100);
        else if (name == "bp-wire") bandpass.set(2000, 12);
        else if (name == "bp-speaker") bandpass.set(400, 7);
        else if (name == "vline-decay") envelope.trigger(10);
        else if (name == "phasor") phase.set(440);
        else if (name == "phasor-negative") { phase.set(-440); phase.phase(0.25f); }
        else if (name == "cos" || name == "wrap") phase.set(-440);
        else if (name == "line") ramp.set(1, 1);
        else if (name == "env") oscillator.set(697);
        else if (name != "noise") throw std::runtime_error("unknown primitive");
    }

    void process() override {
        signal impulse = frame++ == 0 ? 1.0f : 0.0f;
        if (name == "osc-fm") {
            signal modulation = modulator;
            signal carrier = oscillator(1000 + modulation * 2000);
            carrier >> out;
        } else if (name.rfind("osc", 0) == 0) oscillator >> out;
        else if (name.rfind("hip", 0) == 0) impulse >> highpass >> out;
        else if (name == "lop") impulse >> lowpass >> out;
        else if (name.rfind("bp", 0) == 0) impulse >> bandpass >> out;
        else if (name == "noise") noise >> out;
        else if (name == "env") { signal wave = oscillator; wave * 0.25f >> meter >> out; }
        else if (name.rfind("phasor", 0) == 0) phase >> out;
        else if (name == "cos" || name == "wrap") {
            signal x = phase;
            if (name == "cos") x * 4 - 2 >> cosine >> out;
            else x * 4 - 2 >> wrapping >> out;
        } else if (name == "line") {
            const int sample = frame - 1;
            if (sample == int(float(fs) * .1f) / 64 * 64) ramp.set(-0.5f, 150);
            if (sample == int(float(fs) * .175f) / 64 * 64) ramp.set(0.75f, 80);
            if (sample == int(float(fs) * .2f) / 64 * 64) ramp.stop();
            if (sample == int(float(fs) * .3f) / 64 * 64) ramp.set(0);
            ramp >> out;
        }
        else envelope >> out;
    }
};
}
