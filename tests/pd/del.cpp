// Exercise the actual control-clock port; events are applied before each sample poll.
#include <klang/pd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int argc, char** argv) {
    if (argc != 6) return 2;
    fs = std::atoi(argv[1]);
    pd::del del(float(std::atof(argv[3])), float(std::atof(argv[4])), argv[5]);
    FILE* file = std::fopen(argv[2], "r");
    if (!file) return 2;
    int next = -1;
    float value = 0;
    char action[32], unit[32];
    auto read = [&] {
        if (std::fscanf(file, "%d %31s %f %31s", &next, action, &value, unit) != 4) next = -1;
    };
    read();
    for (int frame = 0; frame < int(float(fs)); ++frame) {
        while (frame == next) {
            if (!std::strcmp(action, "bang")) del.bang();
            else if (!std::strcmp(action, "hot")) del = value;
            else if (!std::strcmp(action, "cold")) del.set(value);
            else if (!std::strcmp(action, "stop")) del.stop();
            else if (!std::strcmp(action, "tempo")) del.tempo(value, unit);
            else return 3;
            read();
        }
        if (del()) std::printf("%d\n", frame);
    }
    std::fclose(file);
}
