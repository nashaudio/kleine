#pragma once

#include <klang.h>
using namespace klang::optimised;

#include "klang/utils.h"

struct Mini : Sound {

	struct Engine : Generator {
		static constexpr int N = 4;
    
		Additive<15> hum;
    
		Noise noise;
		Bank<BPF, N> eq;
		Amplitude eq_gain[N];
		Amplitude shelf;
    
		Delay<512> comb;
    
		LPF lpf;
		OnePole::LPF throttle_lpf;
    
		param rpm = 0, throttle = 0, ignition = 0;
		signal rate = 0, gas = 0, power = 0;
		param gear = 0;

		Envelope starter, rev;

		Engine() {
			const Frequency f[N] = { 65,   1672, 3316, 9717 };
			const param q[N] =     { 3,    3,    6,    6    };
			const dB g[N] =        { 14.2, 10.3, 6.9,  1.1  };
			shelf = dB(-25) -> Amplitude; // -25dB noise shelf
			for (int i = 0; i < N; ++i) {
				eq[i].set(f[i], q[i]);
				eq_gain[i] = g[i] -> Amplitude;
			}

			const Additive<15>::PartialsDB partials = {
				{86.1f, 44.5f},  {64.6f, 43.8f},   {43.1f, 40.3f},  {53.8f, 37.0f}, 
				{99.6f, 35.9f},  {21.5f, 35.4f},   {110.4f, 33.8f}, {75.4f, 31.5f}, 
				{175.0f, 29.0f}, {118.4f, 28.5f},  {131.9f, 26.1f}, {142.7f, 24.8f}, 
				{166.9f, 18.7f}, {8.1f, 18.4f},    {185.7f, 17.7f}
			};

			hum = partials - dB(48);
			starter = { { 0, 0 } };
 		}
    
		virtual ~Engine() {}

		// soft clip distortion (for exhaust noise)
		static signal softclip(signal x, signal threshold = 1.5f, signal slope = 1.5f) {
			return threshold * tanh(x * slope / threshold);
		}
    
		// configure engine (based on UE Chaos Vehicle)
		void set(param _ignition, param _rpm, param _throttle, param _gear){

			// start engine
			if (_ignition > ignition) {
				starter.initialise();	// restart starter envelope
				rev.initialise();		// restart rev envelope
				throttle_lpf.set(0.05);	// slow throttle response for startup

				// delayed start, followed by revs at ignition
				const param delay = random(0.25, 0.75);
				starter = { {0, .5 },                   { delay, 1 },                                     { delay + .25, 2 }, { delay + .5, 1 } };
				rev = {     {0, 0  }, { delay - .1, 0}, { delay, 1 },       { delay + .125, random(2,5)}, { delay + .25, 0},  { delay + 5, 0}};
			} 
			
			// stop engine
			else if (_ignition < ignition) {
				starter.release(2);
			}

			ignition = _ignition;
    		rpm = max(0.0, _rpm / 900); // 900rpm = 1.0 (idle)
			rpm -= 0.05 * (rpm * rpm);  // slightly non-linear revs (energy loss at high revs)
    		throttle = _throttle;
			gear = _gear;				// (not currently used)
		}
    
		// generate audio
		void process() {
			// starter envelope (settles on 1.0)
			power = starter;

			// skip processing when idle
			if (power == 0) {
				out = 0;
				return;
			}

			// boost audible rpm for pulling away and accelerating
			signal new_rate = (rpm + rev + min(throttle, 0.707)) * power;
			if(rev.finished())
				throttle_lpf.set(0.5);
			
			// rev up is slower than rev down
			if(!rev.finished() || new_rate > rate)
				rate = rate * 0.9999 + 0.0001 * new_rate;
			else
				rate = rate * 0.999 + 0.001 * new_rate;

			// separate signal for overrev ('flooring it') 
			gas = max(0, ((throttle - 0.5) * 2)) >> throttle_lpf;
			gas *= 1 - min(0.75, abs(sqr(rate * 0.125)));
			const signal gas_2 = gas * gas;

			// engine noise 
			const signal n = noise;
			signal engine_noise = n * shelf;
			for (int i = 0; i < N; ++i)
				engine_noise += (n >> eq[i]) * eq_gain[i];
            
			// engine tone (resynthesised from Mini recording)
			//additive.set(rate, +signal(0.2));
			/*for (int p = 0; p < 15; p++)
				osc[p].set(partials[p].frequency * rate * random(0.8, 1.2));*/
			signal engine_tone = hum(rate, +signal(0.2));
			//for (int p = 0; p < 15; ++p)
   //     		engine_tone += osc[p] * osc_gain[p];
        	
			// distort to emulate exhaust rasp
			engine_tone = softclip(engine_tone, 1.5, 1 + gas * gas * 0.25);
        
			// amplify exhaust for overrevs
			signal engine_throttle = (1 - gas) + gas * engine_tone;
			engine_throttle *= (0.5 + throttle * 0.5 + gas * gas * 0.1 * min(1, sqr(7.5 - rate) / 50.f + 0.125f));

			// modulate noise with engine tone
			signal am = engine_tone * engine_tone * engine_tone * engine_throttle * engine_noise;       

			// add slight resonance with comb filtering
			signal fb = comb(random(0.001, 0.002) * fs);
			am += gas * fb * 0.99 * max(0, (2 - rate));
			comb << am;

			// shape noise character based on revs with additional resonance for overreving
			lpf.set(5000 * (1.f + sqr(rate / 14.f)), max(1, 5 + gas_2 * 5/* - (rate - 1) * 1*/));
        
			// attenuate engine tone for higher revs
			param tone = max(0.5, 1 - abs(sqr(rate * 0.25 - 0.75))) * (1 - (rate - 1) * 0.01f);

			// mix together in proportion (based on revs and overrevs)
			engine_tone * tone + engine_noise * (rate * 0.02)  + (am >> lpf) * 0.075 * (1 + gas * gas) >> out;
			out *= power;
		}
	};

	Engine engine;

	// Initialise plugin (called once at startup)
	Mini() {
		controls = { 
			Toggle("Ignition"),
			Dial("RPM", 0, 7000, 000),
			Dial("Throttle", 0, 1, 0),
			Dial("Gear", -1, 5, 0)
		};
	}

	// Apply processing (called once per sample)
	void process() {
		param ignition = controls[0];
		param rpm = controls[1];
		param throttle = controls[2];
		param gear = controls[3];
			
		engine(ignition, rpm, throttle, gear) * 0.1 >> out;
	}
};
