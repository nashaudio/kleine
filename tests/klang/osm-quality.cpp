// Quality and lifecycle gate for the default signed-frequency V2 oscillators.
// Accuracy is measured against independent integrals, without requiring identity.
#include "oscillator-reference.h"
#include <cstdio>
#include <limits>

// The header above uses the selected build flags; measurements must stay precise.
#pragma float_control(precise, on, push)

namespace fast = klang::Generators::Fast;
using namespace oscillator_reference;
static int checks = 0, failures = 0, fixtures = 0;
static double worst = 0, squares = 0, peak = 0;
static double requestedAudioWorst = 0, requestedEdgeWorst = 0;
static int edgeRate = 0, edgeSample = 0;
static float edgeHz = 0;
static double edgeDuty = 0;
static bool edgePulse = false;
static long long samples = 0;

// Construct exceptional inputs here under precise semantics. The standard
// library's infinity()/quiet_NaN() definitions were included under build flags.
float exceptional(std::uint32_t bits) {
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

void check(bool pass, const char* message) {
    ++checks;
    if (!pass && failures++ < 12) std::printf("FAIL %s\n", message);
}

template<class Osc>
void compare(Osc& osc, std::uint32_t& phase, float hz, int rate, double duty,
             bool pulse, int frames, std::uint32_t offset = 0) {
    double delta;
    std::uint32_t advance;
#ifdef OSM_MULTICYCLE
    if (std::fabs(double(hz)) >= rate) {
        const double magnitude = std::fabs(double(hz));
        const auto fraction = std::uint32_t(std::floor(std::fmod(magnitude, double(rate)) / rate * turn));
        delta = std::copysign(std::floor(magnitude / rate) + double(fraction) / turn, double(hz));
        advance = hz < 0 ? 0u - fraction : fraction;
    } else
#endif
    {
        const auto ticks = step(hz, rate);
        delta = double(ticks) / turn;
        advance = static_cast<std::uint32_t>(ticks);
    }
    const double requestedDuty = duty;
#ifndef OSM_FRACTIONAL_DUTY
    // The fast breakpoint, like phase, has a 32-bit fractional resolution.
    duty = std::floor(duty * turn) / turn;
#endif
    for (int i = 0; i < frames; ++i) {
        const double p = double(std::uint32_t(phase + offset)) / turn;
        const double expected = delta ? (primitive(p, duty, pulse) - primitive(p - delta, duty, pulse)) / delta
                                     : point(p, duty, pulse);
        osc.process();
        const double actual = float(osc.out), error = std::fabs(actual - expected);
        worst = std::max(worst, error);
        squares += error * error;
        peak = std::max(peak, std::fabs(actual));
        ++samples;
        if ((!std::isfinite(actual) || error > 1e-5 || std::fabs(actual) > 1.00001) && failures < 12)
            std::printf("rate=%d hz=%.9g requested_hz=%.9g duty=%.12g pulse=%d sample=%d actual=%.12g expected=%.12g\n",
                        rate, hz, float(osc.frequency), duty, pulse, i, actual, expected);
        check(std::isfinite(actual) && error <= 1e-5, "interval error <= 1e-5 full scale");
        check(std::fabs(actual) <= 1.00001, "bounded waveform");
        if (delta) {
            const double requested = (primitive(p, requestedDuty, pulse) - primitive(p - delta, requestedDuty, pulse)) / delta;
            const double requestedError = std::fabs(actual - requested);
            if (std::fabs(hz) >= 20 && (duty == 0 || duty == 1 || (requestedDuty >= .001 && requestedDuty <= .999))) {
                requestedAudioWorst = std::max(requestedAudioWorst, requestedError);
                check(requestedError <= 1e-5, "ordinary audio error against unquantised duty");
            } else if (requestedError > requestedEdgeWorst) {
                requestedEdgeWorst = requestedError;
                edgeRate = rate; edgeSample = i; edgeHz = hz;
                edgeDuty = requestedDuty; edgePulse = pulse;
            }
        }
        phase += advance;
    }
}

template<class Osc>
void fractionalEdges(bool pulse) {
    for (int rate : {44100, 48000, 96000}) {
        klang::fs = rate;
        const float oneTick = float(rate / turn);
        for (float duty : {1e-12f, float(.25 / turn), float(.5 / turn), float(.75 / turn),
                           float(1.25 / turn), float(4.75 / turn), 1e-9f, 1e-6f}) {
            const double boundary = std::floor(double(duty) * turn);
            for (double tick : {0.0, 1.0, boundary - 1, boundary, boundary + 1, boundary + 44, -1.0}) {
                const float angle = float(tick / turn * fast::twoPi);
                for (float hz : {0.f, oneTick * .49f, oneTick, oneTick * 2.5f,
                                 -oneTick, -oneTick * 2.5f, .001f, -.001f
#ifdef OSM_MULTICYCLE
                                 , float(rate), -float(rate), rate * 1.25f, rate * -1.25f,
                                 rate * 2.f, rate * -2.f, rate * 2.25f, rate * -2.25f,
                                 std::nextafter(float(rate), 0.f),
                                 std::nextafter(float(rate), exceptional(0x7f800000))
#endif
                                 }) {
                    Osc osc;
                    osc.set(hz, angle, duty);
                    auto phase = position(angle);
                    compare(osc, phase, hz, rate, duty, pulse, 64);
                    ++fixtures;
                }
            }
            // Repeated direction changes and PWM must preserve physical phase,
            // including when the fractional reflected coordinate changes.
            Osc osc;
            osc.set(oneTick, fast::twoPi * duty, duty);
            auto phase = position(fast::twoPi * duty);
            for (int i = 0; i < 256; ++i) {
                const float hz = i % 3 == 0 ? 0 : i % 3 == 1 ? oneTick : -oneTick;
                const float width = duty * (i % 2 ? .75f : 1.25f);
                osc.set(hz);
                osc.setDuty(width);
                compare(osc, phase, hz, rate, width, pulse, 1);
            }
#ifdef OSM_MULTICYCLE
            // Combine sub-tick duty PWM with direction, zero/residual steps and
            // whole-cycle boundaries. Compare the requested, unquantised duty.
            const float frequencies[] = {0, oneTick, -oneTick, float(rate), -float(rate),
                rate * 1.25f, rate * -2.25f, rate * 2.f, rate * -2.f,
                std::nextafter(float(rate), 0.f),
                std::nextafter(float(rate), exceptional(0x7f800000)), -440};
            for (int i = 0; i < 768; ++i) {
                const float hz = frequencies[i % 12];
                const float width = duty * (i % 2 ? .75f : 1.25f);
                osc.set(hz);
                osc.setDuty(width);
                compare(osc, phase, hz, rate, width, pulse, 1);
            }
            osc.set(rate * -2.25f, 0, duty); phase = 0;
            osc.set(klang::relative{.25f});
            compare(osc, phase, rate * -2.25f, rate, duty, pulse, 64, 0x40000000u);
            osc.reset(); phase = 0;
            compare(osc, phase, rate * -2.25f, rate, duty, pulse, 64, 0x40000000u);
#endif
            osc.set(-oneTick, 0, duty); phase = 0;
            osc.set(klang::relative{.25f});
            compare(osc, phase, -oneTick, rate, duty, pulse, 64, 0x40000000u);
            osc.reset(); phase = 0;
            compare(osc, phase, -oneTick, rate, duty, pulse, 64, 0x40000000u);
            const int otherRate = rate == 48000 ? 44100 : 48000;
            klang::fs = otherRate;
            osc.set(oneTick);
            compare(osc, phase, oneTick, otherRate, duty, pulse, 64, 0x40000000u);
            klang::fs = rate;
        }
    }
}

template<class Osc>
void matrix(bool pulse, double defaultDuty) {
    for (int rate : {44100, 48000, 96000}) {
        klang::fs = rate;
        for (float hz : {0.f, .001f, .01f, .1f, 1.f, 20.f, 100.f, 440.f, 1000.f,
                         5000.f, 10000.f, 20000.f, rate * .4999f, rate * .5f, rate * .75f, rate * .9999f,
                         std::nextafter(float(rate), 0.f)
#ifdef OSM_MULTICYCLE
                         , float(rate), std::nextafter(float(rate), exceptional(0x7f800000)), rate * 1.25f,
                         std::nextafter(float(rate * 2), 0.f), rate * 2.f, rate * 2.25f,
                         rate * 15.125f, rate * 1000.125f, exceptional(0x7f7fffff)
#endif
                         }) {
            for (int direction : {1, -1}) {
            const float requested = hz * direction;
            for (float duty : {0.f, 1e-9f, 1e-6f, .001f, .01f, .1f, .25f, .5f, .9f, .99f, .999999f, 1.f}) {
                for (float angle : {0.f, fast::twoPi * duty, fast::twoPi * .137f, -fast::twoPi * .25f}) {
                    Osc osc;
                    osc.set(requested, angle, duty);
                    auto phase = position(angle);
                    compare(osc, phase, requested, rate, duty, pulse, 257);
                    check(float(osc.frequency) == requested, "public frequency cache");
                    ++fixtures;
                }
            }
            }
        }
        Osc osc;
        auto phase = std::uint32_t(0);
        compare(osc, phase, 1000, rate, defaultDuty, pulse, 1024);
        osc.set(440);
        compare(osc, phase, 440, rate, defaultDuty, pulse, 1024);
        osc.reset(); phase = 0;
        compare(osc, phase, 440, rate, defaultDuty, pulse, 1024);

        // Through-zero audio-rate FM/PWM, including stops and restarts.
        for (int i = 0; i < 12000; ++i) {
            const float hz = i % 499 == 0 ? 0.f : float(11999.999 * std::sin(i * .031));
            const float duty = float(.5 + .5 * std::sin(i * .047));
            osc.set(hz);
            osc.setDuty(duty);
            compare(osc, phase, hz, rate, duty, pulse, 1);
        }
#ifdef OSM_MULTICYCLE
        // Cross whole-cycle boundaries and direction while also changing duty.
        for (int i = 0; i < 12000; ++i) {
            const float hz = i % 193 == 0 ? float(rate * (i % 7 - 3)) : float(rate * 3.125 * std::sin(i * .031));
            const float duty = float(.5 + .5 * std::sin(i * .047));
            osc.set(hz);
            osc.setDuty(duty);
            compare(osc, phase, hz, rate, duty, pulse, 1);
        }
        // Wide-frequency phase, reset, rate refresh and exceptional containment.
        for (float hz : {rate * -2.f, rate * -2.25f, rate * 1.25f, rate * 2.f, exceptional(0x7f7fffff)}) {
            osc.set(hz, fast::twoPi * .137f, .25f); phase = position(fast::twoPi * .137f);
            osc.set(klang::relative{.25f});
            compare(osc, phase, hz, rate, .25, pulse, 128, 0x40000000u);
            osc.reset(); phase = 0;
            compare(osc, phase, hz, rate, .25, pulse, 128, 0x40000000u);
            const int otherRate = rate == 48000 ? 44100 : 48000;
            klang::fs = otherRate;
            osc.set(hz);
            compare(osc, phase, hz, otherRate, .25, pulse, 128, 0x40000000u);
            klang::fs = rate;
            osc.set(-440);
            compare(osc, phase, -440, rate, .25, pulse, 128, 0x40000000u);
        }
#endif
        osc.set(440, 0, .5); phase = 0;
        osc.set(klang::relative{.25f});
        compare(osc, phase, 440, rate, .5, pulse, 256, 0x40000000u);
        osc.set(440, klang::relative{.5f});
        compare(osc, phase, 440, rate, .5, pulse, 256, 0x80000000u);
        osc.reset(); phase = 0;
        compare(osc, phase, 440, rate, .5, pulse, 256, 0x80000000u);
        osc.set(440, fast::twoPi * .25f); phase = 0x40000000u;
        compare(osc, phase, 440, rate, .5, pulse, 256);
        klang::fs = rate == 48000 ? 44100 : 48000;
        osc.set(440);
        compare(osc, phase, 440, int(klang::fs), .5, pulse, 256);
        osc.set(-440, 0, .25); phase = 0;
        osc.set(klang::relative{.25f});
        compare(osc, phase, -440, int(klang::fs), .25, pulse, 256, 0x40000000u);
        osc.reset(); phase = 0;
        compare(osc, phase, -440, int(klang::fs), .25, pulse, 256, 0x40000000u);
        osc.set(440);
        compare(osc, phase, 440, int(klang::fs), .25, pulse, 256, 0x40000000u);
        osc.set(-440, fast::twoPi * .5f); phase = 0x80000000u;
        compare(osc, phase, -440, int(klang::fs), .25, pulse, 256);
        klang::fs = rate;
        osc.set(-440);
        compare(osc, phase, -440, rate, .25, pulse, 256);
        // Exercise virtual setter dispatch and signed steps around zero/tick limits.
        fast::Osm& controls = osc;
        controls.set(0, fast::twoPi * .137f, .25f);
        phase = position(fast::twoPi * .137f);
        const float oneTick = float(rate / turn);
        for (float hz : {oneTick * -.49f, -oneTick, -440.f, 440.f,
                         oneTick * .49f, oneTick, 0.f, -0.f}) {
            controls.set(hz);
            controls.setDuty(.25f);
            compare(osc, phase, hz, rate, .25, pulse, 128);
        }
        controls.set(-440, klang::relative{.25f});
        compare(osc, phase, -440, rate, .25, pulse, 128, 0x40000000u);
        for (float duty : {-2.f, 2.f}) {
            osc.set(440, 0, duty); phase = 0;
            compare(osc, phase, 440, int(klang::fs), duty < 0 ? 0 : 1, pulse, 256);
        }
        // Historical fast mode contains multi-cycle inputs; the extension renders them.
        for (float hz : {
#ifndef OSM_MULTICYCLE
                         float(klang::fs), -float(klang::fs), float(klang::fs) * 2.25f, -float(klang::fs) * 2.25f,
#endif
                         exceptional(0x7f800000), exceptional(0xff800000), exceptional(0x7fc00000)}) {
            osc.set(hz, 0, .5); phase = 0;
            const int previousFailures = failures;
            compare(osc, phase, 0, int(klang::fs), .5, pulse, 16);
            if (failures != previousFailures)
                std::printf("containment request=%.9g\n", hz);
        }
    }
}

int main() {
    namespace target = fast;
    matrix<target::Saw>(false, 0);
    matrix<target::Triangle>(false, .5);
    matrix<target::Square>(true, .5);
    matrix<target::Pulse>(true, .5);
#ifdef OSM_FRACTIONAL_DUTY
    fractionalEdges<target::Saw>(false);
    fractionalEdges<target::Triangle>(false);
    fractionalEdges<target::Square>(true);
    fractionalEdges<target::Pulse>(true);
#endif
    std::printf("fixtures=%d checks=%d samples=%lld failures=%d max_error=%.12g rms_error=%.12g peak=%.12g\n",
                fixtures, checks, samples, failures, worst, std::sqrt(squares / samples), peak);
    std::printf("unquantised_duty_audio_max=%.12g unquantised_duty_edge_max=%.12g\n", requestedAudioWorst, requestedEdgeWorst);
    std::printf("edge_rate=%d edge_hz=%.9g edge_duty=%.12g edge_pulse=%d edge_sample=%d\n",
                edgeRate, edgeHz, edgeDuty, edgePulse, edgeSample);
    return failures ? 1 : 0;
}
#pragma float_control(pop)
