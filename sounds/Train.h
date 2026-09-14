
#include <klang.h>
using namespace klang::optimised;

#include "Reverb.k"

signal lerpLin(signal a, signal b, signal t){
	return a + (b - a) * t;
}

signal lerpLog(signal a, signal b, signal t){
	return a * ((b / a) ^ t);
}

struct Whistle : Oscillator {
	Noise noise;
		
	BPF resonance[5];
	LPF lpf;
	HPF hpf;
	DCF dc;
	BPF bpf;
		
	ADSR adsr;
	Envelope tone;
	param f0;
		
	inline static float saturate(float in){
		return tanh(in * 10.f);
	}		

	Whistle() {
		adsr.release();
	}

	void set(param f){
		f0 = f;
		setFrequency(f);
		bpf.set(f0 * 8, 2);
	}
		
	void setFrequency(param f){
		constexpr float ratio[5]  = { 1.00f, 1.52f, 2.02f, 2.38f, 3.01f };
		constexpr float Q[5] = { 900, 1000, 1100, 1200, 1400 };
		for(int r = 0; r < 5; r++)
			resonance[r].set(f * ratio[r], Q[r]);
	}
		
	void start(){
		tone.initialise();
		tone = { { 0, 0 }, { 0.5, 1 } };
			
		adsr.initialise();
		adsr(0.25, 0, 1, 0.5);
	}
		
	void release(){
		adsr.release();
		tone.release(adsr.R * 4);
	}
	
	bool finished(){
		return adsr.finished();
	}
		
	void process(){
		if (adsr.finished())
			return; // not active

		signal env = adsr;
		signal steam = noise;
		signal hiss = steam * env;
		out  = 0.001 * hiss >> bpf;
		out += 0.002 * steam * (env + 0.5 * (1 - env)) >> hpf(12500 - env * 2500);

		setFrequency(f0 + tone * f0 * 0.05);
		const param gain[5] = { 1.0, 0.9, 0.8, 0.7, 0.6 };
		for(int r = 0; r < 5; r++)
			out += (steam >> resonance[r]) * gain[r];

		saturate(out) * env >> dc >> out;
	}
};

struct Funnel : Generator {
	Noise noise;
	BPF resonance;
	LPF rolloff, bass;
	
	struct Pressure : Generator {
		Sine lfo1;
		Saw lfo2;
		param phase;
		param gain;
		
		virtual ~Pressure() = default;
					
		void set(param speed, param new_phase, param direction) {
			gain = min(0.5, speed * 10000);

			if (direction < 0)
				new_phase = fmod(2 * pi - new_phase + 5 * pi / 4, 2 * pi);

			phase = 0.01 * new_phase + 0.99 * phase;
			lfo1.set(speed * 28.0, phase + pi/2);
			lfo2.set(speed * 28.0, phase);
		}
		
		void process() {
			signal mod1 = lfo1 * 0.5 + 0.5;
			signal mod2 = lfo2 * 0.5 + 0.5;
			((mod1 ^ 2) + (mod2 ^ 2)) * gain >> out;
		}
	};
	
	Pressure pressure;
	param speed;
	param direction;
		
	Funnel() {
		rolloff.set(0);
		resonance.set(3200, 1.2);
		bass.set(600);
	}

	virtual ~Funnel() = default;
	
	void set(param s, param p){
		direction = s < 0 ? -1 : 1;
		s = abs(s);
		speed = (s / 180) ^ 1.6;
		
		//param phase = lerpLin(-3 * pi / 2, p, min(1, speed * 10000));
 		pressure.set(speed, p, direction);
		
		signal es = 6 * (s ^ 5) - 15 * (s ^ 4) + 10 * (s ^ 3);
		
		rolloff.set(lerpLog(4500, 12000, speed));
		resonance.set(lerpLog(1800, 3200, speed), lerpLin(1.5, 0.8, es));
	}
		
	void process(){
		signal steam = pressure * (0.5 - speed) + 0.25 + speed * 0.5;
		signal boost = 2 + speed * max(0, 1 - speed);
		steam *= boost; 
		
		param gain = 0.01 + 0.99 * (speed ^ 0.9);
		
		signal n = noise;
		out = (1 - speed * 3) * (n >> resonance) * 0.5;
		out += (n >> rolloff) * (1 + speed);

		signal chuffing = n >> bass;
		out += steam * chuffing * (8 - speed * 8);

		out *= steam * gain * 2 * (1 - speed);
	}
};

// cast to -1 (down) / +1 (up) on input change
struct gate : public Modifier {
	//using Modifier::operator>>;
//public:
	signal last = 0;
	int state = 0;

	//void process() {
	//	
	//}

	operator int() {
		return state;
	}

	bool operator==(int value) const {
		return state == value;
	}

	void input() override {
		if (in > 0.5 && in > last)
			state = 1;
		else if (in < 0.5 && in < last)
			state = -1;
		else
			state = 0;
		last = in;
	}

	bool on() const { return state == 1; }
	bool off() const { return state == -1; }
	bool triggered() const { return state != 0; }
};

struct Train : Stereo::Sound {
	DCF dc;

	Funnel funnel;
	Whistle whistle;
		
	gate ignition;
	gate whistling;
	Envelope fire;

	Reverb2 reverb;

	// Initialise plugin (called once at startup)
	Train() {
		controls = {
			Dial("Speed (MPH)", -100, 100, 0),
			Dial("Phase", -8 * pi, 8 * pi, 0.0),
			Dial("Steam", 0.0, 1.0, 0.0),
			Toggle("Whistle"),
			Dial("Ignition")
		};

		fire = { { 0, 0 } };
		whistle.set(Pitch(65)->Frequency); // F4

		reverb.controls[0].set(0.5); // Resonance 0 - 0.5
		reverb.controls[1].set(0.2); // Room size 0 - 0.4
//		reverb.controls[2].set(5000); // Brightness 0 - 5000
	}

	// Apply processing (called once per sample)
	void process() {
		param speed = controls[0];
		param phase = controls[1];

		controls[3] >> whistling;
		if (whistling.on()) {
			whistle.start();
		} else if(whistling.off()) {
			whistle.release();
		}

		controls[4] >> ignition;
		if (ignition.on()) {
			fire.initialise();	// restart starter envelope
			fire = { {0, 0 }, { 3, 1 } };
		} else if(ignition.off()) {
			fire.release(3);
		}

		signal steam = funnel(speed, phase);
		fire * steam >> dc >> out;
		out += whistle * 2 >> reverb;
	}
};