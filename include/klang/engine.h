#ifndef HAS_KLANG
#include <klang.h>
#endif

#include "audio.h" // MiniAudio library for realtime + file I/O

namespace klang {

    struct Processor {
        enum Config : byte {
            None = 0,
            MonoEffect = 1,
            StereoEffect = 2,
            MonoSynth = 4,
            StereoSynth = 8,
            MonoSound = 16,
            StereoSound = 32,
            IsEffect = MonoEffect | StereoEffect,
            IsSynth = MonoSynth | StereoSynth,
            IsSound = MonoSound | StereoSound,
            IsStereo = StereoEffect | StereoSynth | StereoSound
        };
        int config = None;

        Effect* effect = nullptr;
        Synth* synth = nullptr;
        Sound* sound = nullptr;
        Stereo::Synth* synth2 = nullptr;
        Stereo::Sound* sound2 = nullptr;
        Stereo::Effect* effect2 = nullptr;

        virtual ~Processor() {
            delete effect;
			delete synth;
            delete sound;
            delete synth2;
            delete sound2;
			delete effect2;
        }

        variable::buffer left, right;
		struct Input {
		 	variable::buffer left, right;
		} input;

        bool attach(Plugin* plugin) {

            auto attach = [this](auto*& member, auto* candidate, Config config) -> bool {
                if (candidate) { member = candidate; this->config |= config; return true; } else return false;
                };

            if (attach(synth2, dynamic_cast<Stereo::Synth*>(plugin), StereoSynth))    return true;
            if (attach(sound2, dynamic_cast<Stereo::Sound*>(plugin), StereoSound))    return true;
            if (attach(effect2, dynamic_cast<Stereo::Effect*>(plugin), StereoEffect))   return true;
            if (attach(synth, dynamic_cast<Mono::Synth*>(plugin), MonoSynth))      return true;
            if (attach(sound, dynamic_cast<Mono::Sound*>(plugin), MonoSound))      return true;
            if (attach(effect, dynamic_cast<Mono::Effect*>(plugin), MonoEffect))     return true;
            return false;
        }

        bool isPlaying() const {
            if (hasSynth()) {
                if (config & StereoSynth) {
                    if (synth2->playing())
                        return true;
                } else {
                    if(synth->playing())
                       return true;
                }
            }

			constexpr float db60 = 0.001f; // -60dB threshold for "silence"
            if (isStereo())
				return left.rms() > db60 || right.rms() > db60;
            else
				return left.rms() > db60;
		}

        bool isMono() const { return !(config & IsStereo); }
        bool isStereo() const { return config & IsStereo; }
        bool hasSynth() const { return config & IsSynth; }
        bool hasSound() const { return config & IsSound; }
        bool hasEffect() const { return config & IsEffect; }

        template<typename... BYTE>
        void midiIn(BYTE... bytes) {
            static_assert(sizeof...(bytes) >= 1, "MIDI channel/system message must have 1-3 bytes");
            static_assert((std::is_integral_v<BYTE> && ...), "All MIDI bytes must be integral");

            const int nbBytes = sizeof...(bytes);
            const byte data[] = { static_cast<byte>(bytes)... };

            if (hasSynth()) {
                if (nbBytes == 3) {
                    switch (byte status = data[0] & 0xF0) {
                    case 0x90: // Note On
                        if (data[2] > 0) { // Note On
                            if (config & StereoSynth)
                                synth2->noteOn(data[1], data[2] / 127.f);
                            else
                                synth->noteOn(data[1], data[2] / 127.f);
                            return;
                        } // fall through to Note Off if velocity is 0
                    case 0x80: // Note Off
                        if (config & StereoSynth)
                            synth2->noteOff(data[1], data[2] / 127.f);
                        else
                            synth->noteOff(data[1], data[2] / 127.f);
                        return;
                    case 0xB0: // Control Change
                        if (config & StereoSynth)
                            synth2->onControl(data[0], data[1]);
                        else
                            synth->onControl(data[0], data[1]);
                        return;
					default: // Other MIDI message
                        if (config & StereoSynth)
                            synth2->onMIDI(data[0], data[1], data[2]);
                        else
                            synth->onMIDI(data[0], data[1], data[2]);
                        return;
                    }
                }
                
            }
        }

        void stop() {
            if (hasSynth()) {
                if (config & StereoSynth)
                    synth2->allNotesOff();
                else
                    synth->allNotesOff();
            }
        }

        void process(int frames) {

            left.resize(frames);
            right.resize(frames);

            left.clear();
            right.clear();

			for(int i = 0; i < frames; ++i) {
			 	if(input.left.finished() || input.right.finished())
			 		break;
			 	left[i] = input.left++;
			 	right[i] = input.right++;
			}

			float* stereo[2] = { left.data(), right.data() };

            // Process synth / sound as the source
            if (hasSynth()) {
                if (config & StereoSynth)
                    synth2->process(stereo, left.size);
                else
                    synth->process(left.data(), left.size);
            } else if (hasSound()) {
                if (config & StereoSound)
                    sound2->process(stereo::buffer(left, right));
                else
                    sound->process(left);
            }

            // Apply effect to the input / synth's output
            if (hasEffect()) {
                if (config & StereoEffect)
                    effect2->process(stereo::buffer(left, right));
                else
                    effect->process(left);
            }

            // If mono, copy left channel to right
            if (isMono())
                right = left;
        }

        static void callback(ma_device* device, void* output, const void* /*pInput*/, ma_uint32 frames) {
            float* pOutputF32 = (float*)output;

            Processor* processor = static_cast<Processor*>(device->pUserData);
            if (!processor)
                return;

            processor->process(frames);

            // Interleave left and right channels into the output buffer
            for (ma_uint32 frame = 0; frame < frames; ++frame) {
                pOutputF32[frame * 2] = processor->left[frame] * 0.25f;
                pOutputF32[frame * 2 + 1] = processor->right[frame] * 0.25f;
            }
        }
    };

    struct Engine {
        ma_device_config config;
        ma_device device;

        Processor processor;
        int error = 0;

		std::vector<int> history; // for storing MIDI events in simulated mode
		enum Mode { LIVE, SIMULATED } mode = LIVE;

		struct File : buffer {
			File() = default;
			File(const std::string& path) { load(path); }
			virtual ~File() { close(); }

			std::string path;
			ma_decoder_config config;
			ma_uint64 frames = 0;
			void* data = nullptr;

			variable::buffer left, right;

			bool load(const std::string& path) {
				if (data)
					close();
				config = ma_decoder_config_init(ma_format_f32, 0, 0);
				frames = 0;
				data = nullptr;
				ma_result result = ma_decode_file(path.c_str(), &config, &frames, &data);
				if(result != MA_SUCCESS) {
					printf("Failed to load audio file: %s (Error %d: %s)\n", path.c_str(), result, ma_result_description(result));
					return false;
				} else {
					left.resize(frames);
					if(config.channels == 2)
						right.resize(frames);
					// Copy interleaved audio data to left and right buffers
					for(int i = 0; i < frames; i++) {
						left[i] = ((float*)data)[i * config.channels];
						if(config.channels > 1)
							right[i] = ((float*)data)[i * config.channels + 1];
					}	
				}
				return result == MA_SUCCESS;
			}

			operator stereo::buffer() {
				return { left, config.channels == 1 ? left : right };
			}

			void close(){
				ma_free(data, nullptr);
				data = nullptr;
				frames = 0;
			}
		};

		// buffer input;
 		File output;

		Engine& operator<<(File& file) {
			processor.input.left = file.left;
			processor.input.right = file.config.channels == 1 ? file.left : file.right;
			return *this;
		}

		Engine& operator>>(File& file) {
			output = file;
			return *this;
		}

        Engine(Mode mode = LIVE) : mode(mode) {
            if (mode == LIVE) {
                config = ma_device_config_init(ma_device_type_playback);
                config.playback.format = ma_format_f32;
                config.playback.channels = 2; // stereo output
                config.dataCallback = Processor::callback;
                config.pUserData = &processor;

                if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
                    printf("Failed to open playback device.\n");
                    error = -3;
                    return;
                }
            }

            klang::fs = device.sampleRate;
        }

        virtual ~Engine() {
            // stop all notes and the engine
	        allNotesOff();
            if (mode == LIVE) {
                stop();
                ma_device_uninit(&device);
            }
        }

        bool start() {
            if (mode == SIMULATED) {
                history.clear();
                return true;
            }

            if (ma_device_start(&device) != MA_SUCCESS) {
                printf("Failed to start playback device.\n");
                ma_device_uninit(&device);
                error = -4;
                return false;
            }
            return true;
        }

        void wait(int ms) {
            if (mode == SIMULATED){
				history.push_back(0xF0); // F0 flags start of "wait" call
                history.push_back(ms);   // encode in millseconds
                history.push_back(0xF7); // F7 flags end of "wait" call
                return;
            }
			if (ms == 0) { // wait until all audio has finished playing
                while (processor.isPlaying())
                    wait(100);
			} else { // wait for specified time
                std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            }
		}

        void stop(bool now = false) {
            if (mode == LIVE) {
                if (!now)
                    wait(0);
                ma_device_stop(&device);
            }
        }

        template<typename DERIVED, typename... ARGS>
		DERIVED* attach(ARGS&&... args) {
			static_assert(std::is_base_of_v<Plugin, DERIVED>);
			DERIVED* p = new DERIVED();
			attach(p);
			p->controls.set(std::forward<ARGS>(args)...);
            return p;
		}

        void attach(Plugin* plugin) {
            if(mode == LIVE)
                processor.attach(plugin);
        }

        template<typename... BYTE>
        void midiIn(BYTE ...bytes) {
			if (mode == LIVE)
                processor.midiIn(bytes...);
            else
                (history.push_back(bytes), ...);
        }

        void noteOn(int note, int velocity, int channel = 0) {
            midiIn(0x90 | channel, note, velocity);
		}

        void noteOff(int note, int velocity = 0, int channel = 0) {
            midiIn(0x80 | channel, note, velocity);
        }

        void allNotesOff() {
            if(mode == LIVE)
                midiIn(0xB0, 0x7B, 0); // Control Change: All Notes Off
            else
				history.insert(history.end(), { 0xB0, 0x7B, 0 });
            processor.stop();
        }
    };

	inline Engine& operator<<(Engine::File& file, Engine& engine) {
		return engine >> file;
	}

	inline Engine& operator>>(Engine::File& file, Engine& engine) {
		return engine << file;
	}

}; // namespace klang