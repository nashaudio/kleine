/** @file pd.h
 * @brief Pure Data primitives for Klang; see each class for its supported contract.
 * Set klang::fs before constructing processors. Evaluate audio objects once per
 * sample; reuse out for additional consumers. Controls are changed with set().
 */
#pragma once

#include <klang.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <list>
#include <stdexcept>
#include <string_view>
using namespace klang::optimised;

namespace pd {

// Primitive ports adapted from Pure Data 0.55-2, src/d_osc.c,
// src/d_osc.h, src/d_ctl.c and src/d_filter.c.
// Copyright (c) 1997-2024 Miller Puckette.
// Redistribution terms: ../../docs/licenses/Pure-Data-BSD.txt.

// Pd osc~: cosine table interpolation, frequency in Hz and phase in cycles.
/** @brief PD osc~ cosine oscillator; frequency is Hz and phase is in cycles.
 * @code
 * pd::osc osc;
 * // In process():
 * osc(440) * 0.2f >> out;
 * // On a phase-reset event: osc.phase(0.25f);
 * @endcode
 */
struct osc : Generator {
    float frequency = 0;
    double position = 1572864.0; // Pd's UNITBIT32 bias, preserving phase precision.
    int count = 0;
    static constexpr int size = 2048;
    static constexpr double unit = 1572864.0;

    inline static const std::array<float, size + 1> table = [] {
        std::array<float, size + 1> values{};
        for (int i = 0; i <= size; ++i) values[i] = float(std::cos(2 * pi.d * i / size));
        values[0] = values[size] = 1;
        values[size / 4] = values[3 * size / 4] = 0;
        values[size / 2] = -1;
        return values;
    }();

    void set(param hz) override { frequency = hz; }
    void set(param hz, param cycles) override { set(hz); phase(cycles); }
    void phase(param cycles) { position = unit + size * float(cycles); }
    void reset() { position = unit; count = 0; out = 0; }

    void process() override {
        // memcpy keeps Pd's IEEE-754 index/fraction technique alias-safe.
        uint64_t bits;
        std::memcpy(&bits, &position, sizeof(bits));
        const int index = int((bits >> 32) & (size - 1));
        bits = (bits & 0xffffffffULL) | 0x4138000000000000ULL;
        double fraction;
        std::memcpy(&fraction, &bits, sizeof(fraction));
        const float a = table[index], b = table[index + 1];
        out = a + float(fraction - unit) * (b - a);
        position += frequency * (float(size) / float(fs));
        if (++count == 64) {
            double wrapped = position + (unit * size - unit);
            std::memcpy(&bits, &wrapped, sizeof(bits));
            uint64_t bias;
            const double scaled = unit * size;
            std::memcpy(&bias, &scaled, sizeof(bias));
            bits = (bits & 0xffffffffULL) | (bias & 0xffffffff00000000ULL);
            std::memcpy(&wrapped, &bits, sizeof(wrapped));
            position = wrapped - scaled + unit;
            count = 0;
        }
    }
};

// Pd phasor~: a wrapping phase ramp, including negative frequency and phase reset.
/** @brief PD phasor~ ramp in [0,1), with signed frequency in Hz.
 * @code
 * pd::phasor phasor;
 * signal phase = phasor(2); // Once per sample; two cycles per second.
 * @endcode
 * phase(cycles) sets phase; reset() restores zero phase and maintenance state.
 */
struct phasor : Generator {
    float frequency = 0;
    double position = osc::unit;
    int count = 0;
    void set(param hz) override { frequency = hz; }
    void phase(param cycles) { position = osc::unit + double(float(cycles)); }
    void reset() { position = osc::unit; count = 0; out = 0; }
    void process() override {
        uint64_t bits;
        std::memcpy(&bits, &position, sizeof(bits));
        bits = (bits & 0xffffffffULL) | 0x4138000000000000ULL;
        double wrapped;
        std::memcpy(&wrapped, &bits, sizeof(wrapped));
        out = float(wrapped - osc::unit);
        position += frequency * (1.0f / float(fs));
        if (++count == 64) {
            std::memcpy(&bits, &position, sizeof(bits));
            bits = (bits & 0xffffffffULL) | 0x4138000000000000ULL;
            std::memcpy(&position, &bits, sizeof(position));
            count = 0;
        }
    }
};

// Pd cos~: cosine table lookup for an input phase in cycles (not radians).
/** @brief PD cos~ table lookup, with input phase measured in cycles.
 * @code
 * pd::cos cosine;
 * signal value = signal(0.25f) >> cosine;
 * @endcode
 * lookup(cycles) provides the same operation without a stored input/output.
 */
struct cos : Modifier {
    static float lookup(float cycles) {
        const double position = double(cycles * float(osc::size)) + osc::unit;
        uint64_t bits;
        std::memcpy(&bits, &position, sizeof(bits));
        const int index = int((bits >> 32) & (osc::size - 1));
        bits = (bits & 0xffffffffULL) | 0x4138000000000000ULL;
        double fraction;
        std::memcpy(&fraction, &bits, sizeof(fraction));
        const float a = osc::table[index], b = osc::table[index + 1];
        return a + float(fraction - osc::unit) * (b - a);
    }
    void process() override { out = lookup(in); }
};

// Pd wrap~: fractional part in [0, 1), including negative integral inputs.
/** @brief PD wrap~ fractional part, including negative inputs.
 * @code
 * pd::wrap wrap;
 * signal fraction = signal(-0.25f) >> wrap; // 0.75.
 * @endcode
 */
struct wrap : Modifier {
    void process() override { out = float(in) - std::floor(float(in)); }
};

// Pd metro: millisecond control clock, polled once per sample, even when stopped.
// Adapted from 0.55-2 src/x_time.c and m_sched.c; same BSD attribution as above.
// Returns the number of bangs in this sample; ordinary periods can use if (metro(ms)).
// Starting arms a bang for the next poll, using that poll's interval argument.
// PD's synchronous message delivery and block scheduling are separate contracts.
/** @brief Poll a PD-style metronome, returning the number of bangs this sample.
 * @code
 * pd::metro metro;
 * metro = 1;                    // Control event: start; zero stops.
 * // In process(), including while stopped:
 * if (metro(100)) gate = 1 - gate;
 * @endcode
 * set(ms) changes the cold inlet without moving the pending deadline. Assignment,
 * bang() and start() arm a first tick for the next poll; stop() cancels it.
 * @note Tempo units and synchronous outlet feedback are not yet ported. Polling
 * coalesces unobserved starts; PD block delivery is intentionally separate.
 */
struct metro {
    bool legacy = false; // Before Pd 0.45, positive intervals below 1 ms were clamped.

    explicit metro(param milliseconds = 0) { set(milliseconds); }

    // Cold inlet: changing the period does not move an already scheduled bang.
    void set(param milliseconds) {
        interval = milliseconds;
        if (interval <= 0 || (legacy && interval < 1)) interval = 1;
    }

    // Hot inlet: any nonzero value (or bang) arms a tick and restarts.
    void start(param enabled = 1) {
        starting = enabled != 0;
        next = -1;
    }
    metro& operator=(param enabled) {
        start(enabled);
        return *this;
    }
    void bang() { start(); }
    void stop() { start(0); }

    int operator()(param milliseconds) {
        set(milliseconds);
        return (*this)();
    }
    int operator()() {
        int bangs = 0;
        if (starting) {
            bangs = 1;
            next = time + units * interval;
            starting = false;
        }
        const double end = time + (units * 1000) / double(float(fs));
        while (next >= 0 && next < end) {
            ++bangs;
            next += units * interval;
        }
        time = end;
        return bangs;
    }

private:
    static constexpr double units = 32. * 441.; // PD logical time units per ms.
    double interval = 1, time = 0, next = -1;
    bool starting = false;
};

/** @brief PD del/delay: a retriggerable, one-shot control clock.
 * @code
 * pd::del del{200};
 * del.bang();             // Event: schedule a bang after 200 ms.
 * del = 50;               // Hot float: store 50 and restart the delay.
 * del.set(100);           // Cold float: change the NEXT delay only.
 * // Once per sample, including when idle:
 * if (del()) envelope.set(0, 1);
 * // Other control messages:
 * del.stop();
 * del.tempo(1, "samp");   // Durations are now samples.
 * @endcode
 * Construction accepts (duration, tempo, unit), as PD does; default units are
 * milliseconds. Negative durations become zero. bang() replaces a pending bang;
 * stop() cancels it. Zero delay fires on the next poll, without recursive delivery.
 * tempo(amount, unit) accepts msec/millisecond, sec*, min*, sam*, and their per*
 * forms. Nonpositive amount becomes 1; unknown units return false and select 1 ms.
 * updated reports the last poll's bang; pending() reports an armed clock.
 *
 * Adapted from PD 0.55-2 src/x_time.c and src/m_sched.c (Miller Puckette;
 * ../../docs/licenses/Pure-Data-BSD.txt). The source's pending sample-unit tempo-change
 * behaviour is retained: a negative remaining-time calculation leaves that
 * deadline in place. See docs/tests/pd/del.md for the reference comparison.
 * @note Poll exactly once per sample at a fixed klang::fs. Delivery is at the
 * first sample boundary at or after the deadline. Shared PD clock ordering,
 * synchronous feedback and 64-sample block delivery are separate host concerns.
 */
struct del {
    bool updated = false;

    explicit del(param duration = 0, param amount = 0, std::string_view name = "msec") {
        set(duration);
        if (amount != 0) tempo(amount, name);
    }
    void set(param duration) { interval = std::max(0.0, double(float(duration))); }
    void bang() { next = double(frame) + interval * samplesPerUnit(); }
    del& operator=(param duration) {
        set(duration);
        bang();
        return *this;
    }
    void stop() { next = -1; }
    bool pending() const { return next >= 0; }
    bool operator()() {
        updated = pending() && next <= double(frame);
        if (updated) stop();
        ++frame;
        return updated;
    }
    bool tempo(param amount, std::string_view name) {
        const float value = amount <= 0 ? 1 : float(amount);
        const bool reciprocal = name.substr(0, 3) == "per";
        if (reciprocal) name.remove_prefix(3);
        float scale = 1;
        bool samples = false, valid = true;
        if (name == "msec" || name == "millisecond") scale = 1;
        else if (name.substr(0, 3) == "sec") scale = 1000;
        else if (name.substr(0, 3) == "min") scale = 60000;
        else if (name.substr(0, 3) == "sam") samples = true;
        else valid = false;
        // PD stores the parsed unit in a t_float before configuring its clock.
        const float parsed = valid ? (reciprocal ? scale / value : scale * value) : 1;
        const double replacement = samples ? -double(parsed) : double(parsed);
        if (replacement == unit) return valid;
        const double remaining = pending() ? (next - double(frame)) /
            (unit > 0 ? samplesPerUnit() : unit) : -1;
        unit = replacement;
        if (remaining >= 0) next = double(frame) + remaining * samplesPerUnit();
        return valid;
    }

private:
    double samplesPerUnit() const { return unit > 0 ? unit * double(float(fs)) / 1000 : -unit; }
    double interval = 0, unit = 1, next = -1;
    uint64_t frame = 0;
};

// Pd line~/line: two-argument set selects audio ramps; three arguments select
// control ramps with a millisecond grain. The control output holds between emits;
// updated/updates report emissions, including repeated equal values.
// Adapted from 0.55-2 src/d_ctl.c and x_time.c, Miller Puckette, BSD (see above).
// Audio ramps retain PD's block-64 contract; control clocks are polled per sample.
/** @brief Linear ramps with audio and control forms selected by set() arity.
 * @code
 * pd::line line;
 * line.set(1, 100);      // Event: line~ audio ramp to 1 in 100 ms.
 * line.set(0, 500, 20);  // Alternative: line control ramp, 20 ms emissions.
 * line = { {0, 1}, {3000, 0}, 20 }; // Jump to 1, then decay over 3000 ms.
 * line.set({0, 1}, {3000, 0}, 20);  // Equivalent two-breakpoint form.
 * // Once per sample in either mode:
 * signal value = line;
 * if (line.updated) level = value; // React even if the emitted value is unchanged.
 * @endcode
 * Two arguments select audio mode; three select control mode. One target keeps
 * the mode and jumps unless duration(ms) supplied the next ramp's duration.
 * grain(ms) persists; nonpositive grain means 20 ms. stop() freezes the ramp;
 * reset(value) implements control PD "set value" without emitting a message.
 * updates counts emissions since the previous poll; out holds the last value.
 * @note Configure audio ramps on the 64-sample grid. Control polling exposes the
 * final value/count when multiple messages occur within one sample; synchronous
 * per-message outlet callbacks are not implemented. legacy selects pre-0.48 stop.
 */
struct line : Generator {
    /** @brief Envelope point {milliseconds, value}, relative to this trigger.
     * @code
     * pd::line::Point end{3000, 0};
     * @endcode
     */
    struct Point {
        param milliseconds, value;
    };
    /** @brief Two-breakpoint ramp; supplying grain selects control output.
     * @code
     * pd::line::Ramp decay{{0, 1}, {3000, 0}, 20};
     * @endcode
     * The first time must be zero; the final time is the ramp duration in ms.
     * Two points describe one ramp, not a queued multisegment vline~ envelope.
     */
    struct Ramp {
        Point first, last;
        param grain = 20;
        bool control = false;
        Ramp(Point a, Point b) : first(a), last(b) {}
        Ramp(Point a, Point b, param interval) : first(a), last(b), grain(interval), control(true) {}
    };

    float value = 0, target = 0, increment = 0, blockIncrement = 0, sample = 0;
    int blocks = 0, position = 0;
    bool control = false, updated = false;
    int updates = 0;
    bool legacy = false; // [line] stop behaviour before Pd 0.48.

    /** @brief Trigger one ramp from brace-enclosed {timeMs,value} endpoints.
     * @throws std::invalid_argument If the first time is not zero, or the last
     * time is negative/nonfinite. Validation happens before changing the ramp.
     */
    line& operator=(const Ramp& ramp) {
        if (ramp.first.milliseconds != 0 || !std::isfinite(float(ramp.last.milliseconds)) || ramp.last.milliseconds < 0)
            throw std::invalid_argument("pd::line endpoints require time 0 followed by a nonnegative finite duration in ms");
        if (ramp.control) {
            set(ramp.first.value, 0, ramp.grain);
            set(ramp.last.value, ramp.last.milliseconds, ramp.grain);
        } else {
            set(ramp.first.value, 0);
            set(ramp.last.value, ramp.last.milliseconds);
        }
        return *this;
    }
    void set(Point first, Point last) { *this = Ramp(first, last); }
    void set(Point first, Point last, param grainMs) { *this = Ramp(first, last, grainMs); }

    // Hot inlet; a duration supplied separately is consumed by this target.
    void set(param destination) override {
        if (hasDuration) {
            const float ms = nextDuration;
            hasDuration = false;
            if (control) set(destination, ms, controlGrain);
            else set(destination, ms);
            return;
        }
        if (control) {
            origin = controlTarget = destination;
            next = -1;
            emit(destination);
            return;
        }
        value = target = destination;
        blocks = 0;
    }
    void set(param destination, param milliseconds) override {
        if (control) sample = value = out;
        control = false;
        hasDuration = false;
        next = -1;
        pending = 0;
        if (milliseconds <= 0) {
            set(destination);
            return;
        }
        target = destination;
        blocks = std::max(1, int(float(milliseconds) * (float(fs) / 64000.0f)));
        blockIncrement = (target - value) / blocks;
        increment = blockIncrement / 64;
    }
    void set(param destination, param milliseconds, param grainMs) override {
        if (!control) {
            origin = controlTarget = out;
            start = finish = time;
            next = -1;
        }
        control = true;
        hasDuration = false;
        grain(grainMs);
        if (milliseconds <= 0) {
            set(destination);
            return;
        }
        origin = current(time);
        controlTarget = destination;
        start = time;
        finish = time + double(float(milliseconds)) * units;
        emit(origin);
        next = time + std::min(double(controlGrain) * units, finish - time);
    }
    void duration(param milliseconds) {
        nextDuration = milliseconds;
        hasDuration = true;
    }
    void grain(param milliseconds) { controlGrain = milliseconds > 0 ? float(milliseconds) : 20; }
    void stop() {
        if (control) {
            if (!legacy) origin = current(time);
            controlTarget = origin;
            next = -1;
        } else {
            target = value;
            blocks = 0;
        }
    }
    // PD's control "set value" message: reset internal value without emitting.
    void reset(param destination = 0) {
        if (control) {
            origin = controlTarget = destination;
            next = -1;
        } else {
            value = target = destination;
            blocks = 0;
        }
    }
    void process() override {
        updated = false;
        updates = 0;
        const double end = time + units * 1000 / double(float(fs));
        if (control) {
            while (next >= 0 && next < end) {
                emit(current(next));
                next = finish - next < units * 1e-9 ? -1
                     : next + std::min(double(controlGrain) * units, finish - next);
            }
            updates = pending;
            updated = updates != 0;
            pending = 0;
            time = end;
            position = (position + 1) % 64;
            return;
        }
        if (position == 0) sample = blocks ? value : (value = target);
        out = sample;
        if (blocks) sample += increment;
        if (++position == 64) {
            if (blocks) {
                value += blockIncrement;
                --blocks;
            }
            position = 0;
        }
        time = end;
    }

private:
    static constexpr double units = 32. * 441.;
    double time = 0, start = 0, finish = 0, next = -1;
    float origin = 0, controlTarget = 0, controlGrain = 20, nextDuration = 0;
    bool hasDuration = false;
    int pending = 0;
    float current(double at) const {
        return at >= finish ? controlTarget
            : float(origin + (at - start) / (finish - start) * double(controlTarget - origin));
    }
    void emit(float v) {
        out = v;
        ++pending;
    }
};

/** @brief PD vline~: sample-accurate queued ramps with fractional start times.
 * @code
 * pd::vline envelope;
 * // On a trigger, equivalent to PD's message "1 1, 0 200 1":
 * envelope.set(1, 1);
 * envelope.set(0, 200, 1);
 * // In process(), exactly once per sample:
 * signal amplitude = envelope;
 * @endcode
 * set(target), set(target, durationMs), and set(target, durationMs, delayMs)
 * implement the hot float/list inlet. duration(ms) and delay(ms) set the cold
 * inlets; both are consumed by the next hot message. Delays are relative to the
 * message time, not the end of the previous ramp. A new segment cancels queued
 * segments starting at or after it, except a same-time jump followed by a ramp.
 * Negative duration means a jump. Negative delay cancels all ramps and jumps
 * immediately. stop() clears the queue and holds PD's internal next-sample value.
 * Targets use PD_FLOATSIZE=32's PD_BIGORSMALL sanitisation.
 *
 * time() is the next audio sample's time in milliseconds, initially zero.
 * messageTime(ms) optionally supplies an absolute host logical timestamp for
 * subsequent messages, including fractional times; process() clears this override.
 * By default messages use time(), derived from the sample count to avoid drift.
 * sync(ms) explicitly selects and anchors a host audio clock without changing
 * ramp state or queued timestamps; that clock advances as in PD's perform loop.
 * Deliver messages before processing the samples they affect. PD's scheduler,
 * canvas reblocking and DSP suspension belong to the host; the fixture supplies
 * PD's block-clock anchoring separately. Set fs before use and keep it fixed.
 *
 * Adapted from Pure Data 0.55-2, src/d_ctl.c (vline_tilde_*), Miller Puckette,
 * under the BSD terms above. Queue storage is unbounded and allocates on messages,
 * as PD does; destruction and stop release pending segments.
 */
struct vline : Generator {
    /** @brief Set the next ramp's duration (middle inlet), in milliseconds. */
    void duration(param milliseconds) { nextDuration = milliseconds; }
    /** @brief Set the next ramp's initial delay (right inlet), in milliseconds. */
    void delay(param milliseconds) { nextDelay = milliseconds; }
    /** @brief Next audio sample's time, in milliseconds since construction. */
    double time() const { return audioTime; }
    /** @brief Anchor the next audio sample's time (ms); preserve ramp/queue state. */
    void sync(double milliseconds) {
        audioTime = milliseconds;
        synchronised = true;
    }
    /** @brief Supply a host timestamp for messages before the next evaluation. */
    void messageTime(double milliseconds) {
        logicalTime = milliseconds;
        timestamped = true;
    }

    void set(param destination) override {
        float target = destination;
        uint32_t bits;
        std::memcpy(&bits, &target, sizeof(bits));
        if ((bits & 0x20000000) == ((bits >> 1) & 0x20000000)) target = 0;
        const float ms = nextDuration < 0 ? 0 : nextDuration;
        const double start = (timestamped ? logicalTime : audioTime) + nextDelay;
        if (nextDelay < 0) {
            value = target;
            stop();
            return;
        }
        auto position = segments.begin();
        for (; position != segments.end(); ++position) {
            if (position->start > start || (position->start == start &&
                (position->finish > position->start || ms <= 0))) break;
        }
        segments.erase(position, segments.end());
        segments.push_back({start, start + ms, target});
        nextDuration = nextDelay = 0;
    }
    void set(param destination, param milliseconds) override {
        duration(milliseconds);
        set(destination);
    }
    void set(param destination, param milliseconds, param delayMs) override {
        delay(delayMs);
        set(destination, milliseconds);
    }
    /** @brief Cancel all queued/active ramps and clear both cold inlets. */
    void stop() {
        segments.clear();
        increment = 0;
        nextDuration = nextDelay = 0;
        target = float(value);
        finish = 1e20;
    }
    void process() override {
        const double step = 1000.0 / double(float(fs));
        const double next = synchronised ? audioTime + step : (frame + 1) * 1000 / double(float(fs));
        while (!segments.empty() && segments.front().start < next) {
            const Segment segment = segments.front();
            segments.pop_front();
            if (finish <= next) {
                value = target;
                increment = 0;
            }
            if (segment.finish <= segment.start) {
                value = segment.target;
                increment = 0;
            } else {
                const double slope = (segment.target - value) / (segment.finish - segment.start);
                value += slope * (next - segment.start);
                increment = slope * step;
            }
            target = segment.target;
            finish = segment.finish;
        }
        if (finish <= next) {
            value = target;
            increment = 0;
            finish = 1e20;
        }
        out = float(value);
        value += increment;
        audioTime = next;
        ++frame;
        timestamped = false;
    }

private:
    /** @brief A queued target with absolute start/end times in milliseconds. */
    struct Segment {
        double start, finish;
        float target;
    };
    std::list<Segment> segments;
    double audioTime = 0, logicalTime = 0, value = 0, increment = 0, finish = 1e20;
    double frame = 0;
    float target = 0, nextDuration = 0, nextDelay = 0;
    bool timestamped = false, synchronised = false;
};

// Pd env~ default: 1024-point Hann RMS, 512-sample hop, PD's 100 dB unity reference.
// This initial port fixes the analysis/block sizes used by Farnell's tone detector.
/** @brief PD env~ RMS meter, 1024-sample Hann window and 512-sample hop.
 * @code
 * pd::env env;
 * signal meter = input >> env;
 * if (env.updated) level = meter; // PD dB scale: unity RMS is 100.
 * @endcode
 * out holds the latest value, including the new result when updated is true.
 * Analysis completes at the end of a block. On the following sample, out and
 * updated publish that result together, matching PD's next-block control output.
 * out holds between publications; repeated equal results still set updated.
 * @note Window/hop configuration is not yet ported; block size is fixed at 64.
 */
struct env : Modifier {
    inline static const std::array<float, 1088> window = [] {
        std::array<float, 1088> values{};
        for (int i = 0; i < 1024; ++i) values[i] = float((1 - std::cos(2 * 3.14159 * i / 1024)) / 1024);
        return values;
    }();
    std::array<float, 64> buffer{};
    std::array<float, 3> sums{};
    int position = 0, phase = 0;
    bool updated = false;
    void process() override {
        updated = ready;
        if (updated) out = pending;
        ready = false;
        buffer[position++] = in;
        if (position != 64) return;
        position = 0;
        int slot = 0;
        for (int offset = phase; offset < 1024; offset += 512, ++slot) {
            float sum = sums[slot];
            for (int i = 0; i < 64; ++i) sum += window[offset + i] * (buffer[63 - i] * buffer[63 - i]);
            sums[slot] = sum;
        }
        sums[slot] = 0;
        phase -= 64;
        if (phase < 0) {
            pending = sums[0] > 0 ? std::max(0.0f, float(100 + (10 / std::log(10.0)) * std::log(double(sums[0])))) : 0;
            sums[0] = sums[1]; sums[1] = 0;
            phase = 512 - 64;
            ready = true;
        }
    }

private:
    float pending = 0;
    bool ready = false;
};

// Pd-style lop~ (gentle one-pole low-pass) for Klang
// Matches Pure Data’s lop~ behaviour closely
static inline void flush_denormal(float& x) {
    constexpr float DENORM_LIMIT = std::numeric_limits<float>::min();
    x = (std::fabs(x) < DENORM_LIMIT) ? 0.0f : x;
}


// Pd lop~: a one-pole lowpass with a frequency control in Hz.
/** @brief PD lop~ one-pole lowpass; set(hz) specifies cutoff in Hz.
 * @code
 * pd::lop lowpass;
 * lowpass.set(100); // Configure on a control event or in prepare().
 * signal filtered = input >> lowpass;
 * @endcode
 */
struct lop : Modifier {
    param freq;       // cutoff frequency (Hz)
    signal coef;       // coefficient

    lop() : freq(0), coef(0) {}

    void set(param f) {
//        if(freq != f){
        	freq = f;
        	update();
//        }
    }

    inline void update() {
        // Pd lop~ coefficient formula:
        coef = float(freq) * float(2 * 3.14159 / float(fs));
        if (coef > 1.0f) 
        	coef = 1.0f;
        else if (coef < 0.0f) 
        	coef = 0.0f;
    }

    void process() {
        const signal feedback = 1.0f - coef;
        out = coef * in + feedback * out;

        flush_denormal(out);
    }
};

// Pd noise~: output before advancing the seeded, wrapping 32-bit LCG.
/** @brief PD noise~ white noise; seed(value) sets its reproducible 32-bit state.
 * @code
 * pd::noise noise;
 * noise.seed(404933);
 * // In process():
 * noise * 0.1f >> out;
 * @endcode
 */
struct noise : Generator {
    inline static uint32_t next = 307;
    uint32_t val = (next *= 1319u);
    void seed(uint32_t value) { val = value; }
    void process() override {
        out = float(int32_t(val & 0x7fffffffu) - 0x40000000) / 1073741824.0f;
        val = val * 435898247u + 382842987u;
    }
};

// Pd hip~: a normalised one-pole highpass with a frequency control in Hz.
/** @brief PD hip~ one-pole highpass; cutoff is Hz and clear() removes stored state.
 * @code
 * pd::hip highpass;
 * highpass.set(90);
 * signal filtered = input >> highpass;
 * @endcode
 * legacy selects PD <= 0.43 gain behaviour.
 */
struct hip : Modifier {
    float coefficient = 1, state = 0;
    bool legacy = false; // Pd compatibility <= 0.43 omits normalisation.
    void set(param hz) override {
        coefficient = std::clamp(float(1 - std::max(0.0f, float(hz)) * (2 * 3.14159) / float(fs)), 0.0f, 1.0f);
    }
    void clear() { state = 0; }
    void process() override {
        if (coefficient == 1) { out = in; state = 0; return; }
        const float next = in + coefficient * state;
        out = (legacy ? 1 : 0.5f * (1 + coefficient)) * (next - state);
        state = next;
        flush_denormal(state);
    }
};

static inline float fastcos(float f) {
	// Pd’s polynomial cosine approximation
	if (f >= -0.5f * 3.14159f && f <= 0.5f * 3.14159f) {
		float g = f * f;
		return (((g*g*g * (-1.0f/720.0f) + g*g*(1.0f/24.0f)) - g*0.5) + 1);
	}
	return 0.0f;
}

// Pd bp~: a two-pole bandpass with frequency and Q controls.
/** @brief PD bp~ bandpass; set(frequencyHz, Q) caches coefficient changes.
 * @code
 * pd::bpf resonance;
 * resonance.set(400, 7);
 * signal filtered = input >> resonance;
 * @endcode
 * The one-argument setter changes frequency while retaining Q.
 */
struct bpf : Modifier {
    param freq, q;
    signal x1 = 0, x2 = 0;
    signal coef1 = 0, coef2 = 0, gain = 0;

    bpf() : freq(0), q(0) { }
    
    void set(param f) {
    	set(f, q);
    }
    
    void set(param f, param Q){
    	f = max(0.001f, f);
    	Q = max(0.0f, Q);
    	if(freq != f || q != Q){
    		freq = f;
    		q = Q;
    		update();
    	}
    }

    void update() {
        float omega = float(freq) * (2.0f * 3.14159f) / float(fs);
        float oneminusr = (q < 0.001f ? 1.0f : omega / q);
        if (oneminusr > 1.0f) oneminusr = 1.0f;
        float r = 1.0f - oneminusr;
        coef1 = 2.0f * fastcos(omega) * r;
        coef2 = -r * r;
        gain  = 2.0f * oneminusr * (oneminusr + r * omega);
    }

    void process() {
        out = in + coef1 * x1 + coef2 * x2;
        x2 = x1;
        
        flush_denormal(out);
        x1 = out;
        
        out *= gain;
    }
};

// Pd samphold~: capture when the right-inlet signal decreases.
/** @brief PD samphold~; capture input whenever the trigger signal decreases.
 * @code
 * pd::samphold hold;
 * signal held = input >> hold(trigger);
 * @endcode
 * reset(value) sets the previous trigger (default 1e20); hold(value) sets the
 * stored output, corresponding to PD's "set" message.
 */
struct samphold : Modifier {
    param trigger = 0;
    signal previous = 0;
    void set(param value) override { trigger = value; }
    void reset(param value = 1e20f) { previous = value; }
    void hold(param value) { out = value; }
    void process() override {
        if (trigger < previous) out = in;
        previous = trigger;
    }
};

// Pd rzero~: y[n] = x[n] - coefficient * x[n-1].
/** @brief PD rzero~: input minus coefficient times the previous input sample.
 * @code
 * pd::rzero difference;
 * signal edges = input >> difference(0.99f);
 * @endcode
 * clear() zeros the previous input sample.
 */
struct rzero : Modifier {
    param coefficient = 0;
    signal previous = 0;
    void set(param value) override { coefficient = value; }
    void clear() { previous = 0; }
    void process() override { out = in - coefficient * previous; previous = in; }
};

// Pd vcf~: table-based complex resonator; left/real and right/imaginary outputs.
// Pure Data 0.55-2 src/d_osc.h SIGVCFPERF, retaining its arithmetic and gain.
/** @brief PD vcf~ complex resonator with frequency in Hz and resonance Q.
 * @code
 * pd::vcf mode;
 * signal real = input >> mode(frequency, 80);
 * signal imaginary = mode.im; // Reuse; do not process a second time.
 * @endcode
 * set(hz) retains Q; clear() resets both outputs. out/re is the left PD output,
 * im the right. Legacy lpf/bpf aliases denote these quadrature outputs.
 */
struct vcf : Modifier {
    param freq = 0, q = 0;
    signal re = 0, im = 0;
    signal& lpf = out; // Existing aliases retained; these are quadrature outputs.
    signal& bpf = im;
    int count = 0;
    void set(param hz) override { freq = hz; }
    void set(param hz, param resonance) override { freq = hz; q = max(0, resonance); }
    void clear() { re = im = out = 0; }
    void process() override {
        const float cf = std::max(0.0f, float(freq) * (6.28318f / float(fs)));
        const float qinv = q > 0 ? 1.0f / float(q) : 0;
        const float radius = std::max(0.0f, qinv > 0 ? 1 - cf * qinv : 0);
        const float correction = float(2. - 2. / (float(q) + 2.));
        const float index = cf * (float(osc::size) / 6.28318f);
        const double phase = double(index) + osc::unit;
        uint64_t bits;
        std::memcpy(&bits, &phase, sizeof(bits));
        const int i = int((bits >> 32) & (osc::size - 1));
        bits = (bits & 0xffffffffULL) | 0x4138000000000000ULL;
        double fraction;
        std::memcpy(&fraction, &bits, sizeof(fraction));
        const float f = float(fraction - osc::unit);
        const int j = (i - osc::size / 4) & (osc::size - 1);
        const float cr = radius * (osc::table[i] + f * (osc::table[i+1] - osc::table[i]));
        const float ci = radius * (osc::table[j] + f * (osc::table[j+1] - osc::table[j]));
        const float previous = re;
        out = re = correction * (1 - radius) * in + cr * previous - ci * im;
        im = ci * previous + cr * im;
        if (++count == 64) {
            // PD_BIGORSMALL flushes extreme state at the DSP block boundary.
            if (!(std::fabs(float(re)) >= 1e-19f && std::fabs(float(re)) <= 1e19f)) re = 0;
            if (!(std::fabs(float(im)) >= 1e-19f && std::fabs(float(im)) <= 1e19f)) im = 0;
            count = 0;
        }
    }
};

};
