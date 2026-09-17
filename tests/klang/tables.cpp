#include <klang.h>
#include <cstdio>
#include <cstdlib>

static int checks = 0;
void require(bool okay, const char* expression, int line) {
    ++checks;
    if (!okay) {
        std::fprintf(stderr, "line %d: %s\n", line, expression);
        std::exit(1);
    }
}
#define CHECK(expression) require(bool(expression), #expression, __LINE__)

#ifndef CALLABLE_TABLES
int square(int x) { return x * x; }
#endif

int main() {
#ifdef CALLABLE_TABLES
    const int offset = 3;
    klang::Table<int, 4> values = [offset](int x) { return x * x + offset; };
    const int scale = 2;
    klang::Table<int, 4> enhanced = [scale](int x, klang::Table<int, 4>::Result& y) {
        y = x * scale;
    };
#else
    klang::Table<int, 4> values = square;
    klang::Table<int, 4> enhanced = square;
#endif
    CHECK(values.count == 4 && enhanced.count == 4);
#ifdef CALLABLE_TABLES
    CHECK(values[0] == 3 && values[1] == 4 && values[2] == 7 && values[3] == 12);
    CHECK(enhanced[0] == 0 && enhanced[1] == 2 && enhanced[2] == 4 && enhanced[3] == 6);
#else
    CHECK(values[0] == 0 && values[1] == 1 && values[2] == 4 && values[3] == 9);
    CHECK(enhanced[0] == 0 && enhanced[1] == 1 && enhanced[2] == 4 && enhanced[3] == 9);
#endif
    std::printf("%d table checks passed\n", checks);
}
