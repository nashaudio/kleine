
//
// kleine.cpp : eine kleine klangmusik
//
// This is a small example of using the klang library to play some notes on a DX7 synth with a PingPong effect.
// It demonstrates how to set up the audio engine, attach plugins, send MIDI messages, and manage timing.
//

#include "kleine.h"

#include "DX7.k"		// klang synth plugin emulating the Yamaha DX7
#include "PingPong.k"	// klang effect plugin implementing a stereo (ping-pong) delay

int main(int argc, char** argv)
{
	// create and configure the audio, synth, effect
	Engine engine;
	engine.start();
	engine.attach<DX7>(0);
    engine.attach<PingPong>(0.167, 0.75, 0.333, 0.25);

	// play some notes
	int notes[] = { 62, 64, 60, 48, 55 };
	for(int note : notes) {
		engine.noteOn(note, 127);
		engine.wait(500);
		engine.noteOff(note);
	}

	// wait for notes to finish, stop the engine
	engine.wait(1000);
	engine.stop();
	
    return 0;
}
