// Feed the actual decoder shared float samples; log control changes at block publication.
#include "../../farnell/klang/Artificial Sounds/DTMF Tones/dtmftones.k"
#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv) {
    if (argc == 1) {
        farnell::DTMFTones::Sand sand(1);
        const int events[][2] = {{0,0},{1,0},{0,1},{1,1},{1,1},{0,1},{1,0},
                                 {1,1},{0,0},{0,1},{0,0},{1,0},{1,1},{0,1}};
        for (auto& event : events)
            if (sand.send(event[0], float(event[1]))) std::printf("%d\n", sand.out);
        return 0;
    }
    if (argc != 3) return 2;
    fs = std::atoi(argv[1]);
    FILE* input = std::fopen(argv[2], "rb");
    if (!input) return 2;
    farnell::DTMFTones::Decoder decoder;
    farnell::DTMFTones::DigitDecoder bank;
    decoder.prepare();
    bank.prepare();
    bool previous[8] = {};
    float sample;
    int frame = 0;
    while (std::fread(&sample, sizeof sample, 1, input) == 1) {
        decoder.in = sample;
        decoder.process();
        bank.in = sample;
        bank.process();
        for (char key : decoder.messages) std::printf("standalone %d %c\n", frame, key);
        for (int key : bank.messages) std::printf("bank-key %d %d\n", frame, key);
        for (int i = 0; i < 8; ++i) {
            bool active = decoder.detectors[i].active;
            if (active != previous[i]) std::printf("detector%d %d %d\n", i, frame, active);
            previous[i] = active;
        }
        ++frame;
    }
    std::fclose(input);
}
