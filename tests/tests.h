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

struct mark {
    int marks = 0;
    int total = 0;

    mark& operator+=(const mark& other) {
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

class ScopedStdioCapture {
public:
    ScopedStdioCapture(std::string input = "")
        : input_text_(std::move(input))
        , stdin_buf_(input_text_, &captured_stdin_)
        , stdout_buf_(&captured_stdout_)
        , stderr_buf_(&captured_stderr_)
    {
        hook();
    }

    ScopedStdioCapture(const ScopedStdioCapture&) = delete;
    ScopedStdioCapture& operator=(const ScopedStdioCapture&) = delete;

    ~ScopedStdioCapture() {
        unhook();
    }

    std::string stdin_text() {
        return captured_stdin_;
    }

    std::string stdout_text() {
        std::cout.flush();
        return captured_stdout_;
    }

    std::string stderr_text() {
        std::cerr.flush();
        return captured_stderr_;
    }

private:
    class CaptureInputBuf : public std::streambuf {
    public:
        CaptureInputBuf(std::string prefix, std::string* captured)
            : prefix_(std::move(prefix)), captured_(captured) {}

        void set_source(std::streambuf* source) {
            source_ = source;
        }

    protected:
        int_type underflow() override {
            if (prefix_pos_ < prefix_.size()) {
                return traits_type::to_int_type(prefix_[prefix_pos_]);
            }

            if (!source_) {
                return traits_type::eof();
            }

            return source_->sgetc();
        }

        int_type uflow() override {
            if (prefix_pos_ < prefix_.size()) {
                const int_type c = traits_type::to_int_type(prefix_[prefix_pos_]);
                ++prefix_pos_;
                if (captured_) {
                    captured_->push_back(traits_type::to_char_type(c));
                }
                return c;
            }

            if (!source_) {
                return traits_type::eof();
            }

            const int_type c = source_->sbumpc();
            if (traits_type::eq_int_type(c, traits_type::eof())) {
                return c;
            }

            if (captured_) {
                captured_->push_back(traits_type::to_char_type(c));
            }

            return c;
        }

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
        std::string* captured_ = nullptr;
        std::streambuf* source_ = nullptr;
        std::size_t prefix_pos_ = 0;
    };

    class TeeOutputBuf : public std::streambuf {
    public:
        explicit TeeOutputBuf(std::string* captured)
            : captured_(captured) {}

        void set_sink(std::streambuf* sink) {
            sink_ = sink;
        }

    protected:
        int_type overflow(int_type ch) override {
            if (traits_type::eq_int_type(ch, traits_type::eof())) {
                return traits_type::not_eof(ch);
            }

            const char c = traits_type::to_char_type(ch);
            if (captured_) {
                captured_->push_back(c);
            }

            if (!sink_) {
                return traits_type::eof();
            }

            return sink_->sputc(c);
        }

        std::streamsize xsputn(const char* s, std::streamsize count) override {
            if (captured_ && count > 0) {
                captured_->append(s, static_cast<std::size_t>(count));
            }

            if (!sink_ || count <= 0) {
                return 0;
            }

            return sink_->sputn(s, count);
        }

        int sync() override {
            if (!sink_) {
                return 0;
            }

            return sink_->pubsync();
        }

    private:
        std::string* captured_ = nullptr;
        std::streambuf* sink_ = nullptr;
    };

    std::string input_text_;
    std::string captured_stdin_;
    std::string captured_stdout_;
    std::string captured_stderr_;

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

static std::string trim_trailing_newlines(std::string s) {
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) {
        s.pop_back();
    }
    return s;
}

static std::string lowercase(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

mark test_hello (std::string input, std::string output) {
    int marks = 0;
    const int total = 4;

    // 1 mark for the word "hello" in the output (case-insensitive)
    if(lowercase(output).find("hello") != std::string::npos){
        marks++;
        std::cout << ansi::green << "[PASS] Output contains 'hello'.\n" << ansi::reset;
    } else {
        std::cout << ansi::red << "[FAIL] Output does not contain 'hello'.\n" << ansi::reset;
    }

    // 1 mark for capitals in "Hello" and subsequent word
    size_t hello_pos = output.find("Hello");
    if(hello_pos != std::string::npos && output.length() > hello_pos + 7 && std::isupper(output[hello_pos + 7])){
        marks++;
        std::cout << ansi::green << "[PASS] Output contains correct capitalization.\n" << ansi::reset;
    } else {
        std::cout << ansi::red << "[FAIL] Output does not contain correct capitalization.\n" << ansi::reset;
    }

    // 1 mark for punctuation (commas, exclamation marks)
    if(output.find("Hello,") != std::string::npos && (output[output.length() - 1] == '!' || output[output.length() - 2] == '!')){
        marks++;
        std::cout << ansi::green << "[PASS] Output contains correct punctuation.\n" << ansi::reset;
    } else {
        std::cout << ansi::red << "[FAIL] Output does not contain correct punctuation.\n" << ansi::reset;
    }

    // 1 mark for supporting a user-entered name (e.g. "Hello, Kevin!")
    input = trim_trailing_newlines(input);
    if(input.length() && output.find(input) != std::string::npos){
        marks++;
        std::cout << ansi::green << "[PASS] Output contains user-entered name.\n" << ansi::reset;
    } else {
        std::cout << ansi::red << "[FAIL] Output does not contain user-entered name.\n" << ansi::reset;
    }

    return { marks, 4 };
}

mark test(void (*test_func)()) {
    if(test_func == hello){
        std::cout << ansi::grey << "[Running test: Hello, World!]\n\n" << ansi::white;
        std::string in, out, err;
        { 
            ScopedStdioCapture capture;
            test_func();
            in = capture.stdin_text();
            out = capture.stdout_text();
            err = capture.stderr_text();
        }
        // printf("\nCaptured input:\n%s", in.c_str());
        // printf("\nCaptured output:\n%s", out.c_str());
        // printf("\nCaptured error:\n%s", err.c_str());

        std::cout << "\n";

        mark mark = test_hello(in, out);
        std::cout << ansi::grey << "\n[Test marks: " << mark.marks << " / " << mark.total << "]\n" << ansi::reset;
        return mark;
    } else {
        std::cerr << ansi::red << "[Unknown test function]\n" << ansi::reset;
    }
    return { 0, 0 };
}

// detect: void hello()
template <typename = void>
struct has_hello : std::false_type {};

template <>
struct has_hello<std::void_t<decltype(hello())>> : std::true_type {};

mark test_all() {
    mark marks = { 0, 0 };
    if constexpr (has_hello<>::value) {
        marks += test(hello);
    } else {
        std::cerr << ansi::red << "Test function 'hello()' not found.\n" << ansi::reset;
    }
    std::cout << ansi::white << "\nTotal marks: " << marks.marks << " / " << marks.total << " (" << marks.percent() << "%)\n\n" << ansi::reset;
    return marks;
}