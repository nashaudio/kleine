#pragma once
#include <klang/pd.h>

namespace validation {
// Exercise [line] messages independently of the procedural sound models.
struct ControlLine : Sound {
    pd::line line;
    std::string name;
    int frame = 0, count = 0;
    explicit ControlLine(const std::string& test) : name(test) {}
    void process() override {
        if (frame == 0) {
            line.legacy = name == "ramp-legacy";
            line.set(name == "ramp-repeat" ? 0 : 1, 113,
                     name == "ramp-zero-grain" ? 0 : name == "ramp-negative-grain" ? -3
                     : name == "ramp-fast" ? .25f : 20);
        }
        if (name == "ramp-retarget") {
            if (frame == 17 * 64) line.set(-.4f, 83, 13);
            if (frame == 54 * 64) line.set(.7f, 53, 7);
        }
        if (name == "ramp-stop" || name == "ramp-legacy") {
            if (frame == 23 * 64) line.stop();
            if (frame == 51 * 64) line.set(-.5f, 71, 20);
        }
        if (name == "ramp-reset") {
            if (frame == 23 * 64) line.reset(.4f);
            if (frame == 51 * 64) line.set(-.5f, 71, 20);
        }
        if (name == "ramp-inlets") {
            if (frame == 17 * 64) {
                line.grain(7);
                line.duration(83);
                line.set(-.4f);
            }
            if (frame == 54 * 64) line.set(.7f); // Consumed duration: jump.
            if (frame == 77 * 64) {
                line.duration(-1);
                line.set(.2f);
            }
        }
        out = line;
        count += line.updates;
        if (name == "ramp-repeat") out = count;
        ++frame;
    }
};
}
