#pragma once

// Deterministic, device-free bounces through Kleine's existing Processor.
#include "../kleine.h"
#include "klang/Artificial Sounds/Phone Tones/phonetones.k"
#include "klang/Artificial Sounds/Phone Tones/phoneeffects.k"
#include "klang/Artificial Sounds/Pedestrians/pedestrians.k"
#include "klang/Artificial Sounds/DTMF Tones/dtmftones.k"
#include "klang/Artificial Sounds/Alarm Generator/alarmgenerator.k"
#include "klang/Artificial Sounds/Police/police.k"
#include "klang/Idiophonics/Telephone Bell/telephonebell.k"
#include "../tests/pd/primitive.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace farnell {

// An isolated impulse through the bell casing for delay verification.
struct CasingTest : Sound {
    TelephoneBell::Casing casing;
    uint64_t frame = 0;
    void process() override { float(frame++ == uint64_t(float(fs) / 2)) >> casing >> out; }
};

// Component renders expose the police oscillator, horn and environment independently.
struct PoliceTest : Sound {
    Police::LogOsc oscillator;
    Police::Horn horn;
    Police::Environment environment;
    std::string name;
    int frame = 0;
    explicit PoliceTest(const std::string& value) : name(value) {
        oscillator.graph = name.find("graph") != name.npos;
        oscillator.triangle = name.find("triangle") != name.npos;
        oscillator.legacy = name.find("legacy") != name.npos;
        oscillator.inverted = name.find("inverted") != name.npos;
        oscillator.set(name.find("graph") != name.npos && name.find("compare") == name.npos ? 800 : 700);
    }
    void prepare() override { horn.set(); }
    void process() override {
        if (name == "police-environment") float(frame++ == 0) >> environment >> out;
        else if (name == "police-horn") { signal wave = oscillator; wave >> horn >> out; }
        else oscillator >> out;
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
        // env~ control messages are delivered after processing their source block.
        out = float(bits);
        if (decoder.detectors[0].updated) {
            bits = 0;
            for (int i = 0; i < (single ? 1 : 8); ++i) if (decoder.detectors[i].active) bits |= 1 << i;
        }
    }
};

inline int render(int argc, char** argv) {
    if (argc < 4 || std::string(argv[1]) != "--render") {
        printf("Usage: kleine --render MODEL OUTPUT.wav [seconds] [sample-rate] [gain] [events.tsv]\n"
               "Models: dial, dial-web, dial-line, busy, busy-archive, ringback, pulse, bell\n"
               "Artificial Sounds: pedestrians, dtmf, alarm01..alarm07, police, police-legacy\n"
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
        Police* police = nullptr;
        PhoneEffects* effects = nullptr;
        DecoderTest* decoderTest = nullptr;
        if (name.rfind("pd-", 0) == 0) {
            processor.attach(new validation::Primitive(name.substr(3)));
        } else if (name == "pedestrians") {
            pedestrian = new Pedestrians;
            processor.attach(pedestrian);
            pedestrian->start();
        } else if (name == "phone-effects" || name == "call-recogniser") {
            effects = new PhoneEffects(name == "call-recogniser");
            processor.attach(effects);
            dtmf = &effects->dialler;
        } else if (name == "dtmf-detector" || name == "dtmf-decoder") {
            decoderTest = new DecoderTest(name == "dtmf-detector");
            processor.attach(decoderTest);
            dtmf = &decoderTest->dialler;
        } else if (name == "dtmf") {
            dtmf = new DTMFTones;
            processor.attach(dtmf);
        } else if (name == "alarm-bank") {
            alarmBank = new AlarmGenerator::Bank;
            processor.attach(alarmBank);
        } else if (name.size() == 7 && name.rfind("alarm0", 0) == 0 && name[6] >= '1' && name[6] <= '7') {
            alarm = new AlarmGenerator(name[6] - '0');
            processor.attach(alarm);
            if (argc == 8 && alarm->study >= 6) alarm->set(380, 2, 0, 0, 0, 0, 0);
            if (alarm->study == 1 || alarm->study == 5) alarm->start();
        } else if (name == "police" || name == "police-legacy") {
            police = new Police;
            police->set(0.1f, name == "police-legacy");
            processor.attach(police);
        } else if (name.rfind("police-", 0) == 0) {
            processor.attach(new PoliceTest(name));
        } else if (name == "bell" || name == "bell-single" || name == "bell-dry" || name == "bell-single-dry") {
            bell = new TelephoneBell;
            if (name.find("dry") != std::string::npos) bell->casingMix = 0;
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
            phone = new PhoneTones;
            processor.attach(phone);
            phone->set(tone, name == "dial-web" || name == "dial-line" ? 450 : 440, name != "busy-archive");
            if (name == "ringback-bulk") phone->lineVariant = 1;
        }

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
        } else if (dtmf) {
            for (int i = 0; i < 16; ++i)
                events.push_back({uint64_t((int((0.25 + i * 0.3) * rate) / 64) * 64), 0, "key", {float(i)}});
        } else if (alarm && alarm->study >= 6) {
            events.push_back({uint64_t(rate / 4 / 64 * 64), 0, "trigger", {}});
        }

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
                    if (effects && e.message == "ringer" && v.size() == 1) effects->ring(int(v[0]));
                    else if (effects && e.message == "source" && v.size() == 1) effects->source = int(v[0]);
                    else if (alarmBank && e.message == "bank" && v.size() == 8)
                        alarmBank->set(v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]);
                    else if (dtmf && e.message == "key" && v.size() == 1 && v[0] >= 0 && v[0] < 16)
                        dtmf->dial(DTMFTones::keys[int(v[0])]);
                    else if (dtmf && e.message == "dtmf" && v.size() == 3) dtmf->set(v[0], v[1], v[2]);
                    else if (dtmf && e.message == "tones" && v.size() == 2) dtmf->tones(v[0], v[1]);
                    else if (dtmf && e.message == "gate" && v.size() == 1) dtmf->gate(v[0]);
                    else if (alarm && e.message == "program" && v.size() == 7) {
                        alarm->set(v[0], v[1], v[2], v[3], v[4], v[5], v[6]); alarm->trigger();
                    } else if (alarm && e.message == "trigger" && v.empty()) alarm->trigger();
                    else if (alarm && e.message == "gain" && v.size() == 1) alarm->outputGain = v[0];
                    else if (police && e.message == "rate" && v.size() == 1) police->rate = v[0];
                    else if (pedestrian && e.message == "start" && v.size() == 1) pedestrian->start(v[0] != 0);
                    else if (alarm && e.message == "start" && v.size() == 1) alarm->start(v[0] != 0);
                    else throw std::runtime_error("unsupported event for model: " + e.message);
                } else if (phone) phone->dial(e.action);
                else if (bell && e.action == 2) bell->strike();
                else if (bell) bell->ring(e.action != 0);
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
