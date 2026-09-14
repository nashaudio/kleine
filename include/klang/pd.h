#pragma once

#include <klang.h>
using namespace klang::optimised;

namespace pd {

// Pd-style lop~ (gentle one-pole low-pass) for Klang
// Matches Pure Data’s lop~ behaviour closely
static inline void flush_denormal(float& x) {
    constexpr float DENORM_LIMIT = std::numeric_limits<float>::min();
    x = (std::fabs(x) < DENORM_LIMIT) ? 0.0f : x;
}


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

struct noise : Generator {
    int32_t val = 0x12345678; // seed

    void process() {
        // same LCG constants as Pd
        val = val * 435898247 + 382842987;
        // mask, recenter, scale
        int32_t out_i = (val & 0x7fffffff) - 0x40000000;
        out = float(out_i) * (1.0f / 0x40000000);
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