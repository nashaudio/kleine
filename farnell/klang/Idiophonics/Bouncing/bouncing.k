#pragma once

// Andy Farnell, Designing Sound (2010), chapter 30, figure 30.2.
#include <klang/pd.h>

namespace farnell {
using namespace klang::optimised;
/** @brief Shrinking bounce intervals and envelopes drive a frequency-modulated ball.
 * @code
 * auto* ball = new farnell::Bouncing;
 * processor.attach(ball);
 * ball->trigger();
 * @endcode
 * The height ramp lasts 3000 ms with [line]'s 20 ms control grain.
 */
struct Bouncing : Sound {
    pd::osc carrier, modulation;
    pd::metro metro;
    pd::line line;
    pd::vline impact;
    void trigger() {
        line = { {0, 1}, {3000, 0}, 20 };
        metro = 1;
    }
    void process() override {
        signal height = line;
        if (metro(height * 300)) {
            impact.set(1, 1);
            impact.set(0, height * 200, 1);
        }
        signal e = impact, squared = e ^ 2;
        signal shape = squared * squared;
        signal hz = shape * (height * 130) + 80 + (height * 70) * modulation(120);
        squared * carrier(hz) * height * .2f >> out;
    }
};
}
