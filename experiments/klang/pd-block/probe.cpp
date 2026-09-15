// Focused semantic checks for the design paper, without changing Klang or production models.
#include "prototypes.h"
#include "../../../farnell/klang/Artificial Sounds/Pedestrians/pedestrians.k"
#include <fstream>
#include <iostream>
#include <numeric>

using namespace block_trial;

// Capture the sample positions at which automatic block callbacks occur.
template<class Base> struct Counter : Base {
    int frame = 0, preparations = 0;
    std::vector<int> boundaries;
    void prepare() override { ++preparations; }
    event block() override { boundaries.push_back(frame); }
    void process() override { this->out = float(frame++); }
};

// Exercise callback invocation with a named member and with a captured lambda.
struct Receiver { int calls = 0; void tick() { ++calls; } };

void require(bool condition, const char* description) {
    if (!condition) throw std::runtime_error(description);
}

template<class S> void host_buffer(S& sound, int samples) {
    std::vector<float> audio(samples);
    static_cast<Sound&>(sound).process(klang::buffer(audio.data(), samples));
}

template<class S> std::vector<float> render(int rate, std::vector<int> chunks, bool controls, bool nested) {
    fs = rate;
    auto instance = std::make_unique<S>();
    S& sound = *instance;
    sound.start();
    std::vector<std::pair<int,bool>> events;
    if (controls) {
        const double seconds[] = {0,.25,.9,1.5,2.2,2.6};
        const bool enabled[] = {false,true,false,true,false,true};
        for (int i=0;i<6;++i) events.push_back({int(std::llround(seconds[i]*rate))/64*64,enabled[i]});
    }
    std::vector<float> audio(rate * 4);
    int position = 0, chunk = 0, eventIndex = 0;
    while (position < int(audio.size())) {
        while (eventIndex < int(events.size()) && events[eventIndex].first == position)
            sound.start(events[eventIndex++].second);
        int count = std::min(chunks[chunk++ % chunks.size()], int(audio.size()) - position);
        if (eventIndex < int(events.size())) count = std::min(count,events[eventIndex].first-position);
        if (nested) {
            sound.prepare(); // The parent still prepares nested components once per host buffer.
            for (int i=0;i<count;++i) {
                signal value = static_cast<Sound&>(sound);
                audio[position+i] = value;
            }
        } else static_cast<Sound&>(sound).process(klang::buffer(audio.data()+position,count));
        position += count;
    }
    return audio;
}

void raw(const std::string& path,const std::vector<float>& values) {
    std::ofstream output(path,std::ios::binary);
    output.write(reinterpret_cast<const char*>(values.data()),values.size()*sizeof(float));
    require(bool(output),"could not write scratch audio");
}

int main() {
    try {
        auto absentStorage = std::make_unique<BufferSound>(); auto& absent = *absentStorage;
        host_buffer(absent,100); host_buffer(absent,28);
        require(!absent.blocks.enabled && absent.blocks.samples()==64,"default handler did not disable itself");
        auto alwaysStorage = std::make_unique<AlwaysSound>(); auto& always = *alwaysStorage;
        host_buffer(always,128); require(always.blocks.enabled,"empty handler unexpectedly disabled blocking");
        auto counterStorage = std::make_unique<Counter<BufferSound>>(); auto& counter = *counterStorage;
        host_buffer(counter,100); host_buffer(counter,28); host_buffer(counter,3);
        require(counter.boundaries==std::vector<int>({0,64,128}),"host partition moved block boundaries");
        require(counter.preparations==3,"prepare frequency changed");
        counter.blocks.set(0); host_buffer(counter,17);
        require(counter.boundaries.size()==3 && counter.frame==148,"zero should disable hooks, not samples");
        counter.blocks.set(128); host_buffer(counter,129);
        require(counter.boundaries==std::vector<int>({0,64,128,148,276}),"re-enable/resize phase contract changed");
        auto shortBlockStorage = std::make_unique<Counter<BufferSound>>(); auto& shortBlock = *shortBlockStorage; shortBlock.blocks.set(1); host_buffer(shortBlock,5);
        require(shortBlock.boundaries==std::vector<int>({0,1,2,3,4}),"size one failed");
        bool invalid=false;
        try { Block b; b.set(3.5f); } catch(const std::invalid_argument&) { invalid=true; }
        require(invalid,"fractional block size was accepted");
        Receiver a,b; Block callA,callB;
        for (int i=0;i<130;++i) { callA(&Receiver::tick,&a); callB([&]{b.tick();}); }
        require(a.calls==3 && b.calls==3,"callback/lambda overload failed");
        Metro fast{1}; int last=-1,ticks=0; fs=48000;
        fast.start(true,[&](int n){last=n;++ticks;});
        fast.advance(128,[&](int n){last=n;++ticks;});
        require(last==2 && ticks==3,"metro lost events within a block");
        Metro exact{100}; exact.start(true,[](int){}); int arrived=0;
        exact.advance(4800,[&](int){++arrived;}); require(arrived==0,"right boundary must be excluded");
        exact.advance(1,[&](int){++arrived;}); require(arrived==1,"next block must include its left boundary");
        int comparisons=0;
        for (int rate : {48000,44100}) for (bool controls : {false,true}) {
            const auto reference=render<farnell::Pedestrians>(rate,{64},controls,false);
            raw("build/pd-block/reference-"+std::to_string(rate)+(controls?"-controls":"")+".f32",reference);
            for (auto chunks : {std::vector<int>{64},std::vector<int>{1,3,17,79,100,257},std::vector<int>{1024}}) {
                require(render<MemberPedestrians>(rate,chunks,controls,false)==reference,"member audio mismatch");
                require(render<EquippedPedestrians>(rate,chunks,controls,false)==reference,"inherited audio mismatch");
                require(render<EventPedestrians>(rate,chunks,controls,false)==reference,"automatic buffer audio mismatch");
                require(render<SamplePedestrians>(rate,chunks,controls,false)==reference,"automatic sample audio mismatch");
                require(render<EquippedPedestrians>(rate,chunks,controls,true)==reference,"nested inherited audio mismatch");
                require(render<SamplePedestrians>(rate,chunks,controls,true)==reference,"nested automatic audio mismatch");
                comparisons+=6;
            }
            require(render<EventPedestrians>(rate,{64},controls,true)!=reference,"expected buffer-only nested failure did not appear");
            if (!controls) raw("build/pd-block/ideal-"+std::to_string(rate)+".f32",render<IdealPedestrians>(rate,{100,28},false,false));
        }
        std::cout << "Lifecycle, callback, deadline and 72 audio comparisons passed; buffer-only nested failure reproduced.\n";
        return 0;
    } catch(const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
