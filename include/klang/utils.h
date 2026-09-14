#pragma once

#include <klang.h>
using namespace klang::optimised;

template<int PARTIALS>
struct Additive : Oscillator {
    struct Partial {
		Frequency frequency;
		Amplitude gain;
    };

	struct PartialDB {
		Frequency frequency;
		dB gain;
	};

	struct Partials : Array<Partial, PARTIALS> {
		using Array<Partial, PARTIALS>::Array;

		Partials operator+(param x) const {
			Partials result;
			for (int i = 0; i < PARTIALS; i++)
				result.add({ (*this)[i].frequency, (*this)[i].gain + x });
			return std::move(result);
		}

		Partials operator-(param x) const {
			return operator+(-x);
		}
	};

	struct PartialsDB : Array<PartialDB, PARTIALS> {
		using Array<PartialDB, PARTIALS>::Array;

		PartialsDB operator+(param x) const {
			PartialsDB result;
			for (int i = 0; i < PARTIALS; i++)
				result.add({ (*this)[i].frequency, (*this)[i].gain + x });
			return std::move(result);
		}

		PartialsDB operator-(param x) const {
			return operator+(-x);
		}
	};

	Partials partials;
	
	Sine osc[PARTIALS];
	
	Additive& operator=(const Partials& in){
		partials = in;
		return *this;
	}

	Additive& operator=(const PartialsDB& in){
		for (int p = 0; p < PARTIALS; p++) {
			partials[p].frequency = in[p].frequency;
			partials[p].gain = dB(in[p].gain) -> Amplitude;
		}
		return *this;
	}
	
	void set(param f) {
		for(int p=0; p < PARTIALS; p++)
			osc[p].set(f * partials[p].frequency);
	}

	void set(param f, relative rnd) {
		for(int p=0; p < PARTIALS; p++)
			osc[p].set(f * random(1 - rnd, 1 + rnd) * partials[p].frequency);
	}
	
	void set(param f, param p) {
		for(int o=0; o < PARTIALS; o++)
			osc[o].set(f * partials[o].frequency, p);
	}

	void set(param f, param p, relative rnd) {
		for(int o=0; o < PARTIALS; o++)
			osc[o].set(f * random(1 - rnd, 1 + rnd) * partials[o].frequency, p);
	}
	
	void process() {
		out = 0;
		for(int p=0; p < PARTIALS; p++)
			out += osc[p] * partials[p].gain;
	}
};