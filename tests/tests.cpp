#include <iostream>
#include <string>

void hello () {
    std::string name;
    std::cout << "Please enter your name: ";
    std::getline(std::cin, name);

    if (name.empty()) {
        std::cout << "Hello, World!" << '\n';
    } else {
        std::cout << "Hello, " << name << "!" << '\n';
    }
}