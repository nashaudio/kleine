// Observe public state without changing the PhoneEffects model or scheduling.
#include "../../farnell/klang/Artificial Sounds/Phone Tones/phoneeffects.k"
#include <cstdio>
#include <cstring>
#include <memory>

int main(int argc, char** argv) {
    if (argc != 4) return 2;
    fs = std::stoi(argv[1]);
    const int frames = int(float(fs)) * std::stoi(argv[2]);
    auto phone = std::make_unique<farnell::PhoneEffects>();
    FILE* events = std::fopen(argv[3], "r");
    if (!events) return 2;
    int next = -1;
    char action[32];
    float value = 0;
    auto read = [&] { if (std::fscanf(events, "%d %31s %f", &next, action, &value) != 3) next = -1; };
    read();
    auto programme = [&](int frame) { std::printf("programme %d %g\n", frame, float(phone->alarm.duration)); };
    for (int frame = 0; frame < frames; ++frame) {
        while (frame == next) {
            if (!std::strcmp(action, "ringer")) { phone->ring(int(value)); programme(frame); }
            else if (!std::strcmp(action, "key")) phone->dialler.dial(farnell::DTMFTones::keys[int(value)]);
            else if (!std::strcmp(action, "source")) phone->source = int(value);
            read();
        }
        if (frame % 64 == 0) phone->prepare();
        const bool delayed = frame == phone->nextRinger;
        const bool reset = frame == phone->alarm.resetAt;
        const int ringer = phone->nextRinger;
        phone->process();
        if (delayed || (ringer != phone->nextRinger && phone->nextRinger >= 0)) programme(frame);
        if (reset) std::printf("reset %d 0\n", frame);
        if (phone->dialler.del.updated) std::printf("release %d 0\n", frame);
        for (int key : phone->decoder.messages) std::printf("digit %d %d\n", frame, key);
    }
    std::fclose(events);
}
