#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

int main() {
    const char* log_path = std::getenv("LOG_PATH");
    if (!log_path) {
        log_path = "log.txt";
    }

    std::ofstream log(log_path, std::ios::app);
    if (!log) {
        std::cerr << "demo: cannot open log file: " << log_path << std::endl;
        return 1;
    }

    std::string line;
    while (std::getline(std::cin, line)) {
        log << line << '\n';
        log.flush();
    }
    return 0;
}
