#include "piece_hash.hpp"
#include <stdexcept>

std::vector<std::string> extract_piece_hashes(const std::string& pieces_blob) {

    if (pieces_blob.size() % 20 != 0) {
        throw std::runtime_error("Invalid pieces field length");
    }

    std::vector<std::string> hashes;

    hashes.reserve(pieces_blob.size() / 20);

    for (size_t i = 0; i < pieces_blob.size(); i += 20) {
        hashes.push_back(pieces_blob.substr(i, 20));
    }

    return hashes;
}