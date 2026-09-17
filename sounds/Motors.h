#include <klang.h>
using namespace klang::optimised;

// "A Toy Boat Engine"
// from Designing Sound (Farnell, 2010, p511).

static signal clip_0_1(signal in){
	return in < 0.f ? signal(0) : in > 1.f ? signal(1) : in;
}

struct ToyBoatEngine : Generator {
	// signal generators
	Noise noise;
	Sine osc;
	
	// shaping
	BPF bp_9_15, bp_590_4;
	OnePole::HPF hip_10, hip_1000, hip_100;
	OnePole::LPF lop_30;
	
	// body resonances
	Bank<BPF, 3> body;
	
	bool brk = false;
	
	ToyBoatEngine() {
		osc.set(9);
		bp_9_15.set(9, 15);
		
		hip_10.set(10);
		lop_30.set(30);
		
		hip_1000.set(1000);
		bp_590_4.set(590, 4);
				
		body[0].set(470, 8);
		body[1].set(780, 9);
		body[2].set(1024, 10);
		
		hip_100.set(100);
	}
	virtual ~ToyBoatEngine() {}
	
	void set(param b){
		brk = b != 0;
	}
	
	void process() {
		signal mix = 0;
		
		if(brk) // engine broken?
		 	noise >> bp_9_15 >> mix; // sputtering noise
		else
		 	osc >> mix; // regular pulse
		
		// exhaust outlet valve
		clip_0_1(mix * 600.0) >> hip_10 >> lop_30 >> mix;
		
		// formant filter (enveloped high-pass-filtered noise)
		mix *= noise >> hip_1000 >> bp_590_4;
		
		// tonal shaping (e.g. body resonances)
		mix >> body >> mix >> hip_100 >> out;
			
		// amplify output
		out *= 10;
	}
	
};

// Legacy model with audio-rate random delay-tap jumps; retained for reference,
// superseded by Mini, and excluded from the production acceptance corpus.
struct FourStrokeEngine : Generator {
	virtual ~FourStrokeEngine() {}

	param speed;
	
	Phasor phasor;
	Delay<3840> a, b;

	Noise noise;
	LPF lpf;
	BPF bpf;
	HPF hpf;

	DCF dc;
	
	void set(param spd){
		speed = spd;
		phasor.set(speed * 10);
		
		lpf.set(15);
		hpf.set(100);
		bpf.set(400, 0.5);
	}

	void process() {
		signal n = noise >> lpf;
		n * 30 >> b;
		n * 0.5 >> a;

		signal n2 = noise >> bpf(200 + speed * 400);// lop[1];
		n2 = 1.0 - (speed + 0.1) * n2 * 0.01;
		
		const signal i = phasor * 4;// *random(0.99, 1.0);;
		const signal s = 22 - speed * 15;
		const signal ms = fs / 250 *random(0.99, 1.0);
		
		out = 0;
		for(int d = 0; d < 4; d++){
			const param t = (d + 1) * 5 * ms;
			const param phase = -(0.75 - d * 0.25) * n2;
			
			signal mix = cos( (a(t) + i + phase) * 2 * pi );
			mix *= b(t) + s;
			
			out += 1 / (mix * mix + 1);
		}

		//out >> hpf 
		
		out * speed * min(speed,0.25) >> hpf >> dc >> out;
	}
};

// Legacy wrapper around the known-broken FourStrokeEngine model.
struct Car : Sound {

	FourStrokeEngine engine;

	Car () {
		controls = { Dial("RPM", 0, 7000.f, 0.f) };
	}

	void prepare() override {
		param rpm = controls[0] / 7000.f;// max(0.075, controls[0] / 20000.f);
		engine.set(rpm);
	}

	void process() override {
		engine >> out;

		//const float normalise = 0.25f / tanh(10);
		0.25 * tanh(out * 3) / tanh(3) >> out;

		//out *= 10;
		//if (out > 1) out = 1;
		//else if (out < -1) out = -1;
		//out *= 1.0/10.0;
	}
};
