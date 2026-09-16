
#include <klang.h>
using namespace klang::optimised;

#include "klang/pd.h"
#include "klang/utils.h"

struct Turbine : Generator {
	Additive<5> gears;
    param gain = 0;
	
	Turbine() { 
		const Additive<5>::Partials partials = { 
			{ 3097, 0.25 }, 
			{ 4495, 0.25 }, 
			{ 5588, 1 },
			{ 7471, 0.4 }, 
			{ 11000, 0.4 } 
		};
	
		gears = partials;
	}
	virtual ~Turbine() {}
	
	void set(param speed){
		gears.set(0.25 + speed * 0.5);
        if (speed < 0.125)
            gain = speed * 8; // 0 to 1
        else if (speed < 0.25)
            gain = 1;
        else if (speed < 0.75)
            gain = abs(0.5 - speed) * 2 + 0.50;
        else
            gain = 1 - (speed - 0.5);
	}
	
	void process() {
		gears >> out;
		
		if(out > 0.9)
			out = 0.9;
		else if(out < -0.9)
			out = -0.9;

        out *= gain;
	}
};

inline static float clip(float in){
	return in >= 1 ? 1 : in <= -1 ? -1 : in;
}

struct Burn : Generator {
	Noise noise;
    param overdrive = 30;
	
	pd::vcf vcf0, vcf1;
	pd::bpf bpf;
	HPF hpf;
	
	Burn() {
		bpf.set(8000, 0.5);
		hpf.set(120); // DC filter?
	}
	virtual ~Burn() {}
	
	void set(param speed, param altitude){
		vcf0.set(speed * speed * 150, 1);
		vcf1.set(speed * 12000, 0.6);
        overdrive = speed < 0.5 ? 30 : (30 + (speed - 0.5) * 30);

        // ground resistance
        overdrive *= 1 + min(0.25, max(-0.5, speed * (5 - altitude) * 0.2));

        overdrive *= min(altitude, 2) * 0.5;
	}
	
	void process(){
		clip((noise >> bpf >> vcf0 >> hpf) * overdrive) * 0.1f >> vcf1 >> out;
	}
};

struct Harrier : Sound {
	Turbine turbine;
	Burn burn;
	
	pd::lop lop;
	pd::noise noise;
	LPF lpf;
    LPF lpf2;

    Noise wind;
    BPF bpf;

    Delay<192000> echo;

	// Initialise plugin (called once at startup)
	Harrier() {
		controls = { 
			Dial("Speed", 0.0, 1.0, 0.0),
			Dial("Gain", 0.0, 1.0, 0.5),
            Dial("Altitude", 0, 100000, 0),
			Dial("Turbulence", 0.0, 1.0, 0.0),
		};
		
        lpf.set(1000);
		lop.set(11000);
        bpf.set(220, 3);
	}

	// Prepare for processing (called once per buffer)
	void prepare() {
		
	}

	// Apply processing (called once per sample)
	void process() {
		param speed = controls[0].smooth();
		param gain = controls[1];
        param altitude = controls[2];
		param brake = controls[3].smooth();
		
        lop.set(11000 * (1 - speed * 0.5));
		signal whine = turbine(speed);
		whine * (0.03 * (1-speed * 0.5)) + burn(speed, altitude) >> lop >> out;
        

        bpf.set(min(10000, 500 - max(500, altitude / 10.0) + speed * 200), root2);
        param windspeed = max(0.f, (speed - 0.6));

		param turbulence = speed * 3 + brake * 3;
        signal air = (windspeed ^ 2) * (wind >> bpf) * max(0, (min(200, 0.5 * altitude)) * (0.5 + turbulence));
        //air = clip(air * (2 + speed * 2));
        out += air >> lpf(1000 - speed * 500, root2);

        echo.set(max(10, speed * fs));
        out + echo >> lpf2(11000 - speed * 4000) >> out;
        out * max(0, speed * 0.75) >> echo;

		out *= gain;
	}
};
