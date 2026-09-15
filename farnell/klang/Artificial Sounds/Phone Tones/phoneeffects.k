#pragma once

// Andy Farnell's bulk ALARMS phone-effects / dt-call-recogniser supplements.
// Attribution and licensing: ../../../../README.md.
#include "phonetones.k"
#include "../DTMF Tones/dtmftones.k"
#include "../Alarm Generator/alarmgenerator.k"

namespace farnell {
using namespace klang::optimised;

// Combined dial tone, keypad, number recognition, ringback and three personalised ringers.
struct PhoneEffects : Sound {
    PhoneTones::Line line;
    DTMFTones dialler;
    DTMFTones::Decoder decoder;
    AlarmGenerator alarm;
    pd::osc ringing[2], dial[2];
    pd::lop smoothing;
    DTMFTones::Number tom, dick, harry;
    int frame = 0, source = 0, nextRinger = -1, currentRinger = -1;
    int pendingKey = -1;
    bool recogniserOnly = false;

    explicit PhoneEffects(bool recogniser = false)
        : tom(recogniser ? "07814832212" : "0100200300"),
          dick(recogniser ? "07787123456" : "0987654321"),
          harry(recogniser ? "07890123456" : "1234567890"), recogniserOnly(recogniser) {
        line.set(1); dialler.set(200, .25f, 1);
        alarm.set(380, 2, 0, 0, 0, 0, 0); alarm.outputGain = recogniser ? 0.8f : 0.5f;
    }
    void prepare() override {
        ringing[0].set(480); ringing[1].set(440);
        dial[0].set(350); dial[1].set(450); smoothing.set(100);
        dialler.prepare(); decoder.prepare(); alarm.prepare();
    }
    void programme(param duration, param cycles, param a, param b, param c, param d, param tone) {
        alarm.set(duration, cycles, a, b, c, d, tone);
        // prepare belongs to the next processing buffer; frequency changes here
        // are necessary only for the internal sequencer's block-boundary events.
        for (int i = 0; i < 4; ++i) alarm.voice[i].set(alarm.frequencies[i], alarm.spectrum);
        alarm.trigger();
    }
    void ring(int person) {
        currentRinger = person;
        if (person == 0) programme(317,3,349,0,0,0,1);
        else if (person == 1) programme(recogniserOnly ? 200 : 125,recogniserOnly ? 30 : 20,1000,476.2f,158.7f,0,1);
        else programme(1428,3,619,571,365,206,1);
        const int milliseconds = person == 0 ? 500 : person == 1 ? (recogniserOnly ? 200 : 300) : 1050;
        nextRinger = (frame + int(float(fs) * milliseconds / 1000)) / 64 * 64;
    }
    void digit(char key) {
        if (key < '0' || key > '9') return; // These supplemental recognisers wire digits only.
        if (tom.digit(key,frame)) { source = 2; ring(0); }
        if (dick.digit(key,frame)) { source = 2; ring(1); }
        if (harry.digit(key,frame)) { source = 2; ring(2); }
    }
    void process() override {
        if (frame % 64 == 0 && pendingKey >= 0) {
            digit(DTMFTones::keys[pendingKey]); pendingKey = -1;
        }
        if (frame == int(float(fs) * .01f) / 64 * 64 && !recogniserOnly) source = 1;
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
        signal keypad = dialler;
        signal analysed = keypad >> decoder;
        if (decoder.changed && decoder.key >= 0) pendingKey = decoder.key;
        signal ringtone = alarm;
        if (recogniserOnly) ringtone >> out;
        else {
            signal ringPair = ringing[0] + ringing[1];
            const int tick = ((frame / 64 * 64 + 63) / int(float(fs))) % 6;
            signal cadence = float(tick > 2) >> smoothing;
            signal dialPair = dial[0] + dial[1];
            signal wire = (source == 1 ? dialPair : source == 2 ? ringPair * cadence * .3f : signal(0));
            signal handset = wire >> line;
            handset + keypad * .12f + ringtone >> out;
        }
        ++frame;
    }
};
}
