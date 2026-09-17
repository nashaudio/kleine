// Standalone comparison host for unchanged example effects and synthesisers.
// Six seconds at default controls: two notes for synths, driven input then tail for effects.
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <type_traits>

template<class T>
int renderExample(int rate, const char* path) {
    constexpr bool stereo = std::is_base_of_v<klang::Stereo::Effect, T>;
    constexpr bool synth = std::is_base_of_v<klang::Synth, T> || std::is_base_of_v<klang::Stereo::Synth, T>;
    static_assert(stereo || std::is_base_of_v<klang::Effect, T>);
    klang::fs = rate;
    std::srand(12345);
    auto model = std::make_unique<T>();
    auto* file = std::fopen(path, "wb");
    if (!file) return 3;
    std::array<float,64> left{}, right{};
    for (int frame = 0; frame < rate * 6;) {
        const int remaining = rate * 2 - frame % (rate * 2);
        const int count = std::min(64, remaining);
        if constexpr (synth) {
            if (frame == 0) model->noteOn(48, .7f);
            if (frame == rate * 2) {
                model->noteOff(48, 0);
                model->noteOn(60, .6f);
            }
            if (frame == rate * 4) model->noteOff(60, 0);
        }
        for (int i=0; i<count; ++i) {
            const double t = double(frame+i)/rate;
            const double input = t < 4 ? .2*std::sin(6.283185307179586*220*t)
                + .1*std::sin(6.283185307179586*997*t) : 0;
            left[i] = synth ? 0 : float(input);
            right[i] = synth ? 0 : float(input*.7);
        }
        klang::Debug::Session debugSession(left.data(), count,
            synth ? klang::Debug::Buffer::Synth : klang::Debug::Buffer::Effect);
        if constexpr (stereo && synth) {
            float* channels[] = {left.data(),right.data()};
            static_cast<klang::Stereo::Synth&>(*model).process(channels,count);
        } else if constexpr (synth) {
            static_cast<klang::Synth&>(*model).process(left.data(),count);
        } else if constexpr (stereo) {
            klang::buffer l(left.data(),count), r(right.data(),count);
            klang::Stereo::buffer audio(l,r);
            static_cast<klang::Stereo::Effect&>(*model).process(audio);
        } else {
            klang::buffer audio(left.data(),count);
            static_cast<klang::Effect&>(*model).process(audio);
        }
        for(int i=0; i<count; ++i) {
            const float output[] = {left[i],right[i]};
            if(std::fwrite(output,sizeof(float),stereo ? 2 : 1,file) != (stereo ? 2u : 1u)) return 4;
        }
        frame += count;
    }
    std::printf("channels=%d\n",stereo ? 2 : 1);
    return std::fclose(file)==0 ? 0 : 4;
}

int main(int argc,char** argv) {
    if(argc!=3) return 2;
    const int rate=std::atoi(argv[1]);
    if(rate!=44100 && rate!=48000) return 2;
    return renderExample<Model>(rate,argv[2]);
}
