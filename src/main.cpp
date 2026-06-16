#include "CLI.hpp"
#include <iostream>

int main() {
    try {
        CLI cli;
        cli.Run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }
}
