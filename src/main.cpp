#include <iostream>
#include <fstream>
#include <string>

#include "lib/nlohmann/json.hpp"
#include "bencode/decode.hpp"
#include "bencode/encode.hpp"
#include "crypto/sha1.hpp"

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    if (argc < 3) {
        std::cerr << "Usage:\n";
        std::cerr << argv[0] << " decode <bencoded_value>\n";
        std::cerr << argv[0] << " info <torrent_file>\n";
        return 1;
    }

    std::string command = argv[1];

    if (command == "decode") {
        try {
            json decoded = decode_bencoded_value(argv[2]);
            std::cout << decoded.dump() << "\n";
        } catch (const std::exception& e) {
            std::cerr << "Decode error: " << e.what() << "\n";
            return 1;
        }
    }

    else if (command == "info") {
        std::ifstream file(argv[2], std::ios::binary);
        if (!file) {
            std::cerr << "Failed to open file: " << argv[2] << "\n";
            return 1;
        }

        std::string file_content(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );

        try {
            json torrent = decode_bencoded_value(file_content);

            // Tracker URL
            std::cout << "Tracker URL: "
                      << torrent.at("announce").get<std::string>()
                      << "\n";

            // File length
            std::cout << "Length: "
                      << torrent.at("info").at("length").get<long long>()
                      << "\n";

            // Info hash
            std::string bencoded_info = encode_dict(torrent.at("info"));
            std::string info_hash = sha1_hex(bencoded_info);

            std::cout << "Info Hash: " << info_hash << "\n";

        } catch (const std::exception& e) {
            std::cerr << "Torrent parse error: " << e.what() << "\n";
            return 1;
        }
    }

    else {
        std::cerr << "Unknown command: " << command << "\n";
        return 1;
    }

    return 0;
}
