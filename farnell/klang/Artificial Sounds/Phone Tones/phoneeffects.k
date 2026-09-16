#pragma once

// Andy Farnell's phone-effects and call-recogniser supplements.
// Attribution and licensing: ../../../../README.md.
#include "phonetones.k"
#include "../DTMF Tones/dtmftones.k"
#include "../Alarm Generator/alarmgenerator.k"

namespace farnell {
using namespace klang::optimised;

/** @brief Combined dialling, number recognition, ringback and personalised alarms.
 * @code
 * auto* phone = new farnell::PhoneEffects;
 * processor.attach(phone);
 * phone->dialler.dial('1');
 * phone->ring(0); // Tom; 1 is Dick, 2 is Harry.
 * @endcode
 * Constructor true selects the standalone call-recogniser reference. Number
 * matching reconstructs missing upstream list abstractions; see the series notes.
 */
struct PhoneEffects : Sound {
    PhoneTones::Line line;
    DTMFTones dialler;
    DTMFTones::DigitDecoder decoder;
    AlarmGenerator alarm;
    pd::osc ringing[2], dial[2];
    pd::metro metro;
    pd::lop smoothing;
    DTMFTones::Number tom, dick, harry;
    int frame = 0, source = 0, nextRinger = -1, currentRinger = -1;
    int count = 0;
    signal gate = 0;
    bool recogniserOnly = false;

    explicit PhoneEffects(bool recogniser = false)
        : tom(recogniser ? "07814832212" : "0100200300"),
          dick(recogniser ? "07787123456" : "0987654321"),
          harry(recogniser ? "07890123456" : "1234567890"), recogniserOnly(recogniser) {
        line.set(1);
        metro = 1;
        alarm.set(380, 2, 0, 0, 0, 0, 0);
        alarm.outputGain = recogniser ? 0.8f : 0.5f;
    }
    void prepare() override {
        smoothing.set(100);
        dialler.prepare();
        decoder.prepare();
        alarm.prepare();
    }
    void programme(param duration, param cycles, param a, param b, param c, param d, param tone) {
        alarm.set(duration, cycles, a, b, c, d, tone);
        alarm.trigger();
    }
    void ring(int person) {
        currentRinger = person;
        if (person == 0) programme(317,3,349,0,0,0,1);
        else if (person == 1) programme(recogniserOnly ? 200 : 125,recogniserOnly ? 30 : 20,1000,476.2f,158.7f,0,1);
        else programme(1428,3,619,571,365,206,1);
        const int milliseconds = person == 0 ? 500 : person == 1 ? (recogniserOnly ? 200 : 300) : 1050;
        nextRinger = frame + int(std::ceil(double(float(fs)) * milliseconds / 1000));
    }
    void digit(char key) {
        if (key < '0' || key > '9') return; // These supplemental recognisers wire digits only.
        if (tom.digit(key,frame)) {
            source = 2;
            ring(0);
        }
        if (dick.digit(key,frame)) {
            source = 2;
            ring(1);
        }
        if (harry.digit(key,frame)) {
            source = 2;
            ring(2);
        }
    }
    void process() override {
        signal keypad = dialler >> decoder;
        for (int key : decoder.messages) digit(char('0' + key));
        if (frame == int(std::ceil(float(fs) * .01)) && !recogniserOnly) source = 1;
        if (frame == nextRinger) {
            // Both of Tom's delayed messages are at 500 ms in the supplied patch;
            // the second is immediately replaced by the first, following trigger order.
            if (currentRinger == 0) {
                programme(317,7,300,125,0,0,1);
                programme(238,1,317.46f,0,0,476,0);
            } else if (currentRinger == 1) programme(634,61,1000,476.19f,158.73f,0,1);
            else programme(1031,9,360,238,174,158,1);
            nextRinger = -1;
        }
        signal ringtone = alarm;
        if (recogniserOnly) ringtone >> out;
        else {
            signal ringPair = ringing[0](480) + ringing[1](440);
            if (metro(1000)) gate = count++ % 6 > 2;
            signal cadence = gate >> smoothing;
            signal dialPair = dial[0](350) + dial[1](450);
            signal wire = (source == 1 ? dialPair : source == 2 ? ringPair * cadence * .3f : signal(0));
            (wire >> line) + keypad * .12f + ringtone >> out;
        }
        ++frame;
    }
};
}
