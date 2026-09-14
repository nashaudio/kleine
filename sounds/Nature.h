#include <klang.h>
using namespace klang::optimised;

struct Rain : Sound {
	Noise noise;

	Rain () {
		/*controls = { Dial("RPM", 0, 7000.f, 0.f) };*/
	}

	void prepare() override {
		//param rpm = controls[0] / 7000.f;// max(0.075, controls[0] / 20000.f);
		//engine.set(rpm);
	}

	void process() override {
		noise * 0.1 >> out;
	}
};