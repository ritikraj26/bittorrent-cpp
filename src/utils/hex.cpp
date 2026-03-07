#include "hex.hpp"

std::string to_hex(const std::string& data) {

    static const char* hex_chars = "0123456789abcdef";

    std::string output;
    output.reserve(data.size() * 2);

    for (unsigned char c : data) {
        output.push_back(hex_chars[c >> 4]);
        output.push_back(hex_chars[c & 0x0F]);
    }

    return output;
}