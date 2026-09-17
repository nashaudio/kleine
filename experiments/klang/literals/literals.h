// Proposed optional Klang unit literals; core types and operators are unchanged.
#pragma once
#include <klang.h>

namespace klang::literals {

// Hertz: integer and floating spellings both construct the existing Frequency.
// using namespace klang::literals;
// auto frequency = 440_Hz; // Frequency, also usable as param or signal.
inline Frequency operator""_Hz(unsigned long long value) {
    return Frequency(static_cast<float>(value));
}
inline Frequency operator""_Hz(long double value) {
    return Frequency(static_cast<float>(value));
}

// Kilohertz: convert to hertz before narrowing to Klang's float precision.
// 0.2f * osc(2.5_kHz) >> out;
inline Frequency operator""_kHz(unsigned long long value) {
    return Frequency(static_cast<float>(value * 1000.0L));
}
inline Frequency operator""_kHz(long double value) {
    return Frequency(static_cast<float>(value * 1000.0L));
}

}
