#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iomanip>


#include <openssl/sha.h>
#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

/* Forward declaration */
json decode(const std::string& s, size_t& i);

std::string encode_bencoded_value(const json& value);


std::string calculate_sha1(const std::string& input) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);

    std::ostringstream oss;
    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return oss.str();
}

//encoders
std::string encode_string(const std::string& str) {
    return std::to_string(str.size()) + ":" + str;
}

std::string encode_integer(int64_t value) {
    return "i" + std::to_string(value) + "e";
}

std::string encode_list(const json& list) {
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
    if (value.is_string()) {
        return encode_string(value.get<std::string>());
    } else if (value.is_number_integer()) {
        return encode_integer(value.get<int64_t>());
    } else if (value.is_array()) {
        return encode_list(value);
    } else if (value.is_object()) {
        return encode_dict(value);
    } else {
        throw std::runtime_error("Unsupported JSON type for bencoding");
    }
}

// decoders
json decode_string(const std::string& s, size_t& i) {
    size_t colon = s.find(':', i);
    if (colon == std::string::npos)
        throw std::runtime_error("Invalid string");

    int len = std::stoi(s.substr(i, colon - i));
    i = colon + 1;

    std::string value = s.substr(i, len);
    i += len;

    return value;
}

json decode_integer(const std::string& s, size_t& i) {
    i++; // skip 'i'
    size_t end = s.find('e', i);
    if (end == std::string::npos)
        throw std::runtime_error("Invalid integer");

    long long val = std::stoll(s.substr(i, end - i));
    i = end + 1;

    return val;
}

json decode_list(const std::string& s, size_t& i) {
    i++; // skip 'l'
    json list = json::array();

    while (s[i] != 'e') {
        list.push_back(decode(s, i));
    }

    i++; // skip 'e'
    return list;
}

json decode_dict(const std::string& s, size_t& i) {
    i++; // skip 'd'
    json dict = json::object();

    while (s[i] != 'e') {
        json key = decode_string(s, i);
        json value = decode(s, i);
        dict[key.get<std::string>()] = value;
    }

    i++; // skip 'e'
    return dict;
}

json decode(const std::string& s, size_t& i) {
    if (std::isdigit(static_cast<unsigned char>(s[i])))
        return decode_string(s, i);

    if (s[i] == 'i')
        return decode_integer(s, i);

    if (s[i] == 'l')
        return decode_list(s, i);

    if (s[i] == 'd')
        return decode_dict(s, i);

    throw std::runtime_error("Invalid bencode");
}

json decode_bencoded_value(const std::string& encoded_value) {
    size_t i = 0;
    return decode(encoded_value, i);
}

int main(int argc, char* argv[]) {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " decode <encoded_value>\n";
        return 1;
    }

    std::string command = argv[1];

    if (command == "decode") {
        try {
            json decoded = decode_bencoded_value(argv[2]);
            std::cout << decoded.dump() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error: Failed to decode the value. " << e.what() << "\n";
            return 1;
        }
    } else if (command == "info") {
        std::ifstream file(argv[2], std::ios::binary);
        if (!file) {
            std::cerr << "Error: Unable to open file " << argv[2] << "\n";
            return 1;
        }

        std::string file_content(
            (std::istreambuf_iterator<char>(file)),
             std::istreambuf_iterator<char>()
        );

        try {
            json decoded = decode_bencoded_value(file_content);

            if (decoded.contains("announce")) {
                std::cout<<"Tracker URL: "<<decoded["announce"].get<std::string>()<<"\n";
            } else {
                std::cerr << "Error: 'announce' key not found in the torrent file.\n";
            }

            if (decoded.contains("info") && decoded["info"].contains("length")) {
                std::cout<<"Length: "<<decoded["info"]["length"].get<long long>()<<"\n";
            } else {
                std::cerr << "Error: 'info.length' key not found in the torrent file.\n";
            }

            if (decoded.contains("info")) {
                std::string bencoded_dict = encode_dict(decoded["info"]);
                std::string sha1_hash = calculate_sha1(bencoded_dict);
                std::cout << "Info Hash: "<<sha1_hash<<"\n";

            } else {
                std::cerr << "Error: 'info' key not found in the torrent file.\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: Failed to decode the torrent file. " << e.what() << "\n";
            return 1;
        }
    } else {
        std::cerr << "Unknown command: " << command << "\n";
        std::cerr << "Usage: " << argv[0] << " decode <encoded_value>\n";
        std::cerr << "       " << argv[0] << " info <torrent_file>\n";
        return 1;
    }

    return 0;
}
