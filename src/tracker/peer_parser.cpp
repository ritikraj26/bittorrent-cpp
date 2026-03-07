#include "peer_parser.hpp"
#include <iostream>

void print_peers(const std::string& peers_blob) {

    for (size_t i = 0; i + 6 <= peers_blob.size(); i += 6) {

        unsigned char a = peers_blob[i];
        unsigned char b = peers_blob[i + 1];
        unsigned char c = peers_blob[i + 2];
        unsigned char d = peers_blob[i + 3];

        unsigned short port =
            (static_cast<unsigned char>(peers_blob[i + 4]) << 8) |
            static_cast<unsigned char>(peers_blob[i + 5]);

        std::cout
            << (int)a << "."
            << (int)b << "."
            << (int)c << "."
            << (int)d << ":"
            << port << "\n";
    }
}