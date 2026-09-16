// Observe actual env publications and verify the streamed and stored outputs agree.
#include <klang/pd.h>
#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv) {
    if (argc != 3) return 2;
    fs = std::atoi(argv[1]);
    FILE* input = std::fopen(argv[2], "rb");
    if (!input) return 2;
    pd::env env;
    float sample, previous = 0;
    int frame = 0;
    while (std::fread(&sample, sizeof sample, 1, input) == 1) {
        signal value = signal(sample) >> env;
        if (float(value) != float(env.out) || (!env.updated && float(value) != previous)) return 3;
        if (env.updated) std::printf("%d %.9g\n", frame, float(value));
        previous = value;
        ++frame;
    }
    std::fclose(input);
}
