#pragma once
#include <klang/pd.h>

namespace validation {
using namespace klang::optimised;

// Encode every metro bang as a cumulative count for timing comparisons.
struct Metro : Sound {
    pd::metro metro;
    param interval = 100;
    std::string name;
    int frame = 0, count = 0;

    explicit Metro(const std::string& test) : name(test) {
        if (name == "metro-fractional") interval = 37.5f;
        if (name == "metro-fast" || name == "metro-legacy") interval = .25f;
        if (name == "metro-zero") interval = 0;
        if (name == "metro-negative") interval = -5;
        if (name == "metro-legacy") metro.legacy = true;
        if (name != "metro-stopped") metro = param(1);
    }
    void process() override {
        if (name == "metro-controls") {
            if (frame == 3 * 64) interval = .25f;
            if (frame == 100 * 64) metro = 0;
            if (frame == 130 * 64) metro = -2;
            if (frame == 150 * 64) metro.bang();
            if (frame == 170 * 64) interval = 0;
            if (frame == 250 * 64) metro.stop();
        }
        count += metro(interval);
        out = count;
        ++frame;
    }
};
}
