#include <klang.h>

int main() {
    return klang::version.atLeast(0, 7, 10) ? 0 : 1;
}
