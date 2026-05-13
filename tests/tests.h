#include "tests.cpp"

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
}

struct Mark {
    int marks = 0;
    int total = 0;

    Mark& operator+=(const Mark& other) {
        marks += other.marks;
        total += other.total;
        return *this;
    }

    double percent() const {
        return total > 0 ? (static_cast<double>(marks) / total) * 100.0 : 0.0;
    }
};

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

        void clear(){
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
            : prefix_(std::move(prefix)), io_(io) {}

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
            } else if(!prefix_.empty()){
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
            : io_(io), echo_(echo) {}

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

            if(!echo_)
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
        } catch (...) {
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

Mark test_hello (const StdioCapture::IO& io) {
    int marks = 0;
    const int total = 4;

	size_t hello_pos = lowercase(io.out).find("hello");

    // 1 mark for the word "hello" in the output (case-insensitive)
    if(hello_pos != std::string::npos){
        marks++;
        std::cout << ansi::green << "[PASS] Output contains 'hello'.\n" << ansi::reset;
    } else {
        std::cout << ansi::red << "[FAIL] Output does not contain 'hello'.\n" << ansi::reset;
    }

    // 1 mark for capitals - all words should start with capital letters
    // Extract all alphabetic words and check first character is uppercase
    std::regex word_pattern(R"([A-Za-z]+)");
	std::string greeting = io.out.substr(hello_pos); // Check capitalization in the part of the output starting from "hello"
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
    
    if (word_count > 0 && all_capitalized) {
        marks++;
        std::cout << ansi::green << "[PASS] Output contains correct capitalization.\n" << ansi::reset;
    } else {
        std::cout << ansi::red << "[FAIL] Output does not contain correct capitalization.\n" << ansi::reset;
    }

    std::regex punctuation_pattern(R"([A-Za-z0-9 ]+,\s*[A-Za-z0-9 ]+!)");
    if(std::regex_search(io.out, punctuation_pattern)){
        marks++;
        std::cout << ansi::green << "[PASS] Output contains correct punctuation.\n" << ansi::reset;
    } else {
        std::cout << ansi::red << "[FAIL] Output does not contain correct punctuation.\n" << ansi::reset;
    }

    // 1 mark for supporting a user-entered name (e.g. "Hello, Kevin!")
    std::string name = trim_trailing_newlines(io.in);
    if(io.inputs() != 0 && lowercase(io.out).find(lowercase(name)) != std::string::npos){
        marks++;
        std::cout << ansi::green << "[PASS] Output contains user-entered name.\n" << ansi::reset;
    } else {
        std::cout << ansi::red << "[FAIL] Output does not contain user-entered name.\n" << ansi::reset;
    }

    return { marks, total };
}

Mark test_beats(const StdioCapture::IO& io) {
    int marks = 0;
    const int total = 3;
    std::string output = lowercase(io.out);
    enum { NO_BEATS_PROMPT, BEATS_PROMPT, BEATS_CORRECT };
	int mode = io.inputs() == 2 ? BEATS_PROMPT : NO_BEATS_PROMPT;

    if (mode == BEATS_PROMPT) { // test multiple beats case
        // Three beats at 160 BPM should be 500ms
        // 60000ms / 160 BPM = 375ms per beat
    
        // 1 mark for correctly converting multiple beats (e.g. three beats at 160bpm)
        if (output.find("1125") != std::string::npos) {
            marks++;
			mode = BEATS_CORRECT;
            std::cout << ansi::green << "[PASS] Correctly converts beats to milliseconds.\n" << ansi::reset;
        } else {
            std::cout << ansi::red << "[FAIL] Does not correctly convert beats to milliseconds.\n" << ansi::reset;
        }
	} else { // test single beat case
        // Single beat at 160 BPM should be 375ms
        // 60000ms / 160 BPM = 375ms per beat
        bool found_375 = output.find("375") != std::string::npos;
    
        // 1 mark for correctly converting a single beat (e.g. a beat at 160bpm)
        if (found_375) {
            marks++;
            std::cout << ansi::green << "[PASS] Correctly converts beats to milliseconds.\n" << ansi::reset;
        } else {
            std::cout << ansi::red << "[FAIL] Does not correctly convert beats to milliseconds.\n" << ansi::reset;
        }
    }

    // 1 mark for user interaction (input handling, prompts)
    bool has_prompt = output.find("bpm") != std::string::npos || 
                      output.find("beat") != std::string::npos ||
		              output.find("number") != std::string::npos ||
                      output.find("tempo") != std::string::npos ||
                      output.find("enter") != std::string::npos;
    
    if (has_prompt && io.inputs() != 0) {
        marks++;
        std::cout << ansi::green << "[PASS] Provides user interaction and prompts.\n" << ansi::reset;
    } else {
        std::cout << ansi::red << "[FAIL] Missing user interaction or prompts.\n" << ansi::reset;
    }

    // 1 mark for supporting multiple beats (e.g. 3 beats at 180bpm)
    if (mode == BEATS_CORRECT) {
        marks++;
        std::cout << ansi::green << "[PASS] Supports multiple beats.\n" << ansi::reset;
    } else {
        std::cout << ansi::red << "[FAIL] Does not support multiple beats.\n" << ansi::reset;
    }

    return { marks, total };
}

Mark test_loop(const StdioCapture::IO& io) {
    int marks = 0;
    const int total = 4;

	constexpr char PATTERN[]       = "|p.x.P.x.x.p.P.x.";
    constexpr char PATTERN_NOEND[] = "|p.x.P.x.x.p.P.";
	constexpr int PATTERN_LENGTH = sizeof(PATTERN) - 1; // exclude null terminator

	auto ignored = [] (char c){ return std::isspace(static_cast<unsigned char>(c)) || c == '\n' || c == '\r'; };

    // trim spaces
    std::string pattern = io.out;
	pattern.erase(std::remove_if(pattern.begin(), pattern.end(), ignored), pattern.end());

    // 1 mark for printing the pattern
    // Pattern: | p . x . P . x . x . p . P . x . (spacing/linefeed optional)
	size_t first_bar = pattern.find(PATTERN);
    if (first_bar != std::string::npos) {
        marks++;
        std::cout << ansi::green << "[PASS] Output contains drum pattern.\n" << ansi::reset;
    } else {
        std::cout << ansi::red << "[FAIL] Output does not contain drum pattern.\n" << ansi::reset;
    }

    // 1 mark for including a closing bar line
	size_t last_bar = pattern.rfind(PATTERN_NOEND);
    if (last_bar != std::string::npos && (last_bar + PATTERN_LENGTH < pattern.size())
       && pattern[last_bar + PATTERN_LENGTH] == '|') {
        marks++;
        std::cout << ansi::green << "[PASS] Output includes closing bar line.\n" << ansi::reset;
    } else {
        std::cout << ansi::red << "[FAIL] Output does not include closing bar line.\n" << ansi::reset;
    }

	// 1 mark for repeating the pattern 
    if (last_bar != first_bar) {
        marks++;
        std::cout << ansi::green << "[PASS] Pattern is repeated.\n" << ansi::reset;
    } else {
        std::cout << ansi::red << "[FAIL] Pattern is not repeated.\n" << ansi::reset;
    }

    // 1 mark if the repeat ends with a bass drum (p) instead of a hihat (x)
    // Look for the ending pattern before the final bar
	if (pattern[last_bar + PATTERN_LENGTH - 2] == 'p') {
        marks++;
        std::cout << ansi::green << "[PASS] Repeat ends with bass drum (p).\n" << ansi::reset;
    } else {
        std::cout << ansi::red << "[FAIL] Repeat does not end with bass drum (p).\n" << ansi::reset;
    }

    return { marks, total };
}

Mark test_tree(const StdioCapture::IO& io) {
    int marks = 0;
    const int total = 5;
    std::string output = io.out;
    
    // 1 mark for correctly tagging a given value if it appears at the root
	if (output.find("{ C }") != std::string::npos) {
		marks++;
        std::cout << ansi::green << "[PASS] Correctly tags value at root.\n" << ansi::reset;
    } else {
        std::cout << ansi::red << "[FAIL] Does not correctly tag value at root.\n" << ansi::reset;
	}
     
    // 1 mark for correctly tagging a given value in the root's immediate children
    if (output.find("{ C > Eb }") != std::string::npos) {
        marks++;
        std::cout << ansi::green << "[PASS] Correctly tags values in root's immediate children.\n" << ansi::reset;
    } else {
        std::cout << ansi::red << "[FAIL] Does not correctly tag values in root's immediate children.\n" << ansi::reset;
	}
    
    // 1 mark for correctly tagging a given value anywhere in the tree
    if (output.find("{ C > E > G > B }") != std::string::npos) {
        marks++;
        std::cout << ansi::green << "[PASS] Correctly tags values anywhere in the tree.\n" << ansi::reset;
    } else {
        std::cout << ansi::red << "[FAIL] Does not correctly tag values anywhere in the tree.\n" << ansi::reset;
    }
    
    // 1 mark for correctly printing "<value> not found" when a value is absent
    if (output.find("F# not found") != std::string::npos) {
        marks++;
        std::cout << ansi::green << "[PASS] Correctly prints \"not found\" when value is absent.\n" << ansi::reset;
    } else {
        std::cout << ansi::red << "[FAIL] Does not correctly return false when value is absent.\n" << ansi::reset;
    }
    
    // 1 mark for correctly tagging all instances of a given value in the tree (e.g. if a value appears multiple times)
	if (output.find("{ C > E > G }") != std::string::npos 
     && output.find("{ C > Eb > G }") != std::string::npos) {
        marks++;
        std::cout << ansi::green << "[PASS] Correctly tags all instances of a value in the tree.\n" << ansi::reset;
    } else {
        std::cout << ansi::red << "[FAIL] Does not correctly tag all instances of a value in the tree.\n" << ansi::reset;
	}
    
    return { marks, total };
}

#ifdef WIN32
#define TRY __try
#define CATCH __except(EXCEPTION_EXECUTE_HANDLER)
#else
#define TRY try
#define CATCH catch(const std::exception& e)     
#endif

Mark test(void (*test_func)()) {
    StdioCapture::IO stdio;
    Mark mark = { 0, 0 };

    // Test configuration: name, input, and marking function
    struct TestConfig {
        void (*func)();
        const char* name;
        const char* input;
        Mark (*marker)(const StdioCapture::IO&);
    };

    // Dispatch table
    static const TestConfig tests[] = {
        { hello, "Hello, World!", "kevin\n", test_hello },
        { beats, "Beats by Dr. Nash", "160\n3\n", test_beats },
        { loop, "Amen, Brother", "", test_loop },
        { tree, "Roots Manoeuvre", "", test_tree },
    };

    // Find and run the matching test
    for (const auto& config : tests) {
        if (test_func == config.func) {
            std::cout << ansi::grey << "\n[Running test: " << config.name << "]\n\n" << ansi::white;
            
            {
                StdioCapture capture(stdio, config.input);
                test_func();
            }

            std::cout << "\n";
            mark = config.marker(stdio);
            std::cout << ansi::grey << "\n[Test marks: " << mark.marks << " / " << mark.total << "]\n" << ansi::reset;
            return mark;
        }
    }

    // Unknown test function
    std::cerr << ansi::red << "\n[Unknown test function]\n" << ansi::reset;
    return mark;
}

// detect: void hello()
template <typename = void>
struct has_hello : std::false_type {};

template <>
struct has_hello<std::void_t<decltype(hello())>> : std::true_type {};

// detect: void beats()
template <typename = void>
struct has_beats : std::false_type {};

template <>
struct has_beats<std::void_t<decltype(beats())>> : std::true_type {};

// detect: void loop()
template <typename = void>
struct has_loop : std::false_type {};

template <>
struct has_loop<std::void_t<decltype(loop())>> : std::true_type {};

// detect: void tree()
template <typename = void>
struct has_tree : std::false_type {};

template <>
struct has_tree<std::void_t<decltype(tree())>> : std::true_type {};

Mark test_all() {
    Mark marks = { 0, 0 };
    
    if constexpr (has_hello<>::value)
        marks += test(hello);
    else
        std::cerr << ansi::red << "Test function 'hello()' not found.\n" << ansi::reset;
    
    if constexpr (has_beats<>::value)
        marks += test(beats);
    else
        std::cerr << ansi::red << "Test function 'beats()' not found.\n" << ansi::reset;

    if constexpr (has_loop<>::value)
        marks += test(loop);
    else
        std::cerr << ansi::red << "Test function 'loop()' not found.\n" << ansi::reset;

    if constexpr (has_tree<>::value)
        marks += test(tree);
    else
        std::cerr << ansi::red << "Test function 'tree()' not found.\n" << ansi::reset;

    std::cout << ansi::white << "\nTotal marks: " << marks.marks << " / " << marks.total << " (" << marks.percent() << "%)\n\n" << ansi::reset;
    return marks;
}