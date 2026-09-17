// Included after one unchanged sound source and a Model alias by check_sounds.py.
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <type_traits>

void configure(Model& model, int phase) {
#if SOUND_KIND == 0 // Bicycle: pedalling, coasting, pedalling again
    model.controls[0].set(phase == 1 ? 0.f : .8f);
    model.controls[1].set(phase == 1 ? 12.f : 6.f);
    model.controls[2].set(phase == 2 ? 2.f : 1.f);
#elif SOUND_KIND == 1 // Harrier
    model.controls[0].set(phase == 1 ? .95f : .6f);
    model.controls[1].set(.5f);
    model.controls[2].set(phase == 1 ? 1000.f : 20.f);
    model.controls[3].set(phase == 1 ? .2f : .1f);
#elif SOUND_KIND == 2 // Helicopter
    model.controls[0].set(phase == 1 ? .95f : .6f);
    model.controls[1].set(phase == 1 ? 1000.f : 20.f);
    model.controls[2].set(0);
#elif SOUND_KIND == 3 // Mini: ignition, acceleration, ignition off/tail
    model.controls[0].set(phase == 2 ? 0.f : 1.f);
    model.controls[1].set(phase == 1 ? 4500.f : 900.f);
    model.controls[2].set(phase == 1 ? .8f : .1f);
    model.controls[3].set(phase == 1 ? 2.f : 0.f);
#elif SOUND_KIND == 4 // Car
    model.controls[0].set(phase == 1 ? 4500.f : 900.f);
#elif SOUND_KIND == 5 // ToyBoatEngine: regular/broken/regular
    model.set(phase == 1 ? 1.f : 0.f);
#elif SOUND_KIND == 6 // FourStrokeEngine
    model.set(phase == 1 ? .8f : .3f);
#elif SOUND_KIND == 8 // Train reconstruction
    model.controls[0].set(phase == 1 ? 30.f : 5.f);
    model.controls[1].set(0);
    model.controls[2].set(1);
    model.controls[3].set(phase == 1 ? 1.f : 0.f);
    model.controls[4].set(phase == 2 ? 0.f : 1.f);
#endif
}

template<class T> void prepare(T& model) {
    if constexpr (std::is_base_of_v<klang::Effect, T> ||
                  std::is_base_of_v<klang::Stereo::Effect, T> ||
                  std::is_base_of_v<klang::Sound, T> ||
                  std::is_base_of_v<klang::Stereo::Sound, T>) model.prepare();
}

int main(int argc, char** argv) {
    if (argc != 3) return 2;
    const int rate = std::atoi(argv[1]);
    if (rate != 44100 && rate != 48000) return 2;
    klang::fs = rate;
    std::srand(12345);
    auto model = std::make_unique<Model>();
    FILE* file = std::fopen(argv[2], "wb");
    if (!file) return 3;
    for (int frame = 0; frame < rate * 6; ++frame) {
        if (frame % (rate * 2) == 0) configure(*model, frame / (rate * 2));
        if (frame % 64 == 0) prepare(*model);
        model->process();
#if SOUND_KIND == 8
        const float samples[] = {model->out.l, model->out.r};
#else
        const float samples[] = {model->out};
#endif
        if (std::fwrite(samples, sizeof(float), sizeof(samples) / sizeof(float), file)
            != sizeof(samples) / sizeof(float)) return 4;
    }
    return std::fclose(file) == 0 ? 0 : 4;
}
