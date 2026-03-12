#include "decode.hpp"
#include <cctype>
#include <stdexcept>

static json decode(const std::string& s, size_t& i);

static json decode_string(const std::string& s, size_t& i) {
    size_t colon = s.find(':', i);
    if (colon == std::string::npos)
        throw std::runtime_error("Invalid bencode string");

    int len = std::stoi(s.substr(i, colon - i));
    i = colon + 1;

    json value = s.substr(i, len);
    i += len;
    return value;
}

static json decode_integer(const std::string& s, size_t& i) {
    i++;
    size_t end = s.find('e', i);
    if (end == std::string::npos)
        throw std::runtime_error("Invalid bencode integer");

    long long val = std::stoll(s.substr(i, end - i));
    i = end + 1;
    return val;
}

static json decode_list(const std::string& s, size_t& i) {
    i++;
    json list = json::array();

    while (s[i] != 'e')
        list.push_back(decode(s, i));

    i++;
    return list;
}

static json decode_dict(const std::string& s, size_t& i) {
    i++;
    json dict = json::object();

    while (s[i] != 'e') {
        json key = decode_string(s, i);
        json value = decode(s, i);
        dict[key.get<std::string>()] = value;
    }

    i++;
    return dict;
}

static json decode(const std::string& s, size_t& i) {
    if (std::isdigit(static_cast<unsigned char>(s[i])))
        return decode_string(s, i);
    if (s[i] == 'i')
        return decode_integer(s, i);
    if (s[i] == 'l')
        return decode_list(s, i);
    if (s[i] == 'd')
        return decode_dict(s, i);

    throw std::runtime_error("Invalid bencode format");
}

json decode_bencoded_value(const std::string& encoded) {
    size_t i = 0;
    return decode(encoded, i);
}

std::pair<json, size_t> decode_bencoded_value_with_position(const std::string& encoded) {
    size_t i = 0;
    json result = decode(encoded, i);
    return {result, i};
}
