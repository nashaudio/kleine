namespace ansi {
    const char* reset  = "\x1b[0m";
    const char* red    = "\x1b[31m";
    const char* green  = "\x1b[32m";
    const char* yellow = "\x1b[33m";
    const char* blue   = "\x1b[34m";
    const char* cyan   = "\x1b[36m";
    const char* bold   = "\x1b[1m";
    const char* grey   = "\x1b[90m";
    const char* white  = "\x1b[37m";
	const char* amber =  "\x1b[38;5;214m";

    const char* dark_red = "\x1b[2;31m";    
    const char* dark_green = "\x1b[2;32m";  
    const char* dark_yellow = "\x1b[2;33m";  
}

#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <fstream>
#include <algorithm>
#include <streambuf>
#include <regex>

#ifdef _WIN32
    #include <io.h>
    #define OAI_DUP     _dup
    #define OAI_DUP2    _dup2
    #define OAI_FILENO  _fileno
    #define OAI_CLOSE   _close
#else
    #include <unistd.h>
    #define OAI_DUP     dup
    #define OAI_DUP2    dup2
    #define OAI_FILENO  fileno
    #define OAI_CLOSE   close
#endif

struct Track {
	std::vector<int> pitch;  // MIDI note numbers (0 = rest)
	std::vector<int> length; // note lengths (in 100ms ticks)
};

#include "tests.cpp"

// Disable runtime checks and optimizations to detect uninitialized variables
#ifdef _MSC_VER
    #pragma runtime_checks("", off)
    #pragma optimize("", on)
    #pragma warning(disable:4700)
#elif defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wuninitialized"
    #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
    // GCC/Clang: Use function attribute instead of global pragma for optimization
#endif


namespace test {

    struct Mark {
        int marks = 0;
        int total = 0;

		Mark() = default;
		explicit Mark(int total) : total(total) {}

        Mark& operator+=(const Mark& other) {
            marks += other.marks;
            total += other.total;
            return *this;
        }

        Mark& operator++(int) {
            marks++;
            return *this;
		}

        double percent() const {
            return total > 0 ? (static_cast<double>(marks) / total) * 100.0 : 0.0;
        }
    };

    class StdioCapture {
    public:
        struct IO {
            std::string in, out, err;
            std::string console;

            int inputs() const {
                int linefeeds = 0;
                for (char c : in) {
                    if (c == '\n') {
                        linefeeds++;
                    }
                }
                return linefeeds;
            }

            void clear() {
                in.clear();
                out.clear();
                err.clear();
                console.clear();
            }
        };

        IO& io;

        StdioCapture(IO& io, std::string input = "")
            : input_text_(std::move(input))
            , stdin_buf_(input_text_, &io)
            , stdout_buf_(&io, false)
            , stderr_buf_(&io)
            , io(io)
        {
            io.clear();
            hook();
        }

        StdioCapture(const StdioCapture&) = delete;
        StdioCapture& operator=(const StdioCapture&) = delete;

        ~StdioCapture() {
            unhook();
            std::cout << io.console; // Output the captured console interactions (in and out)
        }

        std::string stdin_text() const {
            return io.in;
        }

        std::string stdout_text() const {
            std::cout.flush();
            return io.out;
        }

        std::string stderr_text() const {
            std::cerr.flush();
            return io.err;
        }

    private:

        // A stream buffer that provides input from a given string and captures what is read from it, while also forwarding to an optional source (e.g. original std::cin buffer) for additional input if needed.
        class CaptureInputBuf : public std::streambuf {
        public:
            CaptureInputBuf(std::string prefix, IO* io)
                : prefix_(std::move(prefix)), io_(io) {
            }

            // Set the source stream buffer to read from (e.g. original std::cin buffer)
            void set_source(std::streambuf* source) {
                source_ = source;
            }

        protected:
            // Called when the stream needs to read a character (e.g. std::cin.peek())
            int_type underflow() override {
                if (prefix_pos_ < prefix_.size()) {
                    const int_type c = traits_type::to_int_type(prefix_[prefix_pos_]);
                    if (io_ && c == 10) {
                        prefix_pos_++; // skip the newline in the prefix
                        io_->in.push_back('\n');
                        io_->console.push_back('\n');
                    }
                    return c;
                    //return traits_type::to_int_type(prefix_[prefix_pos_]);
                }

                if (!source_)
                    return traits_type::eof();

                return source_->sgetc();
            }

            // Called when a character is read from the stream (e.g. std::cin.get())
            int_type uflow() override {
                if (prefix_pos_ < prefix_.size()) {
                    const int_type c = traits_type::to_int_type(prefix_[prefix_pos_]);
                    ++prefix_pos_;
                    if (io_ && c != 10) {
                        io_->in.push_back(traits_type::to_char_type(c));
                        io_->console.append(ansi::grey);
                        io_->console.push_back(traits_type::to_char_type(c));
                        io_->console.append(ansi::reset);
                        if (prefix_pos_ < prefix_.size() && traits_type::to_int_type(prefix_[prefix_pos_]) == '\n') {
                            //++prefix_pos_; // skip the newline in the prefix
                            //io_->in.push_back(traits_type::to_char_type('\n'));
                            //io_->console.push_back(traits_type::to_char_type('\n'));
                            //return '\n';
                        }
                    }
                    return c;
                } else if (!prefix_.empty()) {
                    return '\n'; // simulate end of input with a newline after the prefix is fully read
                }

                if (!source_)
                    return traits_type::eof();

                const int_type c = source_->sbumpc();
                if (traits_type::eq_int_type(c, traits_type::eof())) {
                    //io_->in.push_back('\n'); // simulate end of input with a newline
                    //io_->console.push_back('\n');
                    return c;
                }

                if (io_) {
                    io_->in.push_back(traits_type::to_char_type(c));
                    io_->console.push_back(traits_type::to_char_type(c));
                }

                return c;
            }

            // Called when multiple characters are read from the stream (e.g. std::cin.read())
            std::streamsize xsgetn(char* s, std::streamsize count) override {
                if (!s || count <= 0) {
                    return 0;
                }

                std::streamsize read = 0;
                while (read < count) {
                    const int_type c = uflow();
                    if (traits_type::eq_int_type(c, traits_type::eof())) {
                        break;
                    }

                    s[read++] = traits_type::to_char_type(c);
                }

                return read;
            }

        private:
            std::string prefix_;
            IO* io_ = nullptr;
            std::streambuf* source_ = nullptr;
            std::size_t prefix_pos_ = 0;
        };

        // A stream buffer that captures output written to it and forwards it to an optional sink (e.g. original std::cout or std::cerr buffer).
        class TeeOutputBuf : public std::streambuf {
        public:
            // Constructor takes a pointer to the string where output will be captured.
            explicit TeeOutputBuf(IO* io, bool echo = true)
                : io_(io), echo_(echo) {
            }

            // Set the sink stream buffer to write to (e.g. original std::cout or std::cerr buffer)
            void set_sink(std::streambuf* sink) {
                sink_ = sink;
            }

        protected:
            // Called when a character is written to the stream. 
            int_type overflow(int_type ch) override {
                if (traits_type::eq_int_type(ch, traits_type::eof())) {
                    return traits_type::not_eof(ch);
                }

                const char c = traits_type::to_char_type(ch);
                if (io_) {
                    io_->out.push_back(c);
                    io_->console.push_back(c);
                }

                if (!echo_)
                    return ch; // pretend we wrote the character successfully, but don't actually write to the sink

                if (!sink_)
                    return traits_type::eof();

                return sink_->sputc(c);
            }

            // Called when multiple characters are written to the stream.
            std::streamsize xsputn(const char* s, std::streamsize count) override {
                if (io_ && count > 0) {
                    io_->out.append(s, static_cast<std::size_t>(count));
                    io_->console.append(s, static_cast<std::size_t>(count));
                }

                if (!echo_)
                    return count; // pretend we wrote everything successfully, but don't actually write to the sink

                if (!sink_ || count <= 0) {
                    return 0;
                }

                return sink_->sputn(s, count);
            }

            // Called when the stream is flushed (e.g. std::endl, std::flush).
            int sync() override {
                if (!sink_) {
                    return 0;
                }

                return sink_->pubsync();
            }

        private:
            IO* io_ = nullptr;
            bool echo_ = true;
            std::streambuf* sink_ = nullptr;
        };

        std::string input_text_;
        //std::string captured_stdin_;
        //std::string captured_stdout_;
        //std::string captured_stderr_;

        CaptureInputBuf stdin_buf_;
        TeeOutputBuf stdout_buf_;
        TeeOutputBuf stderr_buf_;

        std::streambuf* old_cin_buf_ = nullptr;
        std::streambuf* old_cout_buf_ = nullptr;
        std::streambuf* old_cerr_buf_ = nullptr;

        void hook() {
            old_cin_buf_ = std::cin.rdbuf();
            old_cout_buf_ = std::cout.rdbuf();
            old_cerr_buf_ = std::cerr.rdbuf();

            stdin_buf_.set_source(old_cin_buf_);
            stdout_buf_.set_sink(old_cout_buf_);
            stderr_buf_.set_sink(old_cerr_buf_);

            std::cin.rdbuf(&stdin_buf_);
            std::cout.rdbuf(&stdout_buf_);
            std::cerr.rdbuf(&stderr_buf_);
        }

        void unhook() noexcept {
            try {
                std::cout.flush();
                std::cerr.flush();

                if (old_cin_buf_) {
                    std::cin.rdbuf(old_cin_buf_);
                    old_cin_buf_ = nullptr;
                }
                if (old_cout_buf_) {
                    std::cout.rdbuf(old_cout_buf_);
                    old_cout_buf_ = nullptr;
                }
                if (old_cerr_buf_) {
                    std::cerr.rdbuf(old_cerr_buf_);
                    old_cerr_buf_ = nullptr;
                }
            }
            catch (...) {
            }
        }
    };

    // Helper function to read the entire contents of a file into a string
    static std::string read_file(const std::string& path) {
        std::ifstream in(path);
        if (!in) {
            return "";
        }

        return std::string(
            (std::istreambuf_iterator<char>(in)),
            std::istreambuf_iterator<char>()
        );
    }

    // Helper function to trim trailing newlines from a string (for cleaner test output comparisons)
    static std::string trim_trailing_newlines(std::string s) {
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) {
            s.pop_back();
        }
        return s;
    }

    // Helper function to convert a string to lowercase for case-insensitive comparisons
    static std::string lowercase(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }

	// Base class for a test case, which includes functionality to capture stdio and calculate marks based on the captured input and output.
    struct Test {
        virtual ~Test() = default;

        std::string name;
        std::string input;

        Mark operator()() { 
            StdioCapture::IO stdio;
            Mark m;

            std::cout << ansi::grey << "\n[Running test: " << name << "]\n\n" << ansi::white;
            {
                StdioCapture capture(stdio, input);
                run();
            }
            std::cout << "\n";
            m = mark(stdio);
            std::cout << ansi::grey << "\n[Test marks: " << m.marks << " / " << m.total << "]\n" << ansi::reset;
            return m;
        }

		virtual void run() = 0;
        virtual Mark mark(const StdioCapture::IO& io) = 0;
    };

#define PASS(comment) { marks++; std::cout << ansi::green << "[PASS] " << comment << "\n" << ansi::reset; }
#define FAIL(comment) std::cout << ansi::red << "[FAIL] " << comment << "\n" << ansi::reset;

#define MARK(condition, pass, fail) { if (condition) { PASS(pass); } else { FAIL(fail); } }
#define MARK_QUIET(condition) { if (condition) { marks++; } }
#define MARK_IF(condition) if(condition)

    enum { Pitch = 1, Duration = 2, Time = 4, All = Pitch|Duration|Time };

    struct Output {
        const int tick = 0;
		const int split = 0; // split point between melody and bass (in MIDI note number)

        struct Note {
            int pitch = 0, duration = 0, time = 0;
            int compare(const Note& other) const {
                return (pitch == other.pitch ? Pitch : 0)
                    | (duration == other.duration ? Duration : 0)
                    | (time == other.time ? Time : 0);
            }
        };
        struct Notes : std::vector<Note> {
            Notes& operator=(const ::Track in) {
                int time = 0;
                for (int e = 0; e < in.pitch.size(); e++) {
                    if (in.pitch[e]) // note on
                        push_back({ in.pitch[e], in.length[e], time });
                    time += in.length[e];
                }
                return *this;
            }

			template<typename T>
            bool matches(const T& array, int mode = Pitch) const {
                if (size() != array.size())
                    return false;
                
                if (mode & Pitch)
                    for (size_t n = 0; n < size(); n++)
                        if (at(n).pitch != array[n])
                            return false;
                
                if (mode & Duration)
                    for (size_t n = 0; n < size(); n++)
                        if (at(n).duration != array[n])
                            return false;
                
                if (mode & Time)
                    for (size_t n = 0; n < size(); n++)
                        if (at(n).time != array[n])
                            return false;
				
                return true;
			}

            // return a mask indicating what (if anything) was matched
            int find(const Notes& notes, size_t offset = 0) const {
                int match = All;
                for (int n = 0; n < notes.size() && match; n++) {
                    if (n + offset >= size())
                        return 0;
                    const Note& a = at(n + offset);
                    const Note& b = notes[n];
                    match &= a.compare(b);
                }
                return match;
            }

            std::string print(int offset) {
                int time = 0;
                std::string output;

                const int start = offset;
                const int end = start + 60;

                for (const Note& note : *this) {
                    if (note.time < start) {
                        time = note.time + note.duration; // skip notes that have already finished before the current time window
                        continue;
                    } else if (note.time >= end) {
                        break; // stop printing notes that start after the current time window
                    } else if (note.time == start) {
                        time = note.time; // start of time window, print notes starting at this time
                    }

                    if (note.time > time) {
                        int duration = note.time - time;
                        for (int d = 0; d < duration; d++)
                            output += "  "; // print dashes for note duration
                        time = note.time;
                    }

                    output += std::to_string(note.pitch);
                    for (int d = 0; d < note.duration - 1; d++)
                        output += "--"; // print dashes for note duration
                    time += note.duration;
                }

                return output += "\n";
            }
        } melody, bass;

        Output(int tick = 100, int split = 0) : tick(tick), split(split) { }
        Output(const std::vector<int>& midi, int split = 0) : tick(calculate_tick(midi)), split(split) { 
			operator=(midi);
        }

        static int calculate_tick(const std::vector<int>& midi) {
            std::vector<int> intervals;
            for(int m = 0; m < midi.size(); m += 3) {
                if (midi[m] == 0xF0)
                    intervals.push_back(midi[m + 1]);
			}

			// Find the greatest common divisor (GCD) of the intervals to determine the tick size
            auto gcd = [](int a, int b) {
                while (b != 0) {
                    int temp = b;
                    b = a % b;
                    a = temp;
                }
                return a;
            };
            int tick = intervals.empty() ? 100 : intervals[0];
            for (size_t i = 1; i < intervals.size(); i++) {
                tick = gcd(tick, intervals[i]);
            }
			return tick > 0 ? tick : 100; // default to 100ms if GCD is zero or negative
		}

        Output& operator=(const std::vector<int>& midi) {
            int time = 0;
            for (size_t m = 0; m < midi.size(); m += 3) {
                const int status = midi[m];
                const int data1 = midi[m + 1]; // pitch/wait

                Notes& notes = data1 >= split ? melody : bass;

                switch (status) {
                case 0x90: // note on
                    notes.push_back({ data1, 0, time });
                    break;
                case 0x80: // note off
                {
                    auto it = std::find_if(notes.rbegin(), notes.rend(), [data1](const Note& n) { return n.pitch == data1 && n.duration == 0; });
                    if (it != notes.rend())
                        it->duration = time - it->time; // update note duration
                }   break; // find note
                case 0xF0:
                    time += data1 / tick; // 100ms ticks
                    break;
                }
            }
            return *this;
        }
    };

	// Test: 1. hello - "Hello, World!"
    struct Hello : public Test {
        Hello() {
            name = "hello";
            input = "Kevin\n"; 
        }

        void run() override { ::hello(); }

		Mark mark(const StdioCapture::IO& io) override { 
            Mark marks(4);

            // 1 mark for the word "hello" in the output (case-insensitive)
            size_t hello_pos = lowercase(io.out).find("hello");
            MARK (hello_pos != std::string::npos, 
                "Output contains 'hello'.", 
                "Output does not contain 'hello'."); 

            // 1 mark for capitals - all words should start with capital letters
            // Extract all alphabetic words and check first character is uppercase
            std::regex word_pattern(R"([A-Za-z]+)");
            std::string greeting = io.out.substr(hello_pos == std::string::npos ? 0 : hello_pos); // Check capitalization in the part of the output starting from "hello"
            auto words_begin = std::sregex_iterator(greeting.begin(), greeting.end(), word_pattern);
            auto words_end = std::sregex_iterator();

            bool all_capitalized = true;
            int word_count = 0;

            // Iterate through all words and check if they start with an uppercase letter
            for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
                std::smatch match = *i;
                std::string word = match.str();
                word_count++;
                if (word.empty() || !std::isupper(word[0])) {
                    all_capitalized = false;
                    break;
                }
            }
            MARK (word_count > 0 && all_capitalized, 
                "Output contains correct capitalization.", 
                "Output does not contain correct capitalization.");

            // 1 mark for punctuation (comma, exclamation mark)
            std::regex punctuation_pattern(R"([A-Za-z0-9 ]+,\s*[A-Za-z0-9 ]+!)");
            MARK (std::regex_search(io.out, punctuation_pattern), 
                "Output contains correct punctuation.", 
                "Output does not contain correct punctuation.");

            // 1 mark for supporting a user-entered name (e.g. "Hello, Kevin!")
            std::string name = trim_trailing_newlines(io.in);
            MARK (io.inputs() != 0 && lowercase(io.out).find(lowercase(name)) != std::string::npos, 
                "Output contains user-entered name.", 
                "Output does not contain user-entered name.");

            return marks;
        }
	} hello;

	// Test: 2. beats - "Beats by Dr. Nash"
    struct Beats : public Test {
        Beats() {
            name = "beats";
            input = "160\n3\n"; 
        }
		void run() override { ::beats(); }
        Mark mark(const StdioCapture::IO& io) override {
            Mark marks(3);
            std::string output = lowercase(io.out);
            enum { NO_BEATS_PROMPT, BEATS_PROMPT, BEATS_CORRECT };
            int mode = io.inputs() == 2 ? BEATS_PROMPT : NO_BEATS_PROMPT;

            if (mode == BEATS_PROMPT) { // test multiple beats case
                // 1 mark for correctly converting multiple beats (e.g. three beats at 160bpm)
                MARK_IF (output.find("1125") != std::string::npos) {
                    mode = BEATS_CORRECT;
                    PASS ("Correctly converts beats to milliseconds.");
                } else {
                    FAIL ("Does not correctly convert beats to milliseconds.");
                }
            } else { // test single beat case
                // 1 mark for correctly converting a single beat (e.g. a beat at 160bpm)
                MARK (output.find("375") != std::string::npos, 
                    "Correctly converts beats to milliseconds.", 
                    "Does not correctly convert beats to milliseconds.");
            }

            // 1 mark for user interaction (input handling, prompts)
            bool has_prompt = output.find("bpm") != std::string::npos ||
                              output.find("beat") != std::string::npos ||
                              output.find("number") != std::string::npos ||
                              output.find("tempo") != std::string::npos ||
                              output.find("enter") != std::string::npos;
            MARK (has_prompt && io.inputs() != 0, 
                "Provides user interaction and prompts.", 
                "Missing user interaction or prompts.");

            // 1 mark for supporting multiple beats (e.g. 3 beats at 180bpm)
            MARK (mode == BEATS_CORRECT, 
                "Supports multiple beats.", 
                "Does not support multiple beats.");

            return marks;
        }
	} beats;

	// Test: 3. loop - "Amen, Brother"
    struct Loop : public Test {
        Loop() {
            name = "loop";
        }
        void run() override { ::loop(); }
        Mark mark(const StdioCapture::IO& io) {
            Mark marks(4);
            constexpr char PATTERN[] = "|p.x.P.x.x.p.P.x.";
            constexpr char PATTERN_NOEND[] = "|p.x.P.x.x.p.P.";
            constexpr int PATTERN_LENGTH = sizeof(PATTERN) - 1; // exclude null terminator

            auto ignored = [](char c) { return std::isspace(static_cast<unsigned char>(c)) || c == '\n' || c == '\r'; };

            // trim spaces
            std::string pattern = io.out;
            pattern.erase(std::remove_if(pattern.begin(), pattern.end(), ignored), pattern.end());

            // 1 mark for printing the pattern
            // Pattern: | p . x . P . x . x . p . P . x . (spacing/linefeed optional)
            size_t first_bar = pattern.find(PATTERN);
            MARK (first_bar != std::string::npos, 
                "Output contains drum pattern.", 
                "Output does not contain drum pattern.");

            // 1 mark for including a closing bar line
            size_t last_bar = pattern.rfind(PATTERN_NOEND);
            MARK (last_bar != std::string::npos 
             && (last_bar + PATTERN_LENGTH) < pattern.size()
             &&  pattern[last_bar + PATTERN_LENGTH] == '|', 
                "Output includes closing bar line.", 
                "Output does not include closing bar line.");

            // 1 mark for repeating the pattern 
            MARK (last_bar != first_bar, 
                "Pattern is repeated.", 
                "Pattern is not repeated.");

            // 1 mark if the repeat ends with a bass drum (p) instead of a hihat (x)
            // Look for the ending pattern before the final bar
            size_t closing_barline = last_bar + PATTERN_LENGTH - 2;
            MARK (closing_barline < pattern.length() && pattern[closing_barline] == 'p', 
                "Repeat ends with bass drum (p).", 
                "Repeat does not end with bass drum (p).");

            return marks;
        }
	} loop;

	// Test: 4. tree - "Roots Manoeuvre"
    struct Tree : public Test {
        Tree() {
            name = "tree";
        }
        void run() override { ::tree(); }
        Mark mark(const StdioCapture::IO& io) {
            Mark marks(5);
            std::string output = io.out;

            // 1 mark for correctly tagging a given value if it appears at the root
            MARK (output.find("{ C }") != std::string::npos, 
                "Correctly tags value at root.", 
                "Does not correctly tag value at root.");

            //// 1 mark for correctly tagging a given value in the root's immediate children
            MARK (output.find("{ C > Eb }") != std::string::npos, 
                "Correctly tags values in root's immediate children.", 
                "Does not correctly tag values in root's immediate children.");

            //// 1 mark for correctly tagging a given value anywhere in the tree
            MARK (output.find("{ C > E > G > B }") != std::string::npos, 
                "Correctly tags values anywhere in the tree.", 
                "Does not correctly tag values anywhere in the tree.");

            //// 1 mark for correctly printing "<value> not found" when a value is absent
            MARK (output.find("F# not found") != std::string::npos, 
                "Correctly prints \"not found\" when value is absent.", 
                "Does not correctly print \"not found\" when value is absent.");

            //// 1 mark for correctly tagging all instances of a given value in the tree (e.g. if a value appears multiple times)
            MARK (output.find("{ C > E > G }") != std::string::npos
               && output.find("{ C > Eb > G }") != std::string::npos, 
                "Correctly tags all instances of a value in the tree.", 
                "Does not correctly tag all instances of a value in the tree.");

            return marks;
        }
	} tree;

	// Test: 5. transpose - "Uplifting Melody"
    struct Transpose : public Test {
		int semitones = 0;
        std::vector<int> original;
		std::vector<int> notes;

        Transpose() {
            name = "transpose";
        }
        void run() override { 
            std::cout << ansi::grey << "Input:  ";

	        // Generate a random list of MIDI note numbers (between 60 and 71)
            original.clear();
            for (int n = 0; n < 12; n++) {
                original.push_back(60 + rand() % 12);
		        std::cout << original.back() << " ";
            }

            notes = original;

	        // Random transpose amount between 0 and 11 semitones
            semitones = rand() % 12;
	        std::cout << "(+" << semitones << " semitones)\nOutput: " << ansi::reset;

	        // Transpose the notes
            ::transpose(notes, semitones);
        }

        Mark mark(const StdioCapture::IO& io) {
            Mark marks(2);
            std::string output = io.out;

            // remove line beginning "Input: " if present
			size_t input_pos = output.find("Input:");
			if (input_pos != std::string::npos) {
				size_t end_of_line = output.find('\n', input_pos);
				if (end_of_line != std::string::npos)
					output.erase(input_pos, end_of_line - input_pos + 1);
				else
					output.erase(input_pos);
			}

            // 1 mark for printing the transposed note numbers
			bool all_notes_correct = true;
			size_t offset = 0;
            for (size_t i = 0; i < original.size(); i++) {
                int expected_note = original[i] + semitones;
                if((offset = output.find(std::to_string(expected_note), offset)) == std::string::npos) {
                    all_notes_correct = false;
                    break;
                } else {
					offset++; // move past the found note for the next search
                }
            }
            MARK (all_notes_correct, 
                "Output contains correct transposed note numbers.", 
                "Output does not contain correct transposed note numbers.");

            // 1 mark for in-place processing of original array
            MARK_IF (notes != original) {
                for(int i = 0; i < original.size(); i++) {
                    if(notes[i] != original[i] + semitones) {
                        FAIL ("Original array was modified but does not contain correct transposed values.");
                        return marks;
                    }
				}
				PASS ("Original array was modified (in-place processing).");
            } else {
                FAIL ("Original array was not modified (in-place processing not implemented).");
			}

            return marks;
        }
	} transpose;//*/

    // Test: 6. counterpoint - "Music in the Key of C++"
    struct Counterpoint : public Test {
        //typedef std::array<std::string, 15> Scale;

        struct Scale : public std::array<std::string, 15>
        {
            using std::array<std::string, 15>::array;
            using std::array<std::string, 15>::operator=;

            Scale(const std::array<std::string, 15>& scale) : std::array<std::string, 15>(scale) { };

            //const std::array<std::string, 15>:& get() const {
            //    return *this;
            //}

            int indexOf(const std::string& note) const {
			    auto it = std::find(begin(), end(), note);
			    int index = std::distance(begin(), it);
			    return it == end() ? -1 : index;
		    };

		    int intervalBetween(const std::string& from, const std::string& to) const {
			    int from_index = indexOf(from), to_index = indexOf(to);
			    if (from_index == -1 || to_index == -1) return 0;
			    return to_index - from_index; // returns positive for interval up, negative for interval down
		    };

            bool isTonic(const std::string& note) const {
			    return note == at(0) || note == at(7) || note == at(14); // any F
		    };

		    int isStepwise(const std::string& from, const std::string& to) const {
                int interval = intervalBetween(from, to);
                return interval == 1 || interval == -1 ? interval : 0; // returns 1 for step up, -1 for step down, else 0
            }

            bool isLeap(const std::string& from, const std::string& to) const {
                return !isRepeat(from, to) && !isStepwise(from, to);
		    }

		    bool isRepeat(const std::string& from, const std::string& to) const {
			    return from == to;
		    }
        } scale;
		std::vector<std::string> melody[100]; // run counterpoint multiple times to check for randomness / reproducibility

        struct Mark : public test::Mark {
            Mark() = default;
			explicit Mark(int total) : test::Mark(total) {}

            enum {
				SCALE = 1,      // all notes are drawn from the scale
				TONIC = 2,      // beginning and ending on the tonic
				REPEATS = 4,    // no consecutive notes are repeated
				STEPWISE = 8,   // final note is approached in stepwise motion (from one scale step above/below)
				LEAPS = 16,     // melodic leaps (two scale steps or higher) subsequently step back one scale step
            };
			int criteria = 0;

            bool operator==(int marks) const { return this->marks == marks; }
			bool operator!=(int marks) const { return this->marks != marks; }

            Mark& operator++(int) {
                marks++;
                return *this;
			}
        };

        void print(std::vector<std::string>& melody) {
            if(melody.empty())
                return;

            // Print the generated melody 
            if(scale.isTonic(melody.front())) {
                std::cout << ansi::green; // tonic in cyan
            } else {
                if (scale.indexOf(melody.front()) == -1)
                    std::cout << ansi::red; // non-tonic starting note in red
                else
                    std::cout << ansi::yellow; // in scale, but not tonic
			}
			std::cout << melody.front();

            
            for (int n = 1; n < melody.size(); n++) {
                const std::string& prev = melody[n - 1];
                const std::string& note = melody[n];

                int interval = scale.intervalBetween(prev, note);
                if(scale.isLeap(prev, note)) {
                    if (n == melody.size() - 1) {
                        std::cout << ansi::dark_red; // can't step back
                    } else {
                        const std::string& next = melody[n + 1];
                        int next_interval = scale.intervalBetween(note, next);
                        if((next_interval >= 0 != interval >= 0) && abs(next_interval) == 1) // 
                            std::cout << ansi::dark_green; // step back -> green
                        else
                            std::cout << ansi::dark_yellow; // leaps in yellow
                    }
                } else if(scale.isRepeat(prev, note)) {
                    std::cout << ansi::dark_red;    // repeats in red
                } else { // step wise
                    std::cout << ansi::dark_green;   // stepwise motion in green
				}

                
                std::cout << (interval >= 0 ? " +" : " ") << interval << ansi::reset;

                std::cout << ansi::green;
                if (n == melody.size() - 1 && !scale.isTonic(note))
                    std::cout << ansi::yellow;
                if (scale.indexOf(note) == -1)
                    std::cout << ansi::red;
                    
                std::cout << " " << melody[n] << ansi::reset;
			}
	        std::cout << "\n";
        }

        Counterpoint() {
            name = "counterpoint";
        }
        void run() override { 
            // Scale for testing
            scale = {
                "F4", "G4", "A4", "Bb4", "C5", "D5", "E5", 
                "F5", "G5", "A5", "Bb5", "C6", "D6", "E6", "F7" 
            };

	        // Print the provided scale
            std::cout << ansi::grey << "Scale:  ";
            for (const std::string& note : scale)
		        std::cout << note << " ";
	        std::cout << "\n\n" << ansi::reset;

	        // Generate a melody from the provided scale
            for (int i = 0; i < 100; i++) {
                ::counterpoint(scale, melody[i]);
                if (!melody[i].empty()) {
                    std::cout << ansi::grey << std::to_string(1001 + i).substr(1, 3) << ": " << ansi::reset; // print melody number (001, 002, ..., 100)
                    print(melody[i]);
                }
            }
        }

        static Mark mark(const Scale& scale, const std::vector<std::string>& melody) {
            Mark marks(5);

            if(melody.empty())
                return marks; // no marks

            // 1 mark for notes within scale
            bool all_notes_within_scale = true;
            for (const std::string& note : melody) {
                if (scale.indexOf(note) == -1) {
                    all_notes_within_scale = false;
                    break;
                }
            }
            MARK_IF (all_notes_within_scale)
				marks++.criteria |= Mark::SCALE;

            // 1 mark if beginning and ending on the tonic
            MARK_IF (!melody.empty() && scale.isTonic(melody.front()) && scale.isTonic(melody.back()))
				marks++.criteria |= Mark::TONIC;
             
			// 1 mark if no consecutive notes are repeated
            bool no_consecutive_repeats = true;
            for (size_t i = 1; i < melody.size(); i++) {
                if (scale.isRepeat(melody[i - 1], melody[i])) {
                    no_consecutive_repeats = false;
                    break;
                }
			}
            MARK_IF (no_consecutive_repeats)
				marks++.criteria |= Mark::REPEATS;

            // 1 mark if final note is always approached in stepwise motion (from one scale step above/below) 
            MARK_IF (!melody.empty() && scale.isStepwise(melody[melody.size() - 2], melody.back()))
				marks++.criteria |= Mark::STEPWISE;

            // 1 mark if melodic leaps (two scale steps or higher) subsequently step back one scale step
			bool leaps_step_back = true;
            for (size_t i = 1; i < melody.size(); i++) {
                const std::string& prev = melody[i - 1];
                const std::string& note = melody[i];

                int interval = scale.intervalBetween(prev, note);
                if(scale.isLeap(prev, note)) {
                    if (i == melody.size() - 1) {
                        leaps_step_back = false;
                    } else {
                        const std::string& next = melody[i + 1];
                        int next_interval = scale.intervalBetween(note, next);
                        if (!((next_interval >= 0 != interval >= 0) && abs(next_interval) == 1)) {
                            leaps_step_back = false;
                            break;
                        }
                    }
                }
			}
            MARK_IF (leaps_step_back)
				marks++.criteria |= Mark::LEAPS;

            return marks;
        }

        test::Mark mark(const StdioCapture::IO& io) override {
            if(melody[0].empty()) {
                FAIL ("No melody generated.");
                return Mark(5); // no marks
			}

            if(melody[0] == melody[1]) {
                FAIL ("Melody generation does not appear to be random (same melody generated twice).");
                return Mark(5); // no marks
            }

            Mark marks(5), run[100];
			marks.criteria = Mark::SCALE | Mark::TONIC | Mark::REPEATS | Mark::STEPWISE | Mark::LEAPS; // all criteria must be met in all generated melodies to earn marks
            for(int m = 0; m < 100; m++) {
                run[m] = mark(scale, melody[m]);
				marks.criteria &= run[m].criteria; // only award marks if criteria met in all generated melodies
			}

			MARK (marks.criteria & Mark::SCALE,
				"All notes are drawn from the scale.",
				"Not all notes are drawn from the scale.");

            MARK (marks.criteria & Mark::TONIC,
				"Melody begins and ends on the tonic.",
				"Melody does not begin and end on the tonic.");

            MARK (marks.criteria & Mark::REPEATS,
				"No consecutive notes are repeated.",
				"Consecutive notes are repeated.");

            MARK (marks.criteria & Mark::STEPWISE,
				"Final note is approached in stepwise motion.",
				"Final note is not approached in stepwise motion.");

            MARK (marks.criteria & Mark::LEAPS,
				"Melodic leaps are followed by a step back.",
				"Melodic leaps are not followed by a step back.");

            return marks;
        }
	} counterpoint;

    // Test: 7. play - "The Sound of C"
    struct Play : public Test {
        klang::Engine engine;

        const Track melody = {
	        { 81,76,83,76,79,81,76,84,76,86,76,83,84,76,81,76,83,76,79,81,76,84,76,86,76,83,84,76,83,76 },   
	        {  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 } }; 
        const Track bass = {
            {  45, 0, 45, 0, 45, 0, 48, 50, 52, 50, 55, 43, 0, 43, 0, 55, 50, 48, 47, 43, 35, 0 },
            {   2, 2,  6, 2,  4, 2,  2,  2,  2,  2,  2,  2, 2,  6, 2,  8,  4,  2,  2,  2,  1, 1 } };

        Play() : engine(klang::Engine::SIMULATED) {
            name = "play";
        }
        void run() override { 
            engine.start();

            ::play(engine, melody, bass); 

            Output output(100, 60);
            output = engine.history;

            int time = 0;
            const int length = max(output.melody.empty() ? 0 : output.melody.back().time + output.melody.back().duration, 
				                   output.bass.empty() ? 0 : output.bass.back().time + output.bass.back().duration);
            while(time < length) {
                std::cout << output.melody.print(time);
                std::cout << ansi::grey;
                if (!output.bass.empty())
                    std::cout << output.bass.print(time);
                std::cout << ansi::reset << std::endl;
                time += 60;
            }   

            engine.wait(1000);
            engine.stop();
        }
        Mark mark(const StdioCapture::IO& io) override {
            Mark marks(5);
            const std::vector<int>& midi = engine.history;

            if (midi.size() <= 3) {
                FAIL("No recorded MIDI activity.");
                return marks;
            }

            // original pitch sequences without note offs
            std::vector<int> melody_ons = melody.pitch, bass_ons;
            for (int p : bass.pitch)
                if(p) bass_ons.push_back(p);

            // output pitch sequences (note ons only)
            std::vector<int> melody_out_ons, bass_out_ons;
            for (int m = 0; m < midi.size(); m += 3) {
                int status = midi[m];
                if (status == 0x90) {
                    int pitch = midi[m + 1];
                    if (pitch > 60)
                        melody_out_ons.push_back(pitch);
                    else
                        bass_out_ons.push_back(pitch);
                }
            }

            Output original, output(100, 60);
            original.melody = melody;
            original.bass = bass;
            output = midi;

            // 1 mark for accurate melody (pitch, note lengths, and timing)
            int match = output.melody.find(original.melody);
            MARK_IF(match == All) {
                PASS("Accurate melody pitch and timing.");
            } else {
                std::string errors;
                if(!(match & Pitch)) errors += "pitch, ";
                if(!(match & Duration)) errors += "note lengths, ";
                if(!(match & Time)) errors += "timing, ";      
                errors.resize(errors.size() - 2); // remove trailing comma
                FAIL("Inaccurate melody (" + errors + ")");
            }

            // 1 mark for repeating the phrase more than once
			match = output.melody.find(original.melody, original.melody.size());
            MARK (match != 0, 
                "Melody is repeated more than once.", 
				"Melody is not repeated more than once.");
            
            // 1 mark for polyphony (simultaneous notes, i.e. melody and bass)
			bool polyphony = !output.melody.empty() && output.melody[0].time == 0 && 
                             !output.bass.empty() && output.bass[0].time == 0;
			MARK (polyphony,
                "Includes polyphony (simultaneous notes, i.e. melody and bass).", 
				"Does not include polyphony (simultaneous notes, i.e. melody and bass).");
            
            // 1 mark for accurate bass pitch 
			match = output.bass.find(original.bass);
            MARK(match & Pitch,
                "Accurate bass pitch.",
                "Inaccurate bass pitch.");

			// 1 mark for accurate bass timing
            MARK((match & (Time|Duration)) == (Time|Duration), 
				"Accurate bass timing.",
				"Inaccurate bass timing.");

            return marks;
		}
    } play;

    // Test: 8. file - "Beverly Hills 902 I/O"
    struct File : public Test {
        klang::Engine engine;

        std::vector<int> notes;
        const char* input_path = "../../tests/input.dat";
        const char* output_path = "../../tests/output.dat";

		const std::vector<int> pitches = {  65,68,65,65,70,65,63,65,72,65,65,73,72,68,65,72,77,65,63,63,60,67,65 };
        const std::vector<int> durations = { 4, 3, 2, 1, 2, 2, 2, 4, 3, 2, 1, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 2,10 };

        File() : engine(klang::Engine::SIMULATED) {
            name = "file";
        }
        void run() override { 
            engine.start();

            remove(output_path); // ensure output file is removed before running test

            ::file(engine, notes, durations); 

			Output output(engine.history);
            std::cout << output.melody.print(0);
        
            engine.stop();
        }
        Mark mark(const StdioCapture::IO& io) override {
            Mark marks(4);
			Output output(engine.history);

            // 1 mark for loading the notes into the array
            MARK(notes == pitches,
                "Notes correctly loaded into the array.",
				"Notes not correctly loaded into the array.");

            // 1 mark for playing the correct pitches
            MARK(output.melody.matches(pitches, Pitch),
                "Pitches played correctly .",
				"Pitches not played correctly.");
             
            // 1 mark for playing the correct timing (durations)
            MARK(output.melody.matches(durations, Duration),
                "Durations played correctly.",
				"Durations not played correctly.");
             
            // 1 mark for saving the pitches interleaved with their durations to a new file
			FILE* file = fopen(output_path, "r");
            if (file) {
                std::vector<int> file_data;
                int value;
                while (fscanf(file, "%d,", &value) == 1)
                    file_data.push_back(value);
                fclose(file);
                bool interleaved = true;
                if (file_data.size() != pitches.size() * 2)
                    interleaved = false;
                else
                    for (size_t n = 0; n < pitches.size(); n++) {
                        if (file_data[n * 2] != pitches[n] || file_data[n * 2 + 1] != durations[n]) {
                            interleaved = false;
                            break;
                        }
                    }
                MARK(interleaved,
                    "Pitches and durations saved correctly to file.",
                    "Pitches and durations not saved correctly to file.");
            } else {
                FAIL("Output file not found.");
			}

            return marks;
        }
    } file;

    // Test 9: object - "Objection Noted"
    namespace MyNote { // Class checker for MyNote
        // Helper to detect if MyNote exists
        template<typename = void>
        static constexpr bool is_declared() {
            if constexpr (requires { typename ::MyNote; }) return true; else return false;
        }

        // Helper to detect if MyNote has public member variables pitch, duration, and time
        template<typename = void>
        static constexpr bool has_members() {
            if constexpr (!is_declared()) return false;
            if constexpr (requires { ::MyNote::pitch; ::MyNote::velocity; ::MyNote::duration; }) return true; else return false;
        }

        // Helper to detect if note's members are initialised to 0
        template<typename T>
        static constexpr bool is_note_initialised(T) {
            if (std::is_trivially_default_constructible_v<T>)
                return false;

            if constexpr (requires { T::pitch; T::velocity; T::duration; }) {
                T note;
                return note.pitch == 0 && note.velocity == 0 && note.duration == 0;
            }

            return false;
        }

        // Helper to detect if MyNote is initialised to 0
        template<typename = void>
        static constexpr bool is_initialised() {
            if constexpr (requires { typename ::MyNote; })
                return is_note_initialised(::MyNote());
            return false;
        }
    };
    struct Object : public Test {

        Object() {
            name = "object";
        }
        void run() override { }

#if defined(__GNUC__) || defined(__clang__)
        __attribute__((optimize("O0")))  // Disable optimization for this function
#endif
        Mark mark(const StdioCapture::IO& io) override {
            Mark marks(3);

			// 1 mark for declaring the object.
            if constexpr (MyNote::is_declared()) {
                PASS("Defines an object called MyNote.");

                // 1 mark for declaring public member variables.
                if constexpr (MyNote::has_members()) {
                    PASS("MyNote has public member variables pitch, duration, and time.");

                    MARK (MyNote::is_initialised(),
                        "Member variables are initialised to zero.", 
                        "Member variables are not initialised to zero.");
                } else {
                    FAIL("MyNote does not have the required public member variables.");
                }
            } else {
                FAIL("No object called MyNote defined.");
            }

            return marks;
        }
    } object;

	// Test 10: sequence - "Private Notes"
	namespace MySequence { // Class checker for MySequence
        // Helper to detect if MySequence exists
        template<typename = void>
        static constexpr bool is_declared() {
	        if constexpr (requires { typename ::MySequence; }) return true; else return false;
        }

        // Helper to detect if MySequence has a member, notes
        template<typename = void>
        static constexpr bool has_notes() {
            if constexpr (is_declared())
                if constexpr (requires { ::MySequence::notes; }) 
                    return true; 
            return false;
        }

        // Helper to detect if MySequence::notes is accessible (i.e., public) by trying to access its size() member function
        template<typename T>
        static constexpr bool access_notes(T) {
            if constexpr (has_notes()) {
                if constexpr (requires {{ T{}.notes.size() } -> std::convertible_to<std::size_t>;} )
                    return true; 
            }
            return false;
        }

        // Helper to detect if MySequence::notes is accessible (i.e., public)
        template<typename = void>
        static constexpr bool is_notes_accessible() {
            if constexpr (is_declared()) return access_notes(::MySequence()); 
            return false;
        }

        // Helper to detect if MySequence has public member function at()
        template<typename T>
        static constexpr bool has_at(T) {
            if constexpr (!has_notes()) return false;
            if constexpr (requires { { T{}.at(0) } -> std::same_as<int>;}) return true; 
            return false;
        }

        template<typename = void>
        static constexpr bool has_at() {
            if constexpr (is_declared()) return has_at(::MySequence()); return false;
        }

        // Helper to detect if MySequence has public member function add()
        template<typename T>
        static constexpr bool has_add(T) {
            if constexpr (!has_notes()) return false;
            if constexpr (requires { { T{}.add(0) }; }) return true; 
            return false;
        }

        template<typename = void>
        static constexpr bool has_add() {
            if constexpr (is_declared()) return has_add(::MySequence()); 
            return false;
        }


        // Helper to detect if MySequence has public member function size()
        template<typename T>
        static constexpr bool has_size(T) {
            if constexpr (!has_notes()) return false;
            if constexpr (requires { { T{}.size() } -> std::convertible_to<int>; }) return true; 
            return false;
        }

		template<typename = void>
        static constexpr bool has_size() {
            if constexpr (is_declared()) return has_size(::MySequence()); 
            return false;
		}

        // Helper to detect if MySequence has public member function clear()
        template<typename T>
        static constexpr bool has_clear(T) {
            if constexpr (!has_notes()) return false;
            if constexpr (requires { { T{}.clear() }; }) return true; 
            return false;
        }

		template<typename = void>
        static constexpr bool has_clear() {
            if constexpr (is_declared()) return has_clear(::MySequence()); 
            return false;
		}

		// Helper to detect overloaded subscript operator []
		template<typename T>
        static constexpr bool has_subscript(T) {
            if constexpr (!has_notes()) return false;
            if constexpr (requires { { T{}[0] } -> std::convertible_to<int>;}) return true; 
            return false;
		}

		template<typename = void>
        static constexpr bool has_subscript() {
            if constexpr (is_declared()) return has_subscript(::MySequence());
            return false;
        }

        // Helper to detect if MySequence has public member function clear()
        template<typename T>
        static constexpr bool has_all_functions(T) {
            if constexpr (!has_notes()) return false;
            if constexpr (has_at(T()) && has_add(T()) && has_size(T()) && has_clear(T())) return true;
            return false;
        }

		template<typename = void>
        static constexpr bool has_all_functions() {
            if constexpr (is_declared()) return has_all_functions(::MySequence());
            return false;
		}

        enum Checks {
            Size0 = 1,
            Size1 = 2,
            Size2 = 4,
            Size = Size0|Size1|Size2,
            Add0 = 8,
            Add1 = 16,
            Add = Add0|Add1,
            At = 32,
            Clear = 64,
            All = Size|Add|At|Clear,
            Crash = 128,
			Subscript = 256
        };

        template<typename T>
        static constexpr int test_all_functions(T) {
            if constexpr (!is_declared())
                return false;
            if constexpr (has_all_functions(T())) {
                int checks = 0;
                T s;
                if (s.size() == 0) checks |= Size0;
                s.add(42);
                if (s.size() == 1) checks |= Size1 | Add0;
                if (s.at(0) == 42) checks |= At | Add1;
                if constexpr (has_subscript(T()))
					if (s[0] == 42) checks |= Subscript; // check subscript operator returns same as at()
                s.clear();
                if (s.size() == 0) checks |= Size2 | Clear;

                return checks;
            }

            return 0;
        }

		template<typename = void>
        static constexpr int test_all_functions() {
            if constexpr (is_declared()) return test_all_functions(::MySequence());
            return 0;
		}
    };
    struct Sequence : public Test {
        Sequence() {
            name = "sequence";
        }
        void run() override {}
        Mark mark(const StdioCapture::IO& io) override {
            Mark marks(7);

            // 1 mark for declaring the object.
            if constexpr (MySequence::is_declared()) {
                //PASS("Defines an object called MySequence.");

                // 1 mark if the container is inaccessible (e.g. private or protected).
                if constexpr (!MySequence::has_notes()) {
                    FAIL("The container of notes is not defined.");
                    return marks;
                } else {
                    if constexpr (MySequence::is_notes_accessible()) {
                        FAIL("The note container is accessible externally.");
                    } else {
                        PASS("The note container is inaccessible externally.");
                    } 
                }

                // 4 marks for the public functions (1 per function).
                //    at(...) - returns the note at a given index
                if constexpr (MySequence::has_at()) {
                    PASS("MySequence has public function at(...) for reading a note at a given index.");
                } else {
                    FAIL("MySequence does not have public function at(...) for reading a note at a given index.");
				}
                //    add(...) - adds a note to the sequence
                if constexpr (MySequence::has_add()) {
                    PASS("MySequence has public function add(...) for adding a note to the sequence.");
                } else {
					FAIL("MySequence does not have public function add(...) for adding a note to the sequence.");
				}
                //    size() - returns the number of notes
                if constexpr (MySequence::has_size()) {
                    PASS("MySequence has public function size(...) for returning the length of the array.");
                } else {
					FAIL("MySequence does not have public function size(...) for returning the length of the array.");
				}
                //    clear() - resets (empties) the sequence
                if constexpr (MySequence::has_clear()) {
                    PASS("MySequence has public function clear(...) for clearing the array.");
                } else {
					FAIL("MySequence does not have public function clear(...) for clearing the array.");
				}
                 
                // 1 mark if all functions work as expected
                int checks = 0;
                if (MySequence::has_all_functions()) {
                    using enum MySequence::Checks;

                    try {
                        checks = MySequence::test_all_functions();
                    } catch (...) {
                        checks = Crash;
                    }

                    MARK_IF((checks & All) == All) {
                        PASS("All functions work as expected.");
                    } else if(checks == Crash) {
                        FAIL("Function code crashed.");
                    } else {
                        std::string errors;
                        if ((checks & Size) != Size) errors += "size, ";
                        if ((checks & Add) != Add) errors += "add, ";
                        if ((checks & At) != At) errors += "at, ";      
                        if ((checks & Clear) != Clear) errors += "clear, ";
                        errors.resize(errors.size() - 2); // remove trailing comma
                        FAIL("Unexpected function behaviour (" + errors + "?)");
                    }
                } else {
                    FAIL("Function code incomplete.");
                }
                 
                // 1 mark for overloading the subscript [] operator.
                if constexpr (MySequence::has_subscript()) {
                    MARK(checks & MySequence::Subscript, 
                        "Subscript operator [] provided as replacement for at(...).", 
						"Subscript operator [] does not return the same value as at(...).");
                } else {
                    FAIL("No subscript [] operator provided.");
				}

            } else {
                FAIL("No object called MySequence defined.");
            }

            return marks;
        }
    } sequence;

    // Test 11: point - "Point Counterpoint"
    struct Point : public Test {
        const short* ptr = nullptr;

        Point() {
            name = "point";
        }
        void run() override {
			ptr = unknown.data(); // Get a pointer to the first element of the array
            ::point(ptr, unknown.size());
        }
        Mark mark(const StdioCapture::IO& io) override {
            Mark marks(3);

            if (ptr == unknown.data()) {
                FAIL("Pointer was not incremented to iterate over the array.");
                return marks;
            }

			std::string output = io.out;
            size_t offset = 0;
			bool iterates_correctly = true;
            for(int i = 0; i < unknown.size(); i++) {
				offset = output.find(std::to_string(unknown[i]), offset);
                if (offset == std::string::npos) {
					iterates_correctly = false;
					break;
                }
				offset++; // move past the found number for the next search
			}

            offset = 0;
			bool decodes_correctly = true;
            for(int i = 0; i < unknown.size(); i++) {
				offset = output.find(decode(unknown[i]), offset);
                if (offset == std::string::npos) {
					decodes_correctly = false;
					break;
                }
				offset++; // move past the found number for the next search
			}

            // 1 mark for iterating over the array.
            MARK(iterates_correctly || decodes_correctly, 
                "Iterates over the array correctly.", 
				"Does not iterate over the array correctly.");

			// 1 mark for decoding the values correctly.
            MARK(decodes_correctly, 
				"Array is successfully decoded.",
				"Array is not successfully decoded.");
             
            // 1 mark for if the pointer ends after the array.
            MARK(ptr == unknown.data() + unknown.size(), 
				"Pointer now rests at the end of the array.",   
                "Pointer does not rest at the end of the array.");

            return marks;
        }
    } point;
    
    Mark all() {
        Mark marks;
        marks += hello();
        marks += beats();
        marks += loop();
		marks += tree();
		marks += transpose();
		marks += counterpoint();
		marks += play();
		marks += file();
		marks += object();
		marks += sequence();
		marks += point();

		std::cout << "\n[Total marks: " << marks.marks << " / " << marks.total << " - " << int(round(marks.marks * 100.0 / marks.total) + 0.001) << "%]\n";

        return marks;
    }

// Restore settings
#ifdef _MSC_VER
    #pragma warning(default:4700)
    #pragma runtime_checks("", restore)
    #pragma optimize("", on)
#elif defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
#endif


}; // namespace test