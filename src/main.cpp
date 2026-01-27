#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <stdexcept>
#include <fstream>

#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

/* Forward declaration */
json decode(const std::string& s, size_t& i);

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
