#include "encode.hpp"
#include <stdexcept>

// internal helpers
static std::string encode_string(const std::string& str) {
    return std::to_string(str.size()) + ":" + str;
}

static std::string encode_integer(int64_t value) {
    return "i" + std::to_string(value) + "e";
}

static std::string encode_list(const json& list) {
    std::string result = "l";
    for (const auto& item : list) {
        result += encode_bencoded_value(item);
    }
    result += "e";
    return result;
}

std::string encode_dict(const json& dict) {
    std::string result = "d";
    for (auto it = dict.begin(); it != dict.end(); ++it) {
        result += encode_string(it.key());
        result += encode_bencoded_value(it.value());
    }
    result += "e";
    return result;
}

std::string encode_bencoded_value(const json& value) {
    if (value.is_string())
        return encode_string(value.get<std::string>());
    if (value.is_number_integer())
        return encode_integer(value.get<int64_t>());
    if (value.is_array())
        return encode_list(value);
    if (value.is_object())
        return encode_dict(value);

    throw std::runtime_error("Unsupported JSON type for bencoding");
}
