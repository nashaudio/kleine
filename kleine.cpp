
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
	Engine engine;		// load and initialize the audio engine

	// helper lambdas for convenience
	auto wait = [&](int ms) {								 engine.wait(ms); };
	auto midi = [&](byte status, byte data1, byte data2) {	 engine.midiIn(status, data1, data2); };
	auto noteOn = [&](byte note, byte velocity) {			 midi(0x90, note, velocity); };
	auto noteOff = [&](byte note, byte velocity = 0) {		 midi(0x80, note, velocity); };

	// create and configure the synth
	Mono::Synth* synth = new DX7();					// monophonic DX7 synth
	synth->controls.set(0);							// = Tubular Bells preset

	// create and configure the effect
	Stereo::Effect* effect = new PingPong();		// stereo (ping-pong) delay
	effect->controls.set(0.167, 0.75, 0.333, 0.25); // = 167ms delay time, 75% feedback, 333ms delay time, 25% feedback

	// attach plugins and start processing
    engine.attach(synth);
    engine.attach(effect);
	engine.start();

	// play some notes
	int notes[] = { 62, 64, 60, 48, 55 };
	for(int note : notes) {
		noteOn(note, 127);
		wait(500);
		// noteOff(note);
	}
	wait(1000);
	
	// stop all notes and the engine
	engine.allNotesOff();
	engine.stop();

	// clean up
	delete synth;
	delete effect;

    return 0;
}