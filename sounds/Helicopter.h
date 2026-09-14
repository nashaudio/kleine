#include <klang.h>
using namespace klang::optimised;

#include "klang/pd.h"
#include "klang/utils.h"

struct Helicopter : Sound {

	struct Gears : Generator {
		Additive<5> gears;
		Additive<4> sidebands;
		param gain = 0;
		param last_speed = 0;
	
		Gears() { 
			const Additive<5>::Partials partials = { 
				{ 3097, 0.25 }, 
				{ 4495, 0.25 }, 
				{ 5588, 1 },
				{ 7471, 0.4 }, 
				{ 11000, 0.4 } 
			};
			gears = partials;

			const Additive<4>::Partials sb_partials = {
				{ 21, dB(-8)->Amplitude },
				{ 42, dB(-16)->Amplitude },
				{ 63, dB(-22)->Amplitude },
				{ 3500, dB(-60)->Amplitude },
			};
			sidebands = sb_partials;
		}
		virtual ~Gears() {}
	
		void set(param speed) {
			gears.set(0.125 + speed * 0.25);
			sidebands.set(gears.frequency);

			if (speed >= last_speed){
				if (speed < 0.125)
					gain = speed * 8; // 0 to 1
				else if (speed < 0.25)
					gain = 1;
				else if (speed < 0.75)
					gain = abs(0.5 - speed) * 2 + 0.50;
				else
					gain = 1 - (speed - 0.5);
			} else {
				if (speed < 0.125)
					gain = (speed * 8) ^ 2; // 0 to 1
				else if (speed < 0.25)
					gain = 1;
				else if (speed < 0.75)
					gain = abs(0.5 - speed) * 2 + 0.50;
				else
					gain = 1 - (speed - 0.5);
			}
			last_speed = speed;
		}
	
		void process() {
			gears >> out;
			out *= 1 + 0.4 * sidebands;
		
			if(out > 0.9)
				out = 0.9;
			else if(out < -0.9)
				out = -0.9;

			out *= gain;
		}
	};

	struct Rotor : Generator {
		static constexpr int N = 4;
    
		Additive<15> hum;
    
		Noise noise;
		Bank<BPF, N> eq;
		Amplitude eq_gain[N];
		Amplitude shelf;
    
		Delay<512> comb;
    
		LPF lpf;
		OnePole::LPF throttle_lpf;
    
		param rpm = 0, throttle = 0, altitude = 0;
		signal rate = 0, power = 0;

		Rotor() {
			//const Frequency f[N] = { 65,   1672, 3316, 9717 };
			//const param q[N] =     { 3,    3,    6,    6    };
			//const dB g[N] =        { 14.2, 10.3, 6.9,  1.1  };
			//shelf = dB(-25) -> Amplitude; // -25dB noise shelf
			//for (int i = 0; i < N; ++i) {
			//	eq[i].set(f[i], q[i]);
			//	eq_gain[i] = g[i] -> Amplitude;
			//}

			//const Additive<15>::PartialsDB partials = {
			//	{86.1f, 44.5f},  {64.6f, 43.8f},   {43.1f, 40.3f},  {53.8f, 37.0f}, 
			//	{99.6f, 35.9f},  {21.5f, 35.4f},   {110.4f, 33.8f}, {75.4f, 31.5f}, 
			//	{175.0f, 29.0f}, {118.4f, 28.5f},  {131.9f, 26.1f}, {142.7f, 24.8f}, 
			//	{166.9f, 18.7f}, {8.1f, 18.4f},    {185.7f, 17.7f}
			//};

			const Frequency f[N] = {
				 45.f,   // low-frequency pressure/body
				180.f,   // upper rotor harmonics / blade slap
				850.f,   // turbulent blade noise
			   4200.f    // tip hiss
			};

			const param q[N] = {
				0.8f,    // broad, not resonant
				1.0f,
				0.7f,
				0.6f
			};

			const dB g[N] = {
				11.0f,
				 7.0f,
				 3.5f,
				 1.5f
			};

			shelf = dB(-25) -> Amplitude;

			for (int i = 0; i < N; ++i) {
				eq[i].set(f[i], q[i]);
				eq_gain[i] = g[i] -> Amplitude;
			}

			constexpr float R   = 214.f / 60.f; // 3.5667 Hz
			constexpr float BPF = R * 5.f;      // 17.8333 Hz

			const Additive<15>::PartialsDB partials = {
				{  1.f * BPF, 44.5f },  //  17.83 Hz:  5R
				{  2.f * BPF, 39.5f },  //  35.67 Hz: 10R
				{  3.f * BPF, 35.5f },  //  53.50 Hz: 15R
				{  4.f * BPF, 32.0f },  //  71.33 Hz
				{  5.f * BPF, 29.0f },  //  89.17 Hz
				{  6.f * BPF, 26.5f },  // 107.00 Hz
				{  7.f * BPF, 24.0f },  // 124.83 Hz
				{  8.f * BPF, 22.0f },  // 142.67 Hz
				{  9.f * BPF, 20.0f },  // 160.50 Hz
				{ 10.f * BPF, 18.5f },  // 178.33 Hz
				{ 11.f * BPF, 17.0f },  // 196.17 Hz
				{ 12.f * BPF, 15.5f },  // 214.00 Hz
				{ 13.f * BPF, 14.0f },  // 231.83 Hz
				{ 14.f * BPF, 12.5f },  // 249.67 Hz
				{ 15.f * BPF, 11.0f }   // 267.50 Hz
			};

			hum = partials - dB(48);
 		}
    
		virtual ~Rotor() {}

		// soft clip distortion (for exhaust noise)
		static signal softclip(signal x, signal threshold = 1.5f, signal slope = 1.5f) {
			return threshold * FMath::Tanh(x * slope / threshold);
		}
    
		// configure engine (based on UE Chaos Vehicle)
		void set(param _rpm, param _throttle, param _altitude){
			power = _rpm == 0 ? 0 : 1;
    		rpm = max(0.0, _rpm); // 17Hz
			//rpm -= 0.05 * (rpm * rpm);  // slightly non-linear revs (energy loss at high revs)
    		throttle = _throttle;
			altitude = _altitude;
		}

		//void prepare() {
		//	const Frequency f[N] = {
		//		 45.f,   // low-frequency pressure/body
		//		180.f,   // upper rotor harmonics / blade slap
		//		850.f,   // turbulent blade noise
		//	   4200.f    // tip hiss
		//	};

		//	const param q[N] = {
		//		0.8f,    // broad, not resonant
		//		1.0f,
		//		0.7f,
		//		0.6f
		//	};

		//	const dB g[N] = {
		//		11.0f,
		//		 7.0f,
		//		 3.5f,
		//		 0.5f
		//	};

		//	for (int i = 0; i < N; ++i) {
		//		eq[i].set(f[i], q[i] * 3);
		//		eq_gain[i] = g[i] -> Amplitude;
		//	}
		//}
    
		// generate audio
		void process() {
			// skip processing when idle
			if (power == 0) {
				out = 0;
				return;
			}

			// boost audible rpm for pulling away and accelerating
			signal new_rate = (rpm + min(throttle, 0.707)) * power;
			param gain = power * rpm * rpm;
			//throttle_lpf.set(0.5);

			// rev up is slower than rev down
			//if(new_rate > rate)
				//rate = rate * 0.9999 + 0.0001 * new_rate;
			//else
				//rate = rate * 0.999 + 0.001 * new_rate;
			rate = rpm;
			
			// engine noise 
			const signal n = noise;
			signal engine_noise = n * shelf;
			for (int i = 0; i < N; ++i)
				engine_noise += (n >> eq[i]) * eq_gain[i];
            
			// engine tone (resynthesised from Mini recording)
			//additive.set(rate, +signal(0.2));
			/*for (int p = 0; p < 15; p++)
				osc[p].set(partials[p].frequency * rate * random(0.8, 1.2));*/
			signal engine_tone = hum(rate, +signal(0));
			//for (int p = 0; p < 15; ++p)
   //     		engine_tone += osc[p] * osc_gain[p];
        	
			// distort to emulate exhaust rasp
			engine_tone = softclip(engine_tone, 1.5 - altitude * 0.5, 1 + altitude * 0.25);
        
			// amplify exhaust for overrevs
			signal engine_throttle = (0.5 + throttle * 0.5);

			// modulate noise with engine tone
			signal am = engine_tone * engine_tone * engine_tone * engine_throttle * engine_noise;       

			// add slight resonance with comb filtering
			//signal fb = comb(0.001 * fs);
			//am += fb * 0.99;// *max(0, (2 - rate));
			//comb << am;

			// shape noise character based on revs with additional resonance for overreving
			lpf.set(5000 * (1.f + sqr(rate / 14.f)), 2 - altitude * 1);
        
			// attenuate engine tone for higher revs
			param tone = max(0.5, 1 - abs(sqr(rate * 0.25 - 0.75))) * (1 - (rate - 1) * 0.01f);

			// mix together in proportion (based on revs and overrevs)
			engine_tone * tone + engine_noise * (rate * 0.02)  + (am >> lpf) * 0.075 >> out;
			out *= gain;
		}
	};
	
	struct Tail : Rotor {
		Tail() {
			constexpr float TailRPS = 15.766f;
			constexpr float TailBPF = 4.f * TailRPS; // 63.064 Hz
			const Frequency f[N] = {
					180.f,  // lower chopped-air component
					720.f,  // blade loading / turbulent buzz
				2200.f,  // aerodynamic rasp
				6500.f   // blade-tip hiss
			};

			const param q[N] = {
				1.2f,
				0.85f,
				0.95f,
				0.7f
			};

			const dB g[N] = {
				4.0f,
				9.5f,
				7.0f,
				2.5f
			};

			shelf = dB(-34.f) -> Amplitude;

			for (int i = 0; i < N; ++i) {
				eq[i].set(f[i], q[i] * 3);
				eq_gain[i] = g[i] -> Amplitude;
			}

			const Additive<15>::PartialsDB partials = {
				{  63.06f, 31.0f }, // 1 × BPF
				{ 126.13f, 43.0f }, // 2 × BPF: perceptual centre
				{ 189.19f, 39.5f },
				{ 252.26f, 36.0f },
				{ 315.32f, 33.0f },
				{ 378.38f, 30.5f },
				{ 441.45f, 28.0f },
				{ 504.51f, 25.5f },
				{ 567.58f, 23.0f },
				{ 630.64f, 21.0f },
				{ 693.70f, 19.0f },
				{ 756.77f, 17.5f },
				{ 819.83f, 16.0f },
				{ 882.90f, 14.5f },
				{ 945.96f, 13.0f }
			};

			hum = partials - dB(52.f);
		}
	};

	pd::lop lop;
	HPF hpf;
	LPF lpf;

	Gears gears;
	Rotor rotor;
	Tail tail;

	// Initialise plugin (called once at startup)
	Helicopter() {
		controls = { 
			Dial("Speed", 0.0, 1.0, 0.0),
            Dial("Altitude", 0, 100000, 0),
			Dial("Rise", -1, 1, 0),
		};
	}

	// Apply processing (called once per sample)
	void process() {
		param speed = controls[0].smooth();
        param altitude = controls[1];
		param rise = controls[2];

		lop.set(11000 * (1 - speed * 0.5));
		gears(speed)* (0.01 * (1 - speed * 0.125)) >> lop >> out;
		
		param altitude_factor = param(max(0.f, min(1.f, altitude / 4000.f)));
		lpf.set(7500 - altitude_factor * 4500, 0.5);
		hpf.set(50 + (altitude_factor ^ 2) * 50);
		param throttle = max(0.0, (speed - 0.5) * 2); // 0.0 to 1.0 (50% to 100% speed)
		out += rotor(speed, throttle, altitude_factor);
		//out += tail(speed, throttle, altitude_factor) * 0.05; // keep?
		//out *= 1 + tail_out * 0.25;
		out = out >> hpf >> lpf;
			
		//engine(ignition, rpm, throttle, gear) * 0.1 >> out;
	}
};