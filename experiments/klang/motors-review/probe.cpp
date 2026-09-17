#include <klang.h>
#include "../../../sounds/Motors.h"
#include <cstdio>
#include <cstdlib>
#include <memory>

// Diagnostic FourStroke variant: preserve RNG consumption but hold delay jitter fixed.
struct StableFourStroke : FourStrokeEngine {
    void process() override {
        signal n = noise >> lpf;
        n * 30 >> b;
        n * 0.5 >> a;

        signal n2 = noise >> bpf(200 + speed * 400);
        n2 = 1.0 - (speed + 0.1) * n2 * 0.01;

        const signal i = phasor * 4;
        const signal s = 22 - speed * 15;
        random(0.99, 1.0); // Preserve the original RNG sequence.
        const signal ms = fs / 250 * 0.995f;

        out = 0;
        for (int d = 0; d < 4; d++) {
            const param t = (d + 1) * 5 * ms;
            const param phase = -(0.75 - d * 0.25) * n2;
            signal mix = cos((a(t) + i + phase) * 2 * pi);
            mix *= b(t) + s;
            out += 1 / (mix * mix + 1);
        }
        out * speed * min(speed, 0.25) >> hpf >> dc >> out;
    }
};

struct StableCar : Sound {
    StableFourStroke engine;
    StableCar() { controls = { Dial("RPM", 0, 7000.f, 0.f) }; }
    void prepare() override { engine.set(controls[0] / 7000.f); }
    void process() override { engine >> out; 0.25 * tanh(out * 3) / tanh(3) >> out; }
};

template<class Model>
void configure(Model& model, int phase) {
    if constexpr (std::is_base_of_v<Sound, Model>)
        model.controls[0].set(phase == 1 ? 4500.f : 900.f);
    else
        model.set(phase == 1 ? .8f : .3f);
}

template<class Model>
int render(const char* path) {
    std::srand(12345);
    auto model = std::make_unique<Model>();
    FILE* file = std::fopen(path, "wb");
    if (!file) return 1;
    for (int frame = 0; frame < 48000 * 6; ++frame) {
        if (frame % 96000 == 0) configure(*model, frame / 96000);
        if constexpr (std::is_base_of_v<Sound, Model>)
            if (frame % 64 == 0) model->prepare();
        model->process();
        const float sample = model->out;
        if (std::fwrite(&sample, sizeof(sample), 1, file) != 1) return 2;
    }
    return std::fclose(file);
}

int main(int argc, char** argv) {
    if (argc != 5) return 2;
    fs = 48000;
    return render<FourStrokeEngine>(argv[1]) || render<StableFourStroke>(argv[2]) ||
        render<Car>(argv[3]) || render<StableCar>(argv[4]);
}
