// Compile each macro separately: each selected expression must be rejected.
#include "literals.h"
#ifndef REJECT_NO_IMPORT
using namespace klang::literals;
#endif

#ifdef REJECT_OUTPUT_SCALE
// Existing Output::operator* requires an lvalue; unrelated to unit suffixes.
struct Source : klang::Generator {
    void process() override { out = 0; }
};
Source source;
klang::signal invalidScale = source(440) * 0.2f;
#endif

#ifdef REJECT_F_SUFFIX
auto invalid = 1.f_Hz; // f_Hz is one suffix, not f followed by _Hz.
#endif

#ifdef REJECT_NO_IMPORT
auto invisible = 1_Hz; // Merely including the header does not import suffixes.
#endif

#ifdef REJECT_MIXED_MAX
auto mismatched = std::max(klang::Frequency(220), 440);
#endif

#ifdef REJECT_TYPED_ROUTE
// A typed argument does not change the Output& returned by an inline call.
struct Source : klang::Generator {
    void process() override { out = 0; }
};
struct Destination : klang::Modifier {};
Source source;
Destination destination;
klang::signal ambiguous = source(440_Hz) >> destination;
#endif
