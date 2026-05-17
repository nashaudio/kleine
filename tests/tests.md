# Programming in C++ — Automated Tests

## Aim

The automated tests should remain small, concept-led, and complementary to the project. Each test should primarily target one C++ concept or language feature, while staying lightly framed in the module's musical/sequencing context.

## Basic Tests

### 1. hello - "Hello, World!"

**Concepts:** testing, cout / printf\
Implement a function that prints a greeting

- 1 mark for the word "hello"
- 1 mark for capitals (e.g. "Hello, World!")
- 1 mark for punctuation (comma, exclamation mark)
- 1 mark for supporting a user-entered name (e.g. "Hello, Kevin!")

### 2. beats - "Beats by Dr. Nash"

**Concepts:** arithmetic, parameters, numeric reasoning\
Implement a function that converts beats at a given BPM into milliseconds.

- 1 mark for correctly converting a single beat (e.g. a beat at 120bpm)
- 1 mark for user interaction (input handling, prompts)
- 1 mark for supporting multiple beats (e.g. 3 beats at 180bpm)

### 3. loop - "Amen, Brother"
Use iteration to print and repeat the following drum pattern

| p . x . P . x . x . p . P . x . |

(p - bass drum; P - snare drum; x - hi hat; . - rest; | - bar line):

- 1 mark for printing the pattern
- 1 mark for including a closing bar line
- 1 mark for repeating the pattern
- 1 mark if the final repeat ends with a bass drum (p) instead of a hihat (x)

### 4. tree - "Roots Manoeuvre"

**Concepts:** recursion, traversal, conditionals  
Implement a recursive function that searches a provided tree structure for given values.

- 1 mark for correctly tagging a given value if it appears at the root
- 1 mark for correctly tagging a given value in the root's immediate children
- 1 mark for correctly tagging a given value anywhere in the tree
- 1 mark for correctly returning false when a value is absent
- 1 mark for correctly tagging all instances of a given value in the tree (e.g. if a value appears multiple times)

## STL, Containers, and Files

### 5. transpose - "Uplifting Melody".

**Concepts:** iteration, arrays/vectors, value transformation\
Given a list of notes and a transpose amount, return the transposed notes.

- 1 mark for accurate transposition
- 1 mark for in-place processing of original array

### 6. compose - "The Key of C++"

**Concepts:** STL vectors, `push_back`, `size`, indexing\
Implement a function that constructs and returns a short musical sequence as a `std::vector<int>`, given a provided std::array\<int> scale.&#x20;

- 1 mark for notes within scale
- 1 mark if beginning and ending on the tonic
- 1 mark if no consecutive notes are repeated
- 1 mark if final note is approached in stepwise motion (from one scale step above/below)
- 1 mark if melodic leaps (two scale steps or higher) subsequently step back one scale step

### 7. play - "The Sound of C"

**Concepts:** loops, sequencing, engine API, ordered actions\
Write a short program that plays a defined note phrase using `noteOn`, `wait`, and `noteOff`.

- 1 mark for accurate melody pitch
- 1 mark for repeating the phrase more than once
- 1 mark for polyphony (simultaneous notes, e.g. melody and bass)
- 1 mark for accurate bass pitch 
- 1 mark for accurate bass timing

### 8. file - "Beverley Hills 902 I/O"

**Concepts:** file I/O, loops, vectors\
Read a simple list of integer note values from a text file into a `std::vector<int>`.

- 1 mark for loading the notes into the array
- 1 mark for playing the correct pitches
- 1 mark for playing the correct timing (durations)
- 1 mark for saving the pitches interleaved with their durations to a new file

## OOP - Object Oriented Programming

### 9. object - "Objection Noted" (WIP)

**Concepts:** classes/structs, members, constructors\
Create a simple musical event type, MyNote, with fields pitch, velocity, and duration.

- 1 mark for declaring the object.
- 1 mark for declaring public member variables.
- 1 mark if member variables are initialised.

### 10. sequence - "Private Notes"

**Concepts:** encapsulation, classes, vectors, methods\
Implement a small class that stores notes (e.g. as integers) internally, in a vector called "notes", and exposes a minimal public interface, with at(), add(), size() and clear() member function.

- 1 mark if the container is protected (inaccessible externally).
- 4 marks for the public functions (1 per function).\
    at(...) - returns the note value at a given index\
    add(...) - adds a note to the sequence\
    size() - returns the number of notes\
    clear() - resets (empties) the sequence
- 1 mark if all functions behave as expected
- 1 mark for overloading the subscript [] operator.

### 11. pointer - "Point Counterpoint" (draft)

**Concepts:** pointers, dereferencing, member access through pointers\
Use a pointer to iterate through an array, printing each value;

- 1 mark for iterating over the array.
- 1 mark for if the pointer ends after the array.

### 12. model - "Plug and Play"

**Concepts:** object-oriented programming, inheritance, polymorphism\
Declare a objects to model a basic audio plugin system, supporting both effects and synthesisers.

- 1 mark for an EffectPlugin object with functions, input() and output(), that print "audio" and "audio".
- 1 mark for a SynthPlugin object with functions, input() and output(), that print "midi" and "audio".
- 1 mark if EffectPlugin/SynthPlugin are derived from a common parent class called Plugin.
- 1 mark if SynthPlugin overrides input() to print "midi".
- 1 mark if EffectPlugin and SynthPlugin support polymorphism (through Plugin\*).

## Rationale

This set keeps the tests:

- small and legible,
- focused on specific learning concepts,
- aligned with the curriculum sequence,
- distinct from the integrative work of the project, but developing the skills for it

## Assessment Model

Each test is broken down into **small, explicit marking criteria** (up to 6 items per test). Marks are awarded per criterion, similar to sub-parts of an exam question. The total across all criteria sums to **50 marks**, so that the test component maps directly onto the module weighting (50%). A mark in the tests is therefore directly a mark in the module.

### Marking approach

- Each test is worth a **small number of marks** (typically 3–8), composed of simple pass/fail checks.
- Criteria are **clear, observable behaviours** (e.g. correct output, correct structure, correct use of a feature).
- Students accumulate marks across all criteria; there is **no threshold or cliff edge**.
- The final test mark (out of 50) is combined directly with the project (also 50) to form the module mark.

### Indicative weighting (provisional sums)

- **Basic Tests (16 marks total)**

  - 1 - hello [4 marks]
  - 2 - beat [3 marks]
  - 3 - loop [4 marks]
  - 4 - tree [5 marks]

- **STL, Containers, and Files (16 marks total)**

  - 4 - transpose [2 marks]&#x20;
  - 5 - compose [5 marks]
  - 6 - play [5 marks]
  - 7 - file [4 marks]

- **OOP - Object Oriented Programming (18 marks total)**

  - 8 - object [3 marks]
  - 9 - sequence [7 marks]
  - 10 - pointer [3 marks]
  - 11 - model [5 marks]

### Rationale

- The total of **50 marks** aligns directly with the module weighting, making grading transparent.
- Tests are decomposed into **small, checkable behaviours**, making marking granular and objective.
- Each test has entry-level (building to a pass) and more advanced elements (yielding higher marks).
- Students can attempt the test any time, as often as they like, and receive marks immediately.

## Coverage summary

- **Basic Tests:** Tests 1–4
- **STL and File I/O:** Tests 5–8
- **OOP, pointers, polymorphism:** Tests 9–12

