#include "hex_utils.hpp"

#include <stdexcept>

std::string bytes_to_hex(const std::string& data) {

    static const char* hex_chars = "0123456789abcdef";

    std::string output;
    output.reserve(data.size() * 2);

    for (unsigned char c : data) {
        output.push_back(hex_chars[c >> 4]);
        output.push_back(hex_chars[c & 0x0F]);
    }

    return output;
}

std::string hex_to_bytes(const std::string& hex) {
    if (hex.size() % 2 != 0) {
        throw std::runtime_error("Invalid hex string");
    }

    std::string output;
    output.reserve(hex.size() / 2);

    auto hex_value = [](char c) -> int {
        if ('0' <= c && c <= '9') return c - '0';
        if ('a' <= c && c <= 'f') return c - 'a' + 10;
        if ('A' <= c && c <= 'F') return c - 'A' + 10;
        throw std::runtime_error("Invalid hex character");
    };

    for (size_t i = 0; i < hex.size(); i += 2) {
        int high = hex_value(hex[i]);
        int low  = hex_value(hex[i + 1]);

        char byte = (high << 4) | low;
        output.push_back(byte);
    }

    return output;
}