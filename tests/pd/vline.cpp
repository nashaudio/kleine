// Isolated message driver: delivery frame, logical time (ms), inlet, arity, values.
#include <klang/pd.h>
#include <fstream>
#include <sstream>
#include <cassert>

int main(int argc, char** argv) {
    if (argc != 5) return 2;
    fs = std::stoi(argv[1]);
    const int frames = std::stoi(argv[2]);
    struct Event {
        int frame, inlet, count;
        double time;
        float a = 0, b = 0, c = 0;
    };
    std::vector<Event> events;
    std::ifstream source(argv[3]);
    std::string row;
    while (std::getline(source, row)) {
        std::istringstream fields(row);
        Event event{};
        fields.operator>>(event.frame).operator>>(event.time).operator>>(event.inlet)
            .operator>>(event.count).operator>>(event.a).operator>>(event.b).operator>>(event.c);
        if (!fields) return 3;
        events.push_back(event);
    }
    if (!source.eof()) return 4;
    pd::vline line;
    Generator& object = line; // Exercise Klang's one/two/three-argument dispatch.
    std::vector<float> audio(frames);
    size_t index = 0;
    double logicalClock = 0;
    for (int frame = 0; frame < frames; ++frame) {
        if (frame % 64 == 0) {
            // Explicit PD host adapter: m_sched.c tick and d_ctl.c block anchor.
            // Keep this outside the sample-timed primitive and the pure models.
            // st_dacsr is t_float: retain the scheduler's float division.
            logicalClock += double(64 / float(fs)) * (32. * 441. * 1000.);
            line.sync(logicalClock / (32. * 441.) - 64 * (1000.0 / double(float(fs))));
        }
        while (index < events.size() && events[index].frame == frame) {
            const auto& e = events[index++];
            line.messageTime(e.time);
            if (e.inlet == 1) line.duration(e.a);
            else if (e.inlet == 2) line.delay(e.a);
            else if (e.inlet == 3) line.stop();
            else if (e.count == 1) object(e.a);
            else if (e.count == 2) object(e.a, e.b);
            else object(e.a, e.b, e.c);
        }
        audio[frame] = float(signal(line));
        if (!std::isfinite(audio[frame])) return 5;
    }
    if (index != events.size()) return 6;
    std::ofstream output(argv[4], std::ios::binary);
    output.write(reinterpret_cast<const char*>(audio.data()), audio.size() * sizeof(float));
    if (!output) return 7;

    // Sanitisation and ordinary untimestamped use, independently of the PD host.
    pd::vline ordinary;
    ordinary.set(std::numeric_limits<float>::infinity());
    ordinary.process();
    assert(ordinary.out == 0);
    ordinary.set(std::numeric_limits<float>::quiet_NaN());
    ordinary.process();
    assert(ordinary.out == 0);
    ordinary.set(1);
    ordinary.set(0, 2);
    for (int i = 0; i < int(float(fs) * .003f); ++i) ordinary.process();
    assert(ordinary.out == 0);

    // Bouncing's zero-height tail: a deferred jump and the next attack coincide.
    // This catches sample-clock drift changing queue replacement at that instant.
    if (float(fs) == 48000) {
        pd::vline tail;
        for (int frame = 0; frame < 480000; ++frame) {
            if (frame % 48 == 0) {
                tail.set(1, 1);
                tail.set(0, 0, 1);
            }
            tail.process();
            assert(std::fabs(float(tail.out) - float(frame % 48 + 1) / 48) < 1e-6f);
        }
        assert(tail.time() == 10000);
        tail.set(1, 1);
        tail.set(0, 200, 1);
        tail.process();
        assert(std::fabs(float(tail.out) - 1.f/48) < 1e-6f);
    }
}
