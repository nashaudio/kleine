// Compare optimised code for explicit values and unit-literal construction.
#include "literals.h"
using namespace klang::literals;

extern "C" float numeric() { return 440.f; }
extern "C" float constructed() { return klang::Frequency(440.f); }
extern "C" float integerLiteral() { return 440_Hz; }
extern "C" float floatingLiteral() { return 440.0_Hz; }
extern "C" float scaledLiteral() { return .44_kHz; }
