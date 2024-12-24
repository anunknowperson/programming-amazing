#include <iostream>
#include <memory>
#include <stdexcept>

#include "IController.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] char* args[]) {
    try {
        const auto controller = createInstance();
        controller->init();
    } catch (const std::runtime_error& e) {
        std::cerr << "Unhandled exception: " << e.what() << '\n';
    }

    return 0;
}