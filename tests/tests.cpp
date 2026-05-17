#include <iostream>
#include <string>

// =========================================================================
// 1. hello - "Hello, World!"
// -------------------------------------------------------------------------
// Implement a function that prints a greeting to the console
//
// (see assignment brief for marking criteria / additional marks)

void hello () {
	// [Add your code here]
}



// ==========================================================================
// 2. beats - "Beats by Dr. Nash"
// --------------------------------------------------------------------------
// Implement a function that converts beats at a given BPM into milliseconds.
// 
// (see assignment brief for marking criteria / additional marks)

void beats() {
    // [Add your code here]
}



// ==========================================================================
// 3. loop - "Amen, Brother"
// --------------------------------------------------------------------------
// Use iteration to print and repeat the following drum pattern
//
//              | p . x . P . x . x . p . P . x . |
//
// (p - bass drum; P - snare drum; x - hi hat; . - rest; | - bar line):
//
// (see assignment brief for marking criteria / additional marks)

void loop() {
    // [Add your code here]
}



// ==========================================================================
// 4. tree - "Roots Manoeuvre"
// --------------------------------------------------------------------------
// Implement a recursive function that searches a provided tree structure for given values.
//
// (see assignment brief for marking criteria / additional marks)

#include "tree.h"

// Recursive search function to find a value in the tree and tag all instances of it
bool Node::Find(const char* v) const {
    // [Add your code here]
	return false;
}

// Function used to test your Node::Find function
// (NB: do not edit this!)
void tree() {
	// Build a test tree of nodes 
    // (a chord builder of notes)
    const Tree tree;
    
    //       C (root)
    //     / | \
    //    Eb E  F
    //   /  / \  \
    //  G  G   A  A
    //     |
    //     B

	// Run searches
	tree.Find("C");  // should find the root node C and print its value and path
	tree.Find("Eb"); // should find a single instance of Eb and print its value and path
    tree.Find("B");  // should find a single instance of B and print its value and path
	tree.Find("F#"); // should fail to find F# and print "F# not found"
	tree.Find("G");  // should find instances of G and print their values and paths
}



// ==========================================================================
// 5. transpose - "Uplifting Melody"
// --------------------------------------------------------------------------
// Given a list of notes, transpose the notes by a given amount of semitones.
//
// (see assignment brief for marking criteria / additional marks)

// Transpose a list of MIDI note numbers by a given number of semitones
void transpose(std::vector<int>& notes, int semitones) {
    // [Add your code here]
}

// Function used to test your transpose(...) function
// (NB: do not edit this!)
void transpose() {
	std::cout << "Input: ";

	// Generate a random list of MIDI note numbers (between 60 and 71)
    std::vector<int> notes;
    for (int n = 0; n < 12; n++) {
        notes.push_back(60 + rand() % 12);
		std::cout << notes.back() << " ";
    }

	// Random transpose amount between 0 and 11 semitones
    const int semitones = rand() % 12;
	std::cout << "(+" << semitones << " semitones)\nOutput: ";

	// Transpose the notes
    transpose(notes, semitones);
}



// ==========================================================================
// 6. counterpoint - "Music in the Key of C++"
// --------------------------------------------------------------------------
// Implement a function that constructs a short musical sequence using a
// std::vector<std::string>, given a provided std::array<int> scale.
// 
// (see assignment brief for marking criteria / additional marks)

// Generate a melody from the provided scale
void counterpoint(const std::array<std::string, 15>& scale, std::vector<std::string>& melody) {
    // [Add your code here]
}

// Function used to test your counterpoint(...) function
// (NB: do not edit this!)
void counterpoint() {
    // Scale for testing
    const std::array<std::string, 15> f_major = { 
        "F4", "G4", "A4", "Bb4", "C5", "D5", "E5", 
        "F5", "G5", "A5", "Bb5", "C6", "D6", "E6", "F7" 
    };

	// Print the provided scale
    std::cout << ansi::grey << "Scale: ";
    for (const std::string& note : f_major)
		std::cout << note << " ";
	std::cout << "\n" << ansi::reset;

	// Generate a melody using the scale
    std::vector<std::string> melody;
    counterpoint(f_major, melody);

	// Print the generated melody 
    for (const std::string& note : melody)
        std::cout << note << " ";
	std::cout << "\n";
}



// =========================================================================
// 7. play - "The Sound of C"
// -------------------------------------------------------------------------
// Write a short program that plays a defined note phrase using 'noteOn', 'wait', and 'noteOff'.
//
// Note phrases are provided using the following struct:
// 
//  struct Track {
//	    std::vector<int> pitch;  // MIDI note numbers (0 = rest)
//	    std::vector<int> length; // note lengths (in 100ms ticks)
//  };
// 
// (see assignment brief for marking criteria / additional marks)

#include "DX7.k"
#include "SuperSaw.k"
#include "PingPong.k"


// Play the provided track(s) using the provided engine
void play(Engine& engine, const Track& melody, const Track& bass) {
	// [Add your code here]
}

// Function used to test your play(...) function
void play() {
    Engine engine;
    engine.start();
    engine.attach<DX7>(1);
    engine.attach<PingPong>(0.167, 0.75, 0.222, 0.25);

    const Track melody = {
	    { 81,76,83,76,79,81,76,84,76,86,76,83,84,76,81,76,83,76,79,81,76,84,76,86,76,83,84,76,83,76 },   
	    {  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 } }; 
    const Track bass = {
        {  45, 0, 45, 0, 45, 0, 48, 50, 52, 50, 55, 43, 0, 43, 0, 55, 50, 48, 47, 43, 35, 0 },
        {   2, 2,  6, 2,  4, 2,  2,  2,  2,  2,  2,  2, 2,  6, 2,  8,  4,  2,  2,  2,  1, 1 } };

	play(engine, melody, bass);

    engine.wait(1000);
    engine.stop();
}



// =========================================================================
// 8. file - "Beverly Hills 902 I/O"
// -------------------------------------------------------------------------
// Load a pitch sequence from disk and play it with the correct durations.
// 
// (see assignment brief for marking criteria / additional marks)

const char* input_path = "../../tests/input.dat";
const char* output_path = "../../tests/output.dat";

// Load the pitches from file into the array and play using the provided engine
void file(Engine& engine, std::vector<int>& notes, const std::vector<int>& durations) {
	// [Add your code here]
}

// Function used to test your file(...) function
// (NB: do not edit this!)
void file() {
    Engine engine;
    engine.start();
    engine.attach<SuperSaw>(0, 0, 0.05);
    engine.attach<PingPong>(0.167, 0.75, 0.222, 0.25);

	std::vector<int> notes;
	const std::vector<int> durations = { 4, 3, 2, 1, 2, 2, 2, 4, 3, 2, 1, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 2, 10 }; // in ticks (e.g. 120ms)

    file(engine, notes, durations);

	engine.wait(1000);
	engine.stop();
}



// =========================================================================
// 9. object - "Objection Noted"
// -------------------------------------------------------------------------
// Create a simple musical event type, MyNote, with fields pitch, velocity, and duration.
// 
// (see assignment brief for marking criteria / additional marks)

// [Declate/define your object here]



// =========================================================================
// 10. sequence - "Private Notes"
// -------------------------------------------------------------------------
// Implement an object, MySequence, that stores notes (as simple ints)
// internally, in a vector called, notes, and exposes a minimal public
// interface, with at(), add(), size() and clear() member function.
//
// (see assignment brief for marking criteria / additional marks)

// [Declate/define your object here]



// =========================================================================
// 11. point - "Point Counterpoint"
// -------------------------------------------------------------------------
// Use a pointer to iterate through an array, printing each value.
// 
// (see assignment brief for marking criteria / additional marks)

#include "unknown.h"

// Function to RLE decode a short value to a string
// (RLE encoding: lower byte = char, upper byte = count)
static inline std::string decode(short data) {
	return std::string(data & 0xFF, data >> 8);
}

// Use the provided pointer to iterate through the array and print its values
void point(const short*& ptr, size_t size) {
	// [Add your code here]
}

// Function used to test your point(...) function
void point() {
    const short* ptr = unknown.data(); 
    point(ptr, unknown.size());
}