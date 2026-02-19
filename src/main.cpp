#include <iostream>
#include <fstream>
#include <string>

#include "lib/nlohmann/json.hpp"
#include "bencode/decode.hpp"
#include "bencode/encode.hpp"
#include "crypto/sha1.hpp"

#include <vector>


using json = nlohmann::json;

std::string to_hex(const std::string &s) {
    static const char* hex = "0123456789abcdef";
    std::string out;
    for (unsigned char c : s) {
        out.push_back(hex[c >> 4]);
        out.push_back(hex[c & 0x0F]);
    }
    return out;
}



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

            std::cout << "Piece Length: " << torrent.at("info").at("piece length").get<long long>() << "\n";

            std::string piece_hashes_string = torrent.at("info").at("pieces").get<std::string>();
            std::vector<std::string> piece_hashes;

            if (piece_hashes_string.size() % 20 != 0) {
                throw std::runtime_error("Invalid pieces field length");
            }

            for(size_t i = 0; i < piece_hashes_string.size(); i += 20) {
                piece_hashes.push_back(piece_hashes_string.substr(i,20));
            }

            std::cout << "Piece Hashes: " << "\n";
            for(size_t i = 0; i < piece_hashes.size(); i++) {
                std::cout << to_hex(piece_hashes[i]) << "\n";
            }

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
