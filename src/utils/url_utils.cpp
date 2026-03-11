#include "url_utils.hpp"

#include <string>
#include <cctype>
#include <sstream>
#include <iomanip>

std::string url_decode(const std::string& input) {
    std::string result;

    for (size_t i = 0; i < input.size(); i++) {
        if (input[i] == '%' && i + 2 < input.size()) {
            int value = std::stoi(input.substr(i + 1, 2), nullptr, 16);
            result.push_back(static_cast<char>(value));
            i += 2;
        }
        else if (input[i] == '+') {
            result.push_back(' ');
        }
        else {
            result.push_back(input[i]);
        }
    }

    return result;
}

std::string url_encode(const std::string& data) {

    std::ostringstream encoded;

    encoded << std::hex << std::uppercase;

    for (unsigned char c : data) {

        if (
            (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~'
        ) {
            encoded << c;
        }
        else {
            encoded << '%'
                    << std::setw(2)
                    << std::setfill('0')
                    << (int)c;
        }
    }

    return encoded.str();
}