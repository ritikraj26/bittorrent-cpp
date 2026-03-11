#include "sha1.hpp"

#include <openssl/sha.h>
#include "utils/hex_utils.hpp"

std::string sha1_raw(const std::string& data) {

    unsigned char hash[SHA_DIGEST_LENGTH];

    SHA1(
        reinterpret_cast<const unsigned char*>(data.data()),
        data.size(),
        hash
    );

    return std::string(
        reinterpret_cast<char*>(hash),
        SHA_DIGEST_LENGTH
    );
}

std::string sha1_hex(const std::string& data) {

    return bytes_to_hex(sha1_raw(data));
}