// Control/retrigger and existing Harrier smoke checks after changing shared PD filters.
#include <memory>
#include <cstdio>
#include "../../farnell/klang/Idiophonics/Bouncing/bouncing.k"
#include "../../farnell/klang/Idiophonics/Bouncing/variants/bouncing-power.k"
#include "../../farnell/klang/Idiophonics/Rolling/rolling.k"
#include "../../farnell/klang/Idiophonics/Creaking/creaking.k"
#include "../../farnell/klang/Idiophonics/Boing/boing.k"
#include "../../sounds/Harrier.h"

template<class Model, class Control>
bool check(const char* name, Control control) {
    auto model = std::make_unique<Model>();
    double energy = 0;
    float peak = 0;
    const int frames = int(float(fs)) * 5;
    for (int frame = 0; frame < frames; ++frame) {
        if (frame % 64 == 0) { control(*model, frame); model->prepare(); }
        model->process();
        if constexpr (std::is_base_of_v<farnell::Bouncing, Model>) {
            if (!std::isfinite(model->carrier.frequency)) return false;
        }
        const float sample = model->out;
        if (!std::isfinite(sample)) return false;
        peak = std::max(peak, std::fabs(sample)); energy += double(sample) * sample;
    }
    printf("%s %d Hz: peak %.6f RMS %.6f\n", name, int(float(fs)), peak, std::sqrt(energy/frames));
    return peak > 0 && energy > 0;
}

int main() {
    for (int rate : {48000,44100}) {
        fs = rate;
        // Inline calls dispatch by arity; C++ default arguments alone do not supply
        // the shorter virtual setter. Exercise each public control form explicitly.
        {
            auto boingInstance = std::make_unique<farnell::Boing>();
            auto& boing = *boingInstance; boing.prepare();
            signal wave = boing(220);
            if (boing.frequency != 220) { printf("Boing one-argument frequency: %g\n",float(boing.frequency)); return 1; }
            wave = boing(330,3);
            if (boing.frequency != 330 || boing.depth != 3) { printf("Boing two-argument controls: %g %g\n",float(boing.frequency),float(boing.depth)); return 1; }
            auto creakInstance = std::make_unique<farnell::Creaking>();
            auto& creak = *creakInstance; creak.prepare(); wave = creak(.7f);
            if (!creak.stickslip.line.control) { printf("Creaking force must use a control ramp\n"); return 1; }
            auto canInstance = std::make_unique<farnell::Rolling>();
            auto& can = *canInstance; can.prepare(); wave = can(1);
            if (can.struck != 1) { printf("Rolling struck mode: %g\n",float(can.struck)); return 1; }
        }
        if (!check<farnell::Bouncing>("bouncing retrigger", [](auto& s, int f) {
            if (f == 0 || f == 128 || f == 4096) s.trigger();
        })) return 1;
        if (!check<farnell::variants::BouncingPower>("bouncing power retrigger", [](auto& s, int f) {
            if (f == 0 || f == 128 || f == 4096) s.trigger();
        })) return 1;
        if (!check<farnell::Rolling>("rolling repeated push", [](auto& s, int f) {
            if (f == 0 || f == 128 || f == 8192) s.trigger();
        })) return 1;
        if (!check<farnell::Creaking>("creaking force retarget", [](auto& s, int f) {
            if (f == 0) s.set(.9f);
            if (f == 640) s.set(.4f);
            if (f == 1280) s.set(1);
            if (f == 32768) s.set(0);
        })) return 1;
        if (!check<farnell::Boing>("boing retune/retrigger", [](auto& s, int f) {
            if (f == 0) { s.set(800,6);s.trigger(); }
            if (f == 256) { s.set(20,1);s.trigger(); }
        })) return 1;
        if (!check<Harrier>("Harrier shared-filter smoke", [](auto& s, int f) {
            if (f == 0) s.controls.set(.6f,.5f,20.0f,.1f);
            if (f == 32768) s.controls.set(.95f,.5f,1000.0f,.2f);
        })) return 1;
    }
}
