#include "app.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>

int main() { //MAIN HERE
    try {
        App{}.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
