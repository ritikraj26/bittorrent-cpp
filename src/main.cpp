#include <iostream>
#include <string>
#include <unistd.h>

#include "lib/nlohmann/json.hpp"

#include "bencode/decode.hpp"

#include "utils/file_reader.hpp"
#include "utils/hex_utils.hpp"
#include "utils/parse_magnet.hpp"
#include "utils/url_utils.hpp"
#include "utils/random.hpp"

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
        else if (command == "magnet_parse") {
            std::string magnet_link = argv[2];

            json parsed = parse_magnet(magnet_link);

            std::cout << "Info Hash: " <<
                    parsed.at("info_hash").get<std::string>()
                    << "\n";

            std::cout << "Tracker URL: " <<
                    url_decode(parsed.at("tracker_url").get<std::string>())
                    << "\n";
        }
        else if (command == "magnet_handshake") {
            std::string magnet_link = argv[2];

            json parsed = parse_magnet(magnet_link);

            std::string tracker_url = url_decode(parsed.at("tracker_url").get<std::string>());
            std::string info_hash = hex_to_bytes(parsed.at("info_hash").get<std::string>());
            std::string peer_id = generate_peer_id(20);

            std::string peers_blob = request_peers(tracker_url, info_hash, peer_id);

            auto peers = parse_peers(peers_blob);

            auto pos = peers[0].find(':');

            std::string ip = peers[0].substr(0, pos);
            int port = std::stoi(peers[0].substr(pos + 1));

            auto [received_peer_id, peer_extension_id] = perform_extension_handshake(ip, port, peer_id, info_hash);

            std::cout << "Peer ID: " << bytes_to_hex(received_peer_id) << "\n";
            std::cout << "Peer Metadata Extension ID: " << static_cast<int>(peer_extension_id) << "\n";
        }
        else if (command == "magnet_info") {
            std::string magnet_link = argv[2];

            json parsed = parse_magnet(magnet_link);

            std::string tracker_url = url_decode(parsed.at("tracker_url").get<std::string>());
            std::string info_hash_hex = parsed.at("info_hash").get<std::string>();
            std::string info_hash = hex_to_bytes(info_hash_hex);
            std::string peer_id = generate_peer_id(20);

            std::string peers_blob = request_peers(tracker_url, info_hash, peer_id);
            auto peers = parse_peers(peers_blob);

            auto pos = peers[0].find(':');
            std::string ip = peers[0].substr(0, pos);
            int port = std::stoi(peers[0].substr(pos + 1));

            // Setup connection with extension support
            uint8_t peer_extension_id;
            int sock = setup_metadata_connection(ip, port, peer_id, info_hash, peer_extension_id);

            // Request metadata piece 0
            send_metadata_request(sock, peer_extension_id, 0);

            // Receive metadata response
            std::string metadata = receive_metadata_response(sock);

            close(sock);

            // Parse the metadata as a torrent info dictionary
            json info_dict = decode_bencoded_value(metadata);

            // Display the information
            std::cout << "Tracker URL: " << tracker_url << "\n";
            std::cout << "Length: " << info_dict.at("length").get<int>() << "\n";
            std::cout << "Info Hash: " << info_hash_hex << "\n";
            std::cout << "Piece Length: " << info_dict.at("piece length").get<int>() << "\n";
            std::cout << "Piece Hashes:\n";

            std::string pieces_blob = info_dict.at("pieces").get<std::string>();
            auto hashes = extract_piece_hashes(pieces_blob);
            for (const auto& hash : hashes) {
                std::cout << bytes_to_hex(hash) << "\n";
            }
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

            std::string peer_id = generate_peer_id(20);

            std::string peers_blob = request_peers(torrent, peer_id);

            auto peers = parse_peers(peers_blob);

            auto pos = peers[0].find(':');

            std::string ip = peers[0].substr(0, pos);
            int port = std::stoi(peers[0].substr(pos + 1));

            download_piece(
                ip,
                port,
                info_hash,
                piece_index,
                output_file,
                torrent,
                peer_id
            );
        }
        else if (command == "download") {

            if (argc < 5)
                throw std::runtime_error(
                    "Usage: download -o <output_file> <torrent_file>"
                );

            if (std::string(argv[2]) != "-o")
                throw std::runtime_error("Expected -o flag");

            std::string output_file = argv[3];
            std::string torrent_file = argv[4];

            std::string torrent_content = read_file(torrent_file);
            json torrent = parse_torrent(torrent_content);

            std::string info_hash = compute_info_hash_raw(torrent);

            std::string peer_id = generate_peer_id(20);

            std::string peers_blob = request_peers(torrent, peer_id);
            auto peers = parse_peers(peers_blob);

            if (peers.empty())
                throw std::runtime_error("No peers found");

            auto pos = peers[0].find(':');
            std::string ip = peers[0].substr(0, pos);
            int port = std::stoi(peers[0].substr(pos + 1));

            download_file(ip, port, info_hash, output_file, torrent, peer_id);
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
                    std::cout << bytes_to_hex(h) << "\n";
            }

            else if (command == "peers") {

                std::string peer_id = generate_peer_id(20);

                std::string peers_blob = request_peers(torrent, peer_id);

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
                        << bytes_to_hex(peer_id)
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