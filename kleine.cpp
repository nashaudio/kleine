
//
// kleine.cpp : eine kleine klangmusik
//
// This is a small example of using the klang library to play some notes on a DX7 synth with a PingPong effect.
// It demonstrates how to set up the audio engine, attach plugins, send MIDI messages, and manage timing.
//

#include "kleine.h"
#include "tests/tests.h"	// for the test function

int main(int argc, char** argv)
{
	//test::hello();
	//test::beats();
	//test::loop();
	//test::tree();
	//test::transpose();
	//test::counterpoint();
	//test::play();
	//test::file();
	//test::object();
	//test::sequence();
	//test::point();
	//test::model();
	test::all();

    return 0;
}