#include "url_encode.hpp"
#include <sstream>
#include <iomanip>

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