#include "peer/peer_connection.hpp"
#include "peer/handshake.hpp"

#include <iostream>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "utils/random.hpp"
#include "utils/hex.hpp"

#include "torrent/torrent_parser.hpp"
#include "torrent/piece_hash.hpp"

#include <openssl/sha.h>

#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;


void read_exact(int sock, void* buffer, size_t size) {
    size_t total_read = 0;
    char* buf = static_cast<char*>(buffer);

    while (total_read < size) {
        ssize_t bytes = recv(sock,
                            buf + total_read,
                            size - total_read,
                            0);
        if (bytes <= 0) {
            throw std::runtime_error("Socket read failed");
        }

        total_read += bytes;
    }
}
std::vector<uint8_t> receive_bitfield(int sock) {
    uint32_t length;

    // Read message length, 4 bytes
    read_exact(sock, &length, 4);
    length = ntohl(length);

    if (length == 0) {
        throw std::runtime_error("Received keep-alive instead of bitfield");
    }

    uint8_t message_id;

    // Read message ID, 1 byte
    read_exact(sock, &message_id, 1);

    // id is 5 for bitfield
    if (message_id != 5) {
        throw std::runtime_error("Expected bitfield message");
    }

    size_t payload_size = length - 1;
    std::vector<uint8_t> bitfield(payload_size);

    // bitfield.data() return a pointer to the first element of the vector
    read_exact(sock, bitfield.data(), payload_size);

    std::cout << "Received bitfield ("
              << bitfield.size()
              << " bytes)" << std::endl;

    return bitfield;
}


void send_interested(int sock) {
    uint8_t message[5];

    // htonl converts from host byte order to network byte order [example :small endian to big endian]
    uint32_t length = htonl(1);

    memcpy(message, &length, 4);

    //setting message id for interested
    message[4] = 2;

    ssize_t sent = send(sock, message, 5, 0);

    if (sent != 5) {
        throw std::runtime_error("Failed to send interested message");
    }
}

void wait_for_unchoke(int sock) {
    while (true) {
        uint32_t length;

        read_exact(sock, &length, 4);

        // ntohl converts from network byte order to host byte order
        length = ntohl(length);

        // keep alive message, length = 0
        if (length == 0) {
            continue;
        }

        uint8_t message_id;

        read_exact(sock, &message_id, 1);

        // If peer unchoked us
        if (message_id == 1) {
            std::cout << "Peer unchoked us" << std::endl;
            return;
        }

        // Skip remaining payload if message has extra data
        int payload_size = length - 1;

        if (payload_size > 0) {
            std::vector<uint8_t> skip(payload_size);

            read_exact(sock, skip.data(), payload_size);
        }
    }
}

void request_block(
    int sock,
    uint32_t piece_index,
    uint32_t begin,
    uint32_t block_length) {
    // Request message is always 17 bytes
    // 4 bytes length
    // 1 byte message ID
    // 12 bytes payload
    uint8_t message[17];

    // Length = message ID (1 byte) + payload (12 bytes)
    uint32_t length = htonl(13);

    // Convert integers to network byte order
    uint32_t piece = htonl(piece_index);
    uint32_t offset = htonl(begin);
    uint32_t block = htonl(block_length);

    memcpy(message, &length, 4);

    // Message ID for request = 6
    message[4] = 6;

    // Copy payload fields
    memcpy(message + 5, &piece, 4);     // piece index
    memcpy(message + 9, &offset, 4);    // begin offset
    memcpy(message + 13, &block, 4);    // block length

    // Send the full message
    ssize_t sent = send(sock, message, 17, 0);

    if (sent != 17) {
        throw std::runtime_error("Failed to send request message");
    }

    std::cout << "Requested block: piece "
              << piece_index
              << " offset "
              << begin
              << " length "
              << block_length
              << std::endl;
}

std::vector<uint8_t> receive_piece_block(int sock) {

    uint32_t length;

    // Read the message length (4 bytes)
    read_exact(sock, &length, 4);

    // Convert from network byte order → host byte order
    length = ntohl(length);

    uint8_t message_id;

    // Read message id
    read_exact(sock, &message_id, 1);

    // Piece message id must be 7
    if (message_id != 7) {
        throw std::runtime_error("Expected piece message");
    }

    uint32_t piece_index;
    uint32_t begin;

    // Read which piece this block belongs to
    read_exact(sock, &piece_index, 4);

    // Read offset inside the piece
    read_exact(sock, &begin, 4);

    piece_index = ntohl(piece_index);
    begin = ntohl(begin);

    // Remaining bytes are the block data
    uint32_t block_size = length - 9;

    std::vector<uint8_t> block(block_size);

    read_exact(sock, block.data(), block_size);

    std::cout << "Received block: piece "
              << piece_index
              << " offset "
              << begin
              << " size "
              << block_size
              << std::endl;

    return block;
}

void setup_tcp_connection(const std::string& peer_ip,
                          int port,
                          const std::string& info_hash,
                          u_int32_t piece_index,
                          const std::string& output_file,
                          const json& torrent)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        throw std::runtime_error("Socket creation failed");
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, peer_ip.c_str(),
                  &server_addr.sin_addr) <= 0) {

        close(sock);
        throw std::runtime_error("Invalid IP address");
    }

    if (connect(sock,
        (sockaddr*)&server_addr,
        sizeof(server_addr)) < 0) {

        close(sock);
        throw std::runtime_error("Connection failed");
    }

    std::string peer_id = generate_peer_id(20);

    std::string received_peer_id =
        perform_handshake(sock, info_hash, peer_id);

    std::cout << "Peer ID: "
              << to_hex(received_peer_id)
              << std::endl;

    auto bitfield = receive_bitfield(sock);

    send_interested(sock);

    wait_for_unchoke(sock);

    // const uint32_t piece_index = 0;

    const uint32_t block_size = 16 * 1024;  // 16 KiB

    uint32_t piece_length = get_piece_length(torrent);
    uint32_t file_length = get_file_length(torrent);

    uint32_t total_pieces =
        (file_length + piece_length - 1) / piece_length;

    if (piece_index == total_pieces - 1) {
        piece_length =
            file_length - (piece_index * piece_length);
    }

    uint32_t offset = 0;

    std::vector<uint8_t> piece_data;

    while (offset < piece_length) {

        uint32_t current_block =
            std::min(block_size, piece_length - offset);

        request_block(sock, piece_index, offset, current_block);

        auto block = receive_piece_block(sock);

        piece_data.insert(
            piece_data.end(),
            block.begin(),
            block.end());

        offset += current_block;
    }

    std::cout << "Downloaded piece size: "
              << piece_data.size()
              << std::endl;

    std::ofstream out(output_file, std::ios::binary);
    out.write(reinterpret_cast<char*>(piece_data.data()), piece_data.size());
    out.close();

    close(sock);
}

void download_file(
    const std::string& peer_ip,
    int port,
    const std::string& info_hash,
    const std::string& output_file,
    const json& torrent
) {

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, peer_ip.c_str(), &server_addr.sin_addr);

    connect(sock, (sockaddr*)&server_addr, sizeof(server_addr));

    std::string peer_id = generate_peer_id(20);

    perform_handshake(sock, info_hash, peer_id);

    receive_bitfield(sock);

    send_interested(sock);

    wait_for_unchoke(sock);

    uint32_t piece_length = get_piece_length(torrent);
    uint32_t file_length = get_file_length(torrent);

    auto hashes = extract_piece_hashes(get_pieces_blob(torrent));

    uint32_t total_pieces = hashes.size();

    std::vector<uint8_t> file_data;

    for (uint32_t i = 0; i < total_pieces; i++) {

        auto piece = download_piece_from_peer(
            sock,
            i,
            torrent
        );

        unsigned char hash[20];
        SHA1(piece.data(), piece.size(), hash);

        if (memcmp(hash, hashes[i].data(), 20) != 0) {
            throw std::runtime_error("Piece hash mismatch");
        }

        file_data.insert(
            file_data.end(),
            piece.begin(),
            piece.end()
        );
    }

    std::ofstream out(output_file, std::ios::binary);

    out.write(
        reinterpret_cast<char*>(file_data.data()),
        file_data.size()
    );

    out.close();

    close(sock);
}

std::vector<uint8_t> download_piece_from_peer(
    int sock,
    uint32_t piece_index,
    const json& torrent
) {
    uint32_t piece_length = get_piece_length(torrent);
    uint32_t file_length = get_file_length(torrent);

    uint32_t total_pieces =
        (file_length + piece_length - 1) / piece_length;

    if (piece_index == total_pieces - 1)
        piece_length = file_length - piece_index * piece_length;

    const uint32_t block_size = 16 * 1024;

    uint32_t offset = 0;

    std::vector<uint8_t> piece_data;

    while (offset < piece_length) {

        uint32_t current_block =
            std::min(block_size, piece_length - offset);

        request_block(sock, piece_index, offset, current_block);

        auto block = receive_piece_block(sock);

        piece_data.insert(
            piece_data.end(),
            block.begin(),
            block.end());

        offset += current_block;
    }

    // close(sock);

    return piece_data;
}