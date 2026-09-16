#pragma once

// Deterministic, device-free bounces through Kleine's existing Processor.
#include "../kleine.h"
#include "variants.h"
#include "../tests/pd/phone-tones-archive.h"
#include "../tests/pd/dtmf-archive.h"
#include "klang/Artificial Sounds/Phone Tones/phonetones.k"
#include "klang/Artificial Sounds/Phone Tones/phoneeffects.k"
#include "klang/Artificial Sounds/Pedestrians/pedestrians.k"
#include "klang/Artificial Sounds/DTMF Tones/dtmftones.k"
#include "klang/Artificial Sounds/Alarm Generator/alarmgenerator.k"
#include "klang/Artificial Sounds/Police/police.k"
#include "klang/Idiophonics/Telephone Bell/telephonebell.k"
#include "klang/Idiophonics/Telephone Bell/studies.k"
#include "klang/Idiophonics/Bouncing/bouncing.k"
#include "klang/Idiophonics/Rolling/rolling.k"
#include "klang/Idiophonics/Creaking/creaking.k"
#include "klang/Idiophonics/Boing/boing.k"
#include "../tests/pd/primitive.h"
#include "../tests/pd/metro.h"
#include "../tests/pd/line-control.h"
#include "../tests/pd/idiophonics.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace farnell {

// Listening diagnostic: expose the direct bell sum without adding a model control.
struct BellTest : TelephoneBell {
    bool dry;
    explicit BellTest(bool directOnly) : dry(directOnly) {}
    void process() override {
        TelephoneBell::process();
        if (dry) out = bell[0].out + bell[1].out + (hammer.out ^ 4) * noise.out * 0.1f;
    }
};

// An isolated impulse through the bell casing for delay verification.
struct CasingTest : Sound {
    TelephoneBell::Casing casing;
    uint64_t frame = 0;
    void process() override { float(frame++ == uint64_t(float(fs) / 2)) >> casing >> out; }
};

// Component renders expose the police oscillator, horn and environment independently.
struct PoliceTest : Sound {
    std::unique_ptr<Police::LogOsc> oscillator;
    Police::Horn horn;
    Police::Environment environment;
    std::string name;
    int frame = 0;
    explicit PoliceTest(const std::string& value) : name(value) {
        if (name.find("triangle") != name.npos) oscillator = std::make_unique<variants::TriangleOsc>();
        else if (name.find("inverted") != name.npos) oscillator = std::make_unique<variants::InvertedExponentialOsc>();
        else if (name.find("graph") != name.npos) {
            if (name.find("legacy") != name.npos) oscillator = std::make_unique<variants::SymmetricExponentialOsc>();
            else oscillator = std::make_unique<variants::PowerOsc>();
        } else if (name.find("legacy") != name.npos) oscillator = std::make_unique<variants::ExponentialOsc>();
        else oscillator = std::make_unique<Police::LogOsc>();
        oscillator->set(name.find("graph") != name.npos && name.find("compare") == name.npos ? 800 : 700);
    }
    void prepare() override { horn.set(); }
    void process() override {
        if (name == "police-environment") float(frame++ == 0) >> environment >> out;
        else if (name == "police-horn") *oscillator >> horn >> out;
        else *oscillator >> out;
    }
};

// Detector diagnostics retain the original audio input and expose control states as samples.
struct DecoderTest : Sound {
    DTMFTones dialler;
    DTMFTones::Decoder decoder;
    bool single = false;
    int bits = 0;
    explicit DecoderTest(bool one) : single(one) {}
    void prepare() override { dialler.prepare(); decoder.prepare(); }
    void process() override {
        signal wave = dialler;
        signal analysed = wave >> decoder;
        if (decoder.detectors[0].envelope.updated) {
            bits = 0;
            for (int i = 0; i < (single ? 1 : 8); ++i) if (decoder.detectors[i].active) bits |= 1 << i;
        }
        out = float(bits);
    }
};

inline int render(int argc, char** argv) {
    if (argc < 4 || std::string(argv[1]) != "--render") {
        printf("Usage: kleine --render MODEL OUTPUT.wav [seconds] [sample-rate] [gain] [events.tsv]\n"
               "Models: dial, dial-web, dial-line, busy, busy-archive, ringback, pulse, bell\n"
               "Artificial Sounds: pedestrians, dtmf, alarm01..alarm07, police, police-legacy\n"
               "Idiophonics: bouncing, rolling, tincan, uneven2, creaking, boing\n"
               "Variants: bouncing-bulk, bouncing-legacy, boing-bulk, boing-legacy, uneven\n"
               "Bell studies: bell-a0..bell-a4, bell-partial, bell-group, bell-testgroup\n"
               "Diagnostics: bell-single, bell-dry, bell-single-dry, bell-casing, pd-<primitive>\n"
               "Defaults: 10 seconds, 48000 Hz, unity gain. Output: mono float32 WAV.\n");
        return argc == 2 && std::string(argv[1]) == "--help" ? 0 : 2;
    }
    try {
        if (argc > 8) throw std::runtime_error("too many arguments");
        const std::string name = argv[2];
        const double duration = argc > 4 ? std::stod(argv[4]) : 10;
        const int rate = argc > 5 ? std::stoi(argv[5]) : 48000;
        const float gain = argc > 6 ? std::stof(argv[6]) : 1;
        if (!std::isfinite(duration) || duration <= 0 || duration > 600 ||
            rate < 22050 || rate > 192000 || !std::isfinite(gain) || gain < 0 || gain > 16)
            throw std::runtime_error("invalid duration, sample rate, or gain");
        fs = rate; // Construct DSP only after establishing the render rate.
        Processor processor;
        PhoneTones* phone = nullptr;
        TelephoneBell* bell = nullptr;
        Pedestrians* pedestrian = nullptr;
        DTMFTones* dtmf = nullptr;
        AlarmGenerator* alarm = nullptr;
        AlarmGenerator::Bank* alarmBank = nullptr;
        Sound* police = nullptr;
        PhoneEffects* effects = nullptr;
        DecoderTest* decoderTest = nullptr;
        Bouncing* bouncing = nullptr;
        Rolling* rolling = nullptr;
        Creaking* creaking = nullptr;
        Boing* boing = nullptr;
        Rolling::Study* uneven = nullptr;
        validation::BellStage* bellStudy = nullptr;
        BellStudies::Ratios* bellRatios = nullptr;
        BellStudies::Telephone* bellTelephone = nullptr;
        if (name.rfind("pd-metro", 0) == 0) {
            processor.attach(new validation::Metro(name.substr(3)));
        } else if (name.rfind("pd-ramp", 0) == 0) {
            processor.attach(new validation::ControlLine(name.substr(3)));
        } else if (name.rfind("pd-", 0) == 0) {
            processor.attach(new validation::Primitive(name.substr(3)));
        } else if (name == "bell-a3") {
            bellRatios = new BellStudies::Ratios; processor.attach(bellRatios);
        } else if (name == "bell-a4") {
            bellTelephone = new BellStudies::Telephone; bellTelephone->noise.seed(404933); processor.attach(bellTelephone);
        } else if (name == "bell-a0" || name == "bell-a1" || name == "bell-a2" || name == "bell-partial" || name == "bell-group" || name == "bell-testgroup") {
            bellStudy = new validation::BellStage(name.substr(5)); processor.attach(bellStudy);
        } else if (name.rfind("bouncing", 0) == 0) {
            if (name == "bouncing-bulk") bouncing = new variants::BouncingExponential;
            else if (name == "bouncing-legacy") bouncing = new variants::BouncingPower;
            else bouncing = name == "bouncing-envelope" || name == "bouncing-height" ?
                new validation::BouncingStage(name == "bouncing-envelope") : new Bouncing;
            processor.attach(bouncing);
        } else if (name == "rolling" || name == "tincan") {
            rolling = new Rolling;
            rolling->set(name == "tincan"); rolling->ground.noise.seed(404933);
            processor.attach(rolling);
        } else if (name == "uneven2") {
            auto ground = new Rolling::Uneven; ground->noise.seed(404933); processor.attach(ground);
        } else if (name == "uneven") {
            uneven = new Rolling::Study; processor.attach(uneven);
        } else if (name == "creaking" || name == "creaking-pulse") {
            creaking = name == "creaking" ? new Creaking : new validation::CreakingStage;
            creaking->stickslip.random.seed(1); processor.attach(creaking);
        } else if (name.rfind("creaking-", 0) == 0) {
            processor.attach(new validation::CreakingImpulse(name.substr(9)));
        } else if (name.rfind("boing", 0) == 0) {
            if (name == "boing-bulk") boing = new variants::BoingExponential;
            else if (name == "boing-legacy") boing = new variants::BoingPower;
            else boing = name == "boing" ? new Boing : new validation::BoingStage(name.substr(6));
            boing->noise.seed(404933); processor.attach(boing);
        } else if (name == "pedestrians") {
            pedestrian = new Pedestrians;
            processor.attach(pedestrian);
        } else if (name == "phone-effects" || name == "call-recogniser") {
            effects = new PhoneEffects(name == "call-recogniser");
            processor.attach(effects);
            dtmf = &effects->dialler;
        } else if (name == "dtmf-detector" || name == "dtmf-decoder") {
            decoderTest = new DecoderTest(name == "dtmf-detector");
            processor.attach(decoderTest);
            dtmf = &decoderTest->dialler;
        } else if (name == "dtmf" || name == "dtmf-bulk" || name == "dtmf-study") {
            if (name == "dtmf-bulk") dtmf = new validation::DTMFLevel03;
            else if (name == "dtmf-study") dtmf = new variants::DTMFUnfiltered;
            else dtmf = new DTMFTones;
            processor.attach(dtmf);
        } else if (name == "alarm-bank") {
            alarmBank = new AlarmGenerator::Bank;
            processor.attach(alarmBank);
        } else if (name.size() == 7 && name.rfind("alarm0", 0) == 0 && name[6] >= '1' && name[6] <= '7') {
            alarm = new AlarmGenerator(name[6] - '0');
            processor.attach(alarm);
            if (argc == 8 && alarm->study >= 6) alarm->set(380, 2, 0, 0, 0, 0, 0);
        } else if (name == "police" || name == "police-legacy") {
            if (name == "police-legacy") police = new variants::PoliceExponential;
            else police = new Police;
            processor.attach(police);
        } else if (name.rfind("police-", 0) == 0) {
            processor.attach(new PoliceTest(name));
        } else if (name == "bell" || name == "bell-single" || name == "bell-dry" || name == "bell-single-dry") {
            bell = new BellTest(name.find("dry") != std::string::npos);
            processor.attach(bell);
        } else if (name == "bell-casing") {
            processor.attach(new CasingTest);
        } else {
            PhoneTones::Tone tone;
            if (name == "dial" || name == "dial-web") tone = PhoneTones::Dial;
            else if (name == "dial-line") tone = PhoneTones::DialLine;
            else if (name == "busy" || name == "busy-archive") tone = PhoneTones::Busy;
            else if (name == "ringback" || name == "ringback-bulk") tone = PhoneTones::Ringback;
            else if (name == "pulse") tone = PhoneTones::Pulse;
            else throw std::runtime_error("unknown model");
            if (name == "dial") phone = new validation::Dial440;
            else if (name == "busy-archive") phone = new validation::BusyUnsmoothed;
            else if (name == "ringback-bulk") phone = new validation::RingbackHandset;
            else phone = new PhoneTones(tone);
            processor.attach(phone);
        }

        // Reference events sit a quarter-sample inside a PD block; vline retains
        // their fractional timestamps. Ordinary model events use its sample clock.
        auto referenceOffset = [&](double offset) {
            auto timestamp = [&](pd::vline& line) {
                line.messageTime(line.time() + offset * 1000 / rate);
            };
            if (rolling) {
                timestamp(rolling->push);
                timestamp(rolling->impact);
            }
            if (uneven) uneven->messageOffset = offset;
            if (boing) {
                timestamp(boing->pitch);
                timestamp(boing->clamped.decay);
            }
            if (bellStudy) for (auto& e : bellStudy->envelope) timestamp(e);
            if (bellRatios) bellRatios->messageOffset = offset;
        };

        // A scheduled renderer control change at an absolute audio frame.
        struct Event { uint64_t frame; int action = 0; std::string message; std::vector<float> values; };
        std::vector<Event> events;
        auto event = [&](double seconds, int action) {
            uint64_t frame = uint64_t(std::llround(seconds * rate));
            if (name == "pulse") frame = frame / 64 * 64; // Pd sig~ updates at a block boundary.
            events.push_back({frame, action});
        };
        if (name == "pulse") { event(0.25, 1); event(0.75, 5); event(1.75, 7); }
        if (name == "bell" || name == "bell-dry") { event(0.5, 1); event(2.5, 0); event(4, 1); event(6, 0); }
        if (name == "bell-single" || name == "bell-single-dry") event(0.5, 2);
        if (argc == 8) {
            events.clear();
            std::ifstream input(argv[7]);
            if (!input) throw std::runtime_error("cannot read event script");
            std::string row;
            while (std::getline(input, row)) {
                if (row.empty() || row[0] == '#') continue;
                std::istringstream fields(row);
                Event item;
                fields.operator>>(item.frame);
                std::operator>>(fields, item.message);
                if (!fields || item.frame % 64)
                    throw std::runtime_error("event script requires a frame on the 64-sample grid and an action");
                float value;
                while (fields.operator>>(value)) {
                    if (!std::isfinite(value)) throw std::runtime_error("nonfinite event value");
                    item.values.push_back(value);
                }
                if (!fields.eof()) throw std::runtime_error("invalid event value");
                if (!events.empty() && item.frame < events.back().frame)
                    throw std::runtime_error("event script must be chronological");
                events.push_back(item);
            }
        } else if (bouncing || rolling || boing || uneven || bellStudy || bellTelephone) {
            events.push_back({uint64_t(rate / 4 / 64 * 64), 0, "trigger", {}});
        } else if (bellRatios) {
            events.push_back({uint64_t(rate / 4 / 64 * 64), 0, "ring", {1}});
            events.push_back({uint64_t(rate * 3 / 64 * 64), 0, "ring", {0}});
        } else if (creaking) {
            // Moving force is essential to this demonstration's creaking gesture.
            // Destinations arrive one 100 ms source-patch slew ahead of these points.
            static constexpr double points[][2] = {
#include "../tests/pd/creaking-force.inc"
            };
            int point = 0;
            for (int step = 0; step < 66; ++step) {
                const double time = (step + 1) / 10.0;
                while (point < 37 && points[point + 1][0] < time) ++point;
                const double fraction = (time-points[point][0])/(points[point+1][0]-points[point][0]);
                const float force = float(std::round((points[point][1] +
                    fraction*(points[point+1][1]-points[point][1])) * 1e6) / 1e6);
                events.push_back({uint64_t(std::llround(step / 10.0 * rate)) / 64 * 64, 0, "force", {force}});
            }
        } else if (dtmf) {
            for (int i = 0; i < 16; ++i)
                events.push_back({uint64_t((int((0.25 + i * 0.3) * rate) / 64) * 64), 0, "key", {float(i)}});
        } else if (alarm && alarm->study >= 6) {
            events.push_back({uint64_t(rate / 4 / 64 * 64), 0, "trigger", {}});
        }

        // An explicit initial on/off event owns startup; avoid a hidden first tick.
        if (pedestrian && (events.empty() || events.front().frame != 0 || events.front().message != "start"))
            pedestrian->set(1);
        if (alarm && (alarm->study == 1 || alarm->study == 5) &&
            (events.empty() || events.front().frame != 0 || events.front().message != "start"))
            alarm->set(1);
        const uint64_t frames = uint64_t(std::llround(duration * rate));
        std::vector<float> audio(frames);
        size_t next = 0;
        uint64_t position = 0;
        double wall = 0;
        while (position < frames) {
            while (next < events.size() && events[next].frame == position) {
                const auto& e = events[next];
                const auto& v = e.values;
                if (!e.message.empty()) {
                    // PD parses delay milliseconds as t_float before scheduling.
                    referenceOffset(double(float((double(e.frame) + .25) * 1000 / rate)) * rate / 1000 - double(e.frame));
                    if (bellStudy && e.message == "trigger" && v.empty()) bellStudy->trigger();
                    else if (bellTelephone && e.message == "trigger" && v.empty()) bellTelephone->trigger();
                    else if (bellRatios && e.message == "ring" && v.size() == 1) bellRatios->set(v[0]);
                    else if (uneven && e.message == "trigger" && v.empty()) uneven->trigger();
                    else if (uneven && e.message == "stop" && v.empty()) uneven->stop();
                    else if (bouncing && e.message == "trigger" && v.empty()) bouncing->trigger();
                    else if (rolling && e.message == "trigger" && v.empty()) rolling->trigger();
                    else if (boing && e.message == "trigger" && v.empty()) boing->trigger();
                    else if (boing && e.message == "boing" && v.size() == 2) boing->set(v[0], v[1]);
                    else if (creaking && e.message == "force" && v.size() == 1) creaking->set(v[0]);
                    else if (effects && e.message == "ringer" && v.size() == 1) effects->ring(int(v[0]));
                    else if (effects && e.message == "source" && v.size() == 1) effects->source = int(v[0]);
                    else if (alarmBank && e.message == "bank" && v.size() == 8)
                        alarmBank->set(v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]);
                    else if (dtmf && e.message == "key" && v.size() == 1 && v[0] >= 0 && v[0] < 16)
                        dtmf->dial(DTMFTones::keys[int(v[0])]);
                    else if (dtmf && e.message == "dtmf" && v.size() == 3) {
                        // Retained recipe metadata must match the selected model file.
                        if (v[0] != 200) throw std::runtime_error("DTMF reference duration is fixed at 200 ms");
                        const float level = name == "dtmf-study" ? .125f : name == "dtmf-bulk" ? .3f : .25f;
                        if (v[1] != level || v[2] != float(name != "dtmf-study"))
                            throw std::runtime_error("DTMF recipe does not match selected model");
                    }
                    else if (dtmf && e.message == "tones" && v.size() == 2) dtmf->tones(v[0], v[1]);
                    else if (dtmf && e.message == "gate" && v.size() == 1) dtmf->gate(v[0]);
                    else if (alarm && e.message == "program" && v.size() == 7) {
                        alarm->set(v[0], v[1], v[2], v[3], v[4], v[5], v[6]); alarm->trigger();
                    } else if (alarm && e.message == "trigger" && v.empty()) alarm->trigger();
                    else if (alarm && e.message == "gain" && v.size() == 1) alarm->outputGain = v[0];
                    else if (police && e.message == "rate" && v.size() == 1) (*police)(v[0]);
                    else if (pedestrian && e.message == "start" && v.size() == 1) pedestrian->set(v[0]);
                    else if (alarm && e.message == "start" && v.size() == 1) alarm->set(v[0]);
                    else throw std::runtime_error("unsupported event for model: " + e.message);
                } else if (phone) phone->dial(e.action);
                else if (bell && e.action == 2) bell->strike();
                else if (bell) bell->set(e.action != 0);
                ++next;
            }
            uint64_t count = std::min<uint64_t>(64, frames - position);
            if (next < events.size() && events[next].frame > position)
                count = std::min(count, events[next].frame - position);
            const auto start = std::chrono::steady_clock::now();
            processor.process(int(count));
            wall += std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            for (uint64_t i = 0; i < count; ++i) {
                audio[position + i] = processor.left[int(i)] * gain;
                if (!std::isfinite(audio[position + i])) throw std::runtime_error("nonfinite audio sample");
            }
            position += count;
        }

        const std::filesystem::path output = argv[3];
        if (output.has_parent_path()) std::filesystem::create_directories(output.parent_path());
        ma_encoder encoder{};
        const auto config = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, 1, rate);
        if (ma_encoder_init_file(output.string().c_str(), &config, &encoder) != MA_SUCCESS)
            throw std::runtime_error("cannot open output file");
        ma_uint64 written = 0;
        const auto result = ma_encoder_write_pcm_frames(&encoder, audio.data(), frames, &written);
        ma_encoder_uninit(&encoder);
        if (result != MA_SUCCESS || written != frames) throw std::runtime_error("incomplete audio write");
        // Processor wall time excludes construction, sample copy and WAV I/O.
        // The Python harness records whole-process CPU/memory separately.
        printf("{\"model\":\"%s\",\"sample_rate\":%d,\"frames\":%llu,\"gain\":%.6g,"
               "\"processing_wall_seconds\":%.6f}\n",
               name.c_str(), rate, static_cast<unsigned long long>(frames), gain, wall);
        return 0;
    } catch (const std::exception& error) {
        fprintf(stderr, "Kleine render: %s\n", error.what());
        return 1;
    }
}
}
