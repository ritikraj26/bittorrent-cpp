#include <iostream>
#include <string>

#include "lib/nlohmann/json.hpp"

#include "bencode/decode.hpp"

#include "utils/file_reader.hpp"
#include "utils/hex.hpp"

#include "torrent/torrent_parser.hpp"
#include "torrent/piece_hash.hpp"

#include "tracker/tracker_client.hpp"
#include "tracker/peer_parser.hpp"

#include "peer/peer_connection.hpp"
#include "peer/handshake.hpp"

using json = nlohmann::json;

int main(int argc, char* argv[]) {

    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    if (argc < 3) {
        std::cerr << "Usage:\n";
        std::cerr << argv[0] << " decode <value>\n";
        std::cerr << argv[0] << " info <torrent>\n";
        std::cerr << argv[0] << " peers <torrent>\n";
        return 1;
    }

    std::string command = argv[1];

    try {

        if (command == "decode") {

            json decoded = decode_bencoded_value(argv[2]);
            std::cout << decoded.dump() << "\n";
        }
        else if (command == "download_piece") {

            if (argc < 6)
                throw std::runtime_error("Usage: download_piece -o <output> <torrent> <piece>");

            if (std::string(argv[2]) != "-o")
                throw std::runtime_error("Expected -o flag");

            std::string output_file = argv[3];
            std::string torrent_file = argv[4];
            int piece_index = std::stoi(argv[5]);

            std::string torrent_content = read_file(torrent_file);
            json torrent = parse_torrent(torrent_content);

            std::string info_hash = compute_info_hash_raw(torrent);

            std::string peers_blob = request_peers(torrent);

            auto peers = parse_peers(peers_blob);

            auto pos = peers[0].find(':');

            std::string ip = peers[0].substr(0, pos);
            int port = std::stoi(peers[0].substr(pos + 1));

            setup_tcp_connection(
                ip,
                port,
                info_hash,
                piece_index,
                output_file,
                torrent
            );
        }

        else {

            std::string content = read_file(argv[2]);
            json torrent = parse_torrent(content);

            if (command == "info") {

                std::cout << "Tracker URL: "
                          << get_tracker_url(torrent) << "\n";

                std::cout << "Length: "
                          << get_file_length(torrent) << "\n";

                std::cout << "Info Hash: "
                          << compute_info_hash_hex(torrent) << "\n";

                std::cout << "Piece Length: "
                          << get_piece_length(torrent) << "\n";

                auto hashes = extract_piece_hashes(
                    get_pieces_blob(torrent)
                );

                std::cout << "Piece Hashes:\n";

                for (auto& h : hashes)
                    std::cout << to_hex(h) << "\n";
            }

            else if (command == "peers") {

                std::string peers_blob = request_peers(torrent);

                print_peers(peers_blob);
            }

            else if (command == "handshake") {

                std::string torrent_content = read_file(argv[2]);
                json torrent = parse_torrent(torrent_content);

                std::string info_hash = compute_info_hash_raw(torrent);

                std::string peer = argv[3];

                auto pos = peer.find(':');

                std::string ip = peer.substr(0, pos);
                int port = std::stoi(peer.substr(pos + 1));

                std::string peer_id =
                    perform_peer_handshake(ip, port, info_hash);

                std::cout << "Peer ID: "
                        << to_hex(peer_id)
                        << "\n";
            }
            else {
                std::cerr << "Unknown command\n";
                return 1;
            }
        }

    } catch (const std::exception& e) {

        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}