#pragma once

// Experimental audio-thread timers. Not part of the public Klang API.
#include <klang.h>
#include <cstdint>
#include <cstring>
#include <new>

namespace scheduling_trial {

enum class Unit { milliseconds, samples };
struct Interval { double value; Unit unit; };
inline Interval milliseconds(double value) { return {value, Unit::milliseconds}; }
inline Interval samples(double value) { return {value, Unit::samples}; }
enum class First { afterInterval, immediately };
enum class Change { keepNext, restart };
enum class Result { added, unchanged, changed, full, invalid, unavailable, busy };

// Fixed-size, non-owning callback; compare typed member pointers, never their padding.
struct Callback {
    static constexpr int bytes = 32;
    void* object = nullptr;
    void (*invoke)(void*, const unsigned char*) = nullptr;
    bool (*equal)(const unsigned char*, const unsigned char*) = nullptr;
    unsigned char member[bytes]{};

    template<class T> static Callback bind(T* object, void (T::*method)()) {
        using Method = void (T::*)();
        static_assert(sizeof(Method) <= bytes, "increase callback storage for this ABI");
        Callback callback;
        if (!object || !method) return callback;
        callback.object = object;
        std::memcpy(callback.member, &method, sizeof method);
        callback.invoke = [](void* target, const unsigned char* data) {
            Method function;
            std::memcpy(&function, data, sizeof function);
            (static_cast<T*>(target)->*function)();
        };
        callback.equal = [](const unsigned char* left, const unsigned char* right) {
            Method a, b;
            std::memcpy(&a, left, sizeof a);
            std::memcpy(&b, right, sizeof b);
            return a == b;
        };
        return callback;
    }
    bool same(const Callback& other) const {
        return object == other.object && equal == other.equal && equal && equal(member, other.member);
    }
    void operator()() const { invoke(object, member); }
};

// Repeating deadlines use origin + occurrence * period, avoiding repeated rounding/addition.
struct Timer {
    Callback callback;
    Interval interval{};
    double period = 0, origin = 0, next = 0;
    std::uint64_t occurrence = 0, delivery = 0;
    bool active = false;
    void anchor(double deadline) { origin = next = deadline; occurrence = 0; cacheDelivery(); }
    void advance() { next = origin + double(++occurrence) * period; cacheDelivery(); }
    void cacheDelivery() {
        // Remove only floating-point roundoff at an integer boundary (e.g. 10 * 44.1).
        const double integer = std::round(next);
        const double tolerance = 4 * std::numeric_limits<double>::epsilon() * std::max(1.0, std::abs(next));
        delivery = std::uint64_t(std::ceil(std::abs(next - integer) <= tolerance ? integer : next));
    }
};

// Small bounded queue. All mutation and processing happen on one audio thread.
template<int Capacity> class Timers {
    static_assert(Capacity > 0, "timer capacity must be positive");
    Timer timers[Capacity]{};
    int used = 0;
    double rate = 0;
    std::uint64_t frame = 0;
    bool rendering = false, callbackRunning = false;

    static double period(Interval interval, double rate) {
        return interval.unit == Unit::samples ? interval.value : interval.value * rate / 1000.0;
    }
    static bool valid(double value) { return std::isfinite(value) && value >= 1 && value <= 0x1p40; }

public:
    static constexpr int capacity = Capacity;
    int size() const { return used; }
    bool insideCallback() const { return callbackRunning; }
    std::uint64_t position() const { return frame; } // callback/buffer boundary position
    bool running() const {
        for (int i = 0; i < used; ++i) if (timers[i].active) return true;
        return false;
    }

    // Called at a host buffer boundary. Preserve remaining wall time for ms timers;
    // tagged sample timers retain their remaining sample count.
    bool sampleRate(double value) {
        if (value == rate) return true;
        if (!(std::isfinite(value) && value > 0)) return false;
        for (int i = 0; i < used; ++i)
            if (!valid(period(timers[i].interval, value))) return false;
        for (int i = 0; i < used; ++i) {
            auto& timer = timers[i];
            if (!timer.active) timer.anchor(double(frame));
            else if (timer.interval.unit == Unit::milliseconds && rate > 0)
                timer.anchor(double(frame) + (timer.next - double(frame)) * value / rate);
            timer.period = period(timer.interval, value);
        }
        rate = value;
        return true;
    }

    Result every(Interval interval, Callback callback, First first, Change change) {
        if (rendering && !callbackRunning) return Result::busy;
        if (!callback.invoke) return Result::invalid;
        for (int i = 0; i < used; ++i) {
            auto& timer = timers[i];
            if (!timer.callback.same(callback)) continue;
            if (timer.active && timer.interval.value == interval.value && timer.interval.unit == interval.unit)
                return Result::unchanged;
            const double duration = period(interval, rate);
            if (!valid(duration)) return Result::invalid;
            const bool restart = !timer.active || change == Change::restart;
            timer.interval = interval;
            timer.period = duration;
            timer.anchor(restart ? double(frame) + duration : timer.next);
            timer.active = true;
            return Result::changed;
        }
        const double duration = period(interval, rate);
        if (!valid(duration)) return Result::invalid;
        // New registrations only during setup/prepare, never while callbacks are draining.
        if (rendering) return Result::busy;
        if (used == Capacity) return Result::full;
        auto& timer = timers[used++];
        timer.callback = callback;
        timer.interval = interval;
        timer.period = duration;
        timer.anchor(double(frame) + (first == First::immediately ? 0 : duration));
        timer.active = true;
        return Result::added;
    }

    bool cancel(Callback callback) {
        if (rendering && !callbackRunning) return false;
        for (int i = 0; i < used; ++i) {
            if (timers[i].callback.same(callback)) {
                timers[i].active = false;
                return true;
            }
        }
        return false;
    }

    // Audio is rendered in spans between events, without per-sample timer polling.
    // Same delivered sample: registration order wins, even for fractional deadlines.
    template<class Render> void run(int count, Render&& render) {
        const auto end = frame + std::uint64_t(count);
        rendering = true;
        while (frame < end) {
            std::uint64_t next = end;
            for (int i = 0; i < used; ++i) {
                if (!timers[i].active) continue;
                const auto due = timers[i].delivery;
                if (due < next) next = due;
            }
            if (next > frame) {
                render(int(next - frame));
                frame = next;
                continue;
            }
            callbackRunning = true;
            for (int i = 0; i < used; ++i) {
                auto& timer = timers[i];
                if (timer.active && timer.delivery <= frame) {
                    // Establish the next pending deadline before user code changes anything.
                    timer.advance();
                    timer.callback();
                }
            }
            callbackRunning = false;
        }
        rendering = false;
    }
};

// Predictable inline storage: no allocation, with selectable per-object capacity.
template<int Capacity> struct Fixed {
    Timers<Capacity> value;
    Timers<Capacity>* get() { return &value; }
    bool reserve() { return true; }
};

// One pointer when unused; explicitly reserve once before starting the audio thread.
template<int Capacity> struct Reserved {
    std::unique_ptr<Timers<Capacity>> value;
    Timers<Capacity>* get() { return value.get(); }
    bool reserve() {
        if (!value) value.reset(new (std::nothrow) Timers<Capacity>);
        return bool(value);
    }
};

enum class Dispatch { branch, functionPointer };
struct NoDispatch {};

// Buffer adapter retaining Klang's prepare(), process(), input and output conventions.
// In a core proposal the registration/storage portion would live in Plugin.
template<class Base, class Buffer, class Storage = Fixed<8>, Dispatch Mode = Dispatch::branch>
class Scheduled : public Base {
    Storage storage;
    using Self = Scheduled<Base, Buffer, Storage, Mode>;
    std::conditional_t<Mode == Dispatch::functionPointer, void (Self::*)(Buffer&), NoDispatch> dispatch{};
    bool rateValid = true;
    bool inAudio = false;
    Result last = Result::unchanged;
    Result error = Result::unchanged;

    Result record(Result result) {
        last = result;
        if (result >= Result::full) error = result;
        return result;
    }

    void plain(Buffer& audio) {
        while (!audio.finished()) {
            this->input(audio);
            this->process();
            audio++ = this->out;
            klang::debug.buffer++;
        }
    }
    static int length(klang::buffer& audio) { return audio.size; }
    static int length(klang::Stereo::buffer& audio) { return audio.left.size; }
    void timed(Buffer& audio) {
        storage.get()->run(length(audio), [&](int count) {
            for (int i = 0; i < count; ++i) {
                this->input(audio);
                this->process();
                audio++ = this->out;
                klang::debug.buffer++;
            }
        });
    }
    void select() {
        if constexpr (Mode == Dispatch::functionPointer)
            dispatch = storage.get() && storage.get()->size() ? &Self::timed : &Self::plain;
    }

public:
    Scheduled() {
        if constexpr (Mode == Dispatch::functionPointer) dispatch = &Self::plain;
    }
    Scheduled(const Scheduled&) = delete; // callbacks refer to this object's identity
    Scheduled& operator=(const Scheduled&) = delete;
    using Base::process;
    bool reserveTimers() { return storage.reserve(); } // startup only, not prepare()
    Result timerResult() const { return last; }
    Result timerError() const { return error; } // sticky even if a subsequent registration succeeds
    void clearTimerError() { error = Result::unchanged; }
    bool timingValid() const { return rateValid; }
    int timerCount() { return storage.get() ? storage.get()->size() : 0; }

    template<class T> Result every(double ms, void (T::*method)(),
                                   First first = First::afterInterval, Change change = Change::keepNext) {
        return every(milliseconds(ms), method, first, change);
    }
    template<class T> Result every(Interval interval, void (T::*method)(),
                                   First first = First::afterInterval, Change change = Change::keepNext) {
        static_assert(std::is_base_of<Self, T>::value, "handler must belong to this model");
        return every(interval, *static_cast<T*>(this), method, first, change);
    }
    template<class T> Result every(double ms, T& target, void (T::*method)(),
                                   First first = First::afterInterval, Change change = Change::keepNext) {
        return every(milliseconds(ms), target, method, first, change);
    }
    template<class T> Result every(Interval interval, T& target, void (T::*method)(),
                                   First first = First::afterInterval, Change change = Change::keepNext) {
        auto* timers = storage.get();
        if (!timers) return record(Result::unavailable);
        if (inAudio && !timers->insideCallback()) return record(Result::busy);
        if (!timers->sampleRate(double(float(klang::fs)))) return record(Result::invalid);
        record(timers->every(interval, Callback::bind(&target, method), first, change));
        if (last == Result::added) select();
        return last;
    }
    template<class T> bool cancel(void (T::*method)()) {
        return cancel(*static_cast<T*>(this), method);
    }
    template<class T> bool cancel(T& target, void (T::*method)()) {
        auto* timers = storage.get();
        if (inAudio && timers && !timers->insideCallback()) return false;
        return timers && timers->cancel(Callback::bind(&target, method));
    }

    void process(Buffer audio) override {
        auto* timers = storage.get();
        rateValid = !timers || !timers->size() || timers->sampleRate(double(float(klang::fs)));
        this->prepare();
        inAudio = true;
        // Invalid rate changes leave timer state untouched; the host can inspect timingValid().
        if (!rateValid) {
            plain(audio);
            inAudio = false;
            return;
        }
        if constexpr (Mode == Dispatch::branch) {
            if (storage.get() && storage.get()->size()) timed(audio);
            else plain(audio);
        } else {
            (this->*dispatch)(audio);
        }
        inAudio = false;
    }
};

using Sound = Scheduled<klang::Sound, klang::buffer>;
using Effect = Scheduled<klang::Effect, klang::buffer>;
using StereoSound = Scheduled<klang::Stereo::Sound, klang::Stereo::buffer>;
using CompactSound = Scheduled<klang::Sound, klang::buffer, Reserved<8>>;
using PointerSound = Scheduled<klang::Sound, klang::buffer, Fixed<8>, Dispatch::functionPointer>;

} // namespace scheduling_trial
