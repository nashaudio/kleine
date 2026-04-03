
#include <klang.h>
using namespace klang::optimised;

struct Partial {
	Frequency frequency;
	dB gain;
};

static const Partial partials[15] = {
	{86.1f, 44.5f},  {64.6f, 43.8f},   {43.1f, 40.3f},  {53.8f, 37.0f}, 
	{99.6f, 35.9f},  {21.5f, 35.4f},   {110.4f, 33.8f}, {75.4f, 31.5f}, 
	{175.0f, 29.0f}, {118.4f, 28.5f},  {131.9f, 26.1f}, {142.7f, 24.8f}, 
	{166.9f, 18.7f}, {8.1f, 18.4f},    {185.7f, 17.7f}
};

struct MiniNoise : Generator {
    static constexpr int N = 4;
    
    Sine osc[15];
    Amplitude osc_gain[15];
    
    Noise noise;
    Bank<BPF, N> eq;
    Amplitude eq_gain[N];
    Amplitude shelf;
    
    Delay<512> comb;
    
    LPF lpf;
    
	param rate = 1;
    param delta = 1;
    param gas = 0;
    
    param last_rate;

    
    MiniNoise() {
		
    
        const Frequency f[N] = { 65,   1672, 3316, 9717 };
        const param q[N] =     { 0.7,  2.9,  4.9,  6.7 };
        const dB g[N] =    { 14.2, 10.3, 6.9,  1.1 };
        
        for (int i = 0; i < N; ++i) {
            eq[i].set(f[i], q[i]);
            eq_gain[i] = g[i] -> Amplitude;
        }
        
        for (int p = 0; p < 15; p++) {
        	osc[p].set(partials[p].frequency);
        	osc_gain[p] = dB(partials[p].gain - 48) -> Amplitude;
        }
        
        shelf = dB(-25) -> Amplitude;
        
        lpf.set(4000, 5);
    }
    
    
    void set(param rpm, param throttle){
    	rate = max(0.1, rpm / 900);
    	gas = throttle;
    }
    
    void prepate() {
//    	delta = random(0.9, 1.1);
    }

    void process() {
//    	gas = max(0, rate - last_rate * fs);
//    	last_rate = rate * 0.001 + last_rate * 0.999;    	
    	
    
        signal n = noise;
        out = n * shelf;
        for (int i = 0; i < N; ++i)
            out += (n >> eq[i]) * eq_gain[i];
            
        for (int p = 0; p < 15; p++) {
        	osc[p].set(partials[p].frequency * rate * delta * random(0.8, 1.2));
        }
            
        signal engine = 0;
        for (int p = 0; p < 15; ++p)
        	engine += osc[p] * osc_gain[p];
        	
        if(engine > 1.5) engine = 1.5;
        else if (engine < -1.5) engine = -1.5;
        
        signal engine_throttle = (1 - gas) + gas * engine;
        	
        signal am = engine * engine * engine * engine_throttle * out;
        
        signal fb = comb(random(0.001, 0.002) * fs);
        am += gas * fb * 0.99 * max(0, (2 - rate));
        comb << am;
        
        param tone =  gas + max(3, (rate - 1));
        	
        engine * tone + out * 0.1 + (am >> lpf) * 0.075 * (1 + gas) >> out;	
    }
};


struct Mini : Sound {
	MiniNoise noise;

	// Initialise plugin (called once at startup)
	Mini() {
		controls = { 
			Dial("Gain", 0.0, 1.0, 0.5),
			Dial("RPM", 0, 7000, 900),
			Dial("Throttle", 0, 1, 0)
		};
	}

	// Prepare for processing (called once per buffer)
	void prepare() {
		
	}

	// Apply processing (called once per sample)
	void process() {
		param gain = controls[0];
		param rpm = controls[1];
		param throttle = controls[2];
			
		noise(rpm, throttle) * 0.01 * gain >> out;
	}
};