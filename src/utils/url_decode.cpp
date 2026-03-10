#include "url_decode.hpp"

#include <string>
#include <cctype>

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