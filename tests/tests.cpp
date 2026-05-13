#include <iostream>
#include <string>

// =========================================================================
// 1. hello - "Hello, World!"
// -------------------------------------------------------------------------
// Implement a function that prints a greeting to the console
//
// - 1 mark for the word "hello"
// - 1 mark for capitals (e.g. "Hello, World!")
// - 1 mark for punctuation (comma, exclamation mark)
// - 1 mark for supporting a user-entered name (e.g. "Hello, Kevin!")

void hello () {
	// [Add your code here]
}



// ==========================================================================
// 2. beats - "Beats by Dr. Nash"
// --------------------------------------------------------------------------
// Implement a function that converts beats at a given BPM into milliseconds.
// 
// - 1 mark for correctly converting a single beat (e.g. a beat at 120bpm)
// - 1 mark for user interaction (input handling, prompts)
// - 1 mark for supporting multiple beats (e.g. 3 beats at 180bpm)

void beats() {
    // [Add your code here]
}



// ==========================================================================
// 3. loop - "Amen, Brother"
// --------------------------------------------------------------------------
// Use iteration to print and repeat the following drum pattern
//
// | p . x . P . x . x . p . P . x . |
//
// (p - bass drum; P - snare drum; x - hi hat; . - rest; | - bar line):
//
// - 1 mark for printing the pattern
// - 1 mark for including a closing bar line
// - 1 mark for repeating the pattern
// - 1 mark if the final repeat ends with a bass drum (p) instead of a hihat (x)

void loop() {
    // [Add your code here]
}



// ==========================================================================
// 4. tree - "Roots Manoeuvre"
// --------------------------------------------------------------------------
// Implement a recursive function that searches a provided tree structure for given values.
//
// - 1 mark for correctly tagging a given value if it appears at the root
// - 1 mark for correctly tagging a given value in the root's immediate children
// - 1 mark for correctly tagging a given value anywhere in the tree
// - 1 mark for correctly returning false when a value is absent
// - 1 mark for correctly tagging all instances of a given value in the tree (e.g. if a value appears multiple times)

#include "tree.h"

// Recursive search function to find a value in the tree and tag all instances of it
bool Node::Find(const char* v) const {
    // [Add your code here]
	return false;
}

// Function used to test your Node::Find function
// (NB: do not need to edit this!)
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

