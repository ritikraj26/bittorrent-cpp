#include "file_reader.hpp"

#include <fstream>
#include <stdexcept>
#include <iterator>

std::string read_file(const std::string& path) {

    std::ifstream file(path, std::ios::binary);

    if (!file) {
        throw std::runtime_error("Failed to open file: " + path);
    }

    return std::string(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
}