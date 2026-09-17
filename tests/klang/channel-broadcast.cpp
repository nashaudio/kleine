// A single mono signal broadcasts to every channel.
#include <klang.h>
#include <cstdio>
int main() {
    klang::signal mono = 2;
    klang::signals<3> channels(mono);
    std::printf("channels=%g,%g,%g\n", float(channels[0]), float(channels[1]), float(channels[2]));
    return channels[0] == 2 && channels[1] == 2 && channels[2] == 2 ? 0 : 1;
}
