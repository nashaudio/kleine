// Verify the new mode-selection API through Klang's virtual setter interface.
#include <klang/pd.h>
#include <cassert>
#include <cmath>

int main() {
    fs = 48000;
    pd::line ramp;
    Generator& object = ramp;
    object(1, 4, 1); // Four-millisecond control ramp, one-millisecond grain.
    for (int i = 0; i < 192; ++i) {
        ramp.process();
        assert(std::fabs(float(ramp.out) - (i / 48) * .25f) < 1e-6f);
        assert(ramp.updated == (i % 48 == 0));
    }
    object(0, 4); // At an audio block boundary; resume from held 0.75.
    assert(!ramp.control);
    for (int i = 0; i < 192; ++i) {
        ramp.process();
        assert(std::fabs(float(ramp.out) - (.75f * (192 - i) / 192)) < 1e-5f);
        assert(!ramp.updated);
    }
    ramp.process();
    assert(ramp.out == 0);

    pd::line assigned, configured, messages;
    assigned = { {0, 1}, {3000, 0}, 20 };
    configured.set({0, 1}, {3000, 0}, 20);
    messages.set(1, 0, 20);
    messages.set(0, 3000, 20);
    for (int i = 0; i <= 144000; ++i) {
        assigned.process();
        configured.process();
        messages.process();
        assert(assigned.out == messages.out && configured.out == messages.out);
        assert(assigned.updates == messages.updates && configured.updates == messages.updates);
    }
    assert(assigned.out == 0);
    assigned = { {0, 1}, {4, 0} }; // Omitted grain selects audio mode.
    assert(!assigned.control);
    bool rejected = false;
    try { assigned.set({10, 1}, {20, 0}, 5); }
    catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected && !assigned.control);
}
