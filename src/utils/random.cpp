#include "random.hpp"
#include <random>

std::string generate_peer_id(int length) {

    const std::string charset =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789";

    std::random_device rd;
    std::mt19937 generator(rd());

    std::uniform_int_distribution<> distribution(0, charset.size() - 1);

    std::string id;
    id.reserve(length);

    for (int i = 0; i < length; i++) {
        id.push_back(charset[distribution(generator)]);
    }

    return id;
}