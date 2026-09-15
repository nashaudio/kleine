#pragma once

#include <klang.h>
#include <array>
#include <cstdint>
#include <cstring>
using namespace klang::optimised;

namespace pd {

// Primitive ports adapted from Pure Data 0.55-2, src/d_osc.c,
// src/d_osc.h, src/d_ctl.c and src/d_filter.c.
// Copyright (c) 1997-2024 Miller Puckette.
// Redistribution terms: ../../licenses/Pure-Data-BSD.txt.

// Pd osc~: cosine table interpolation, frequency in Hz and phase in cycles.
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
struct wrap : Modifier {
    void process() override { out = float(in) - std::floor(float(in)); }
};

// Pd line~: block-quantised ramps; deliver control messages on the 64-sample grid.
// Adapted from Pure Data 0.55-2 src/d_ctl.c, Miller Puckette, BSD (see above).
struct line : Generator {
    float value = 0, target = 0, increment = 0, blockIncrement = 0, sample = 0;
    int blocks = 0, position = 0;
    void set(param destination) override {
        value = target = destination;
        blocks = 0;
    }
    void set(param destination, param milliseconds) override {
        if (milliseconds <= 0) { set(destination); return; }
        target = destination;
        blocks = std::max(1, int(float(milliseconds) * (float(fs) / 64000.0f)));
        blockIncrement = (target - value) / blocks;
        increment = blockIncrement / 64;
    }
    void stop() { target = value; blocks = 0; }
    void process() override {
        if (position == 0) sample = blocks ? value : (value = target);
        out = sample;
        if (blocks) sample += increment;
        if (++position == 64) {
            if (blocks) { value += blockIncrement; --blocks; }
            position = 0;
        }
    }
};

// Pd env~ default: 1024-point Hann RMS, 512-sample hop, PD's 100 dB unity reference.
// This initial port fixes the analysis/block sizes used by Farnell's tone detector.
struct env : Modifier {
    inline static const std::array<float, 1088> window = [] {
        std::array<float, 1088> values{};
        for (int i = 0; i < 1024; ++i) values[i] = float((1 - std::cos(2 * 3.14159 * i / 1024)) / 1024);
        return values;
    }();
    std::array<float, 64> buffer{};
    std::array<float, 3> sums{};
    int position = 0, phase = 0;
    float level = 0;
    bool updated = false;
    void process() override {
        out = level; // Control output becomes visible in the following DSP block.
        updated = false;
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
            level = sums[0] > 0 ? std::max(0.0f, float(100 + (10 / std::log(10.0)) * std::log(double(sums[0])))) : 0;
            sums[0] = sums[1]; sums[1] = 0;
            phase = 512 - 64;
            updated = true;
        }
    }
};

// Pd-style lop~ (gentle one-pole low-pass) for Klang
// Matches Pure Data’s lop~ behaviour closely
static inline void flush_denormal(float& x) {
    constexpr float DENORM_LIMIT = std::numeric_limits<float>::min();
    x = (std::fabs(x) < DENORM_LIMIT) ? 0.0f : x;
}


// Pd lop~: a one-pole lowpass with a frequency control in Hz.
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
        coef = freq * fs.w;
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
	if (f >= -0.5f * pi && f <= 0.5f * pi) {
		float g = f * f;
		return (((g*g*g * (-1.0f/720.0f) + g*g*(1.0f/24.0f)) - g*0.5f) + 1.0f);
	}
	return 0.0f;
}

// Pd bp~: a two-pole bandpass with frequency and Q controls.
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
        float omega = freq * fs.w;
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

// Pd-style vcf~ for Klang
// Signal input: in()
// Params: freq (Hz), q (resonance)
// Behaviour: per-sample coefficient update, 2-pole resonant band-pass

// Pd vcf~: a complex resonator exposing real and imaginary outputs.
struct vcf : Modifier {
    param freq = 0;
    param q    = 0;
    
    signal re = 0, im = 0;
    
    signal& lpf = out;
    signal& bpf = im;

    signal x1 = 0, x2 = 0;  // previous states
    signal coef1 = 0, coef2 = 0, gain = 0, r = 0;
    
    signal cos, sin;
    
    void set(param f) {
    	set(f, q);
    }

    void set(param f, param Q){
    	f = max(0.001f, f);
    	Q = max(0.001f, Q);
    	if(freq != f || q != Q){
    		freq = f;
    		q = Q;
    		update();
    	}
    }

    inline void update() {
        float omega = freq * fs.w;
        float oneminusr = (q < 0.001f ? 1.f : omega / q);
        if (oneminusr > 1.f) oneminusr = 1.f;
        r = 1.f - oneminusr;

        coef1 = 2.f * fastcos(omega) * r;
        coef2 = -r * r;
        gain  = 2.f * oneminusr;
        
        omega = freq * fs.w;
        if (omega < 0.f) omega = 0.f;
        
        const float qinv = (q > 0.f) ? (1.0f / q) : 0.0f;
        r = (qinv > 0.f) ? (1.0f - omega * qinv) : 0.0f;
        if (r < 0.f) r = 0.f;
        
        cos = fastcos(omega);
        sin = fastsin(omega);
        
        gain = 2.0f - 2.0f / (q + 2.0f); // Pd’s formula
    }

    void process() {
        const float coefr = r * cos;
        const float coefi = r * sin;

        // Update (complex resonator)
        re = gain * (1 - r) * in + coefr * out - coefi * im; // LPF state
        im = coefi * out + coefr * im;                       // BPF state

        // Outputs
        out = re;

		flush_denormal(out);
		flush_denormal(im);
    }
};

};
