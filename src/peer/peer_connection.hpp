#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <utility>

#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

// Low-level TCP socket connection
int establish_tcp_connection(const std::string& peer_ip, int port);

// Performs complete extension protocol handshake, returns pair of <peer_id, ut_metadata_extension_id>
std::pair<std::string, uint8_t> perform_extension_handshake(const std::string& peer_ip, int port, const std::string& peer_id, const std::string& info_hash);

// Downloads metadata from peer (complete flow: connect, handshake, request, receive)
std::string download_metadata_from_peer(const std::string& peer_ip, int port, const std::string& peer_id, const std::string& info_hash);

// Returns socket and sets peer_extension_id to the peer's ut_metadata extension ID
int setup_metadata_connection(const std::string& peer_ip, int port, 
                              const std::string& peer_id, 
                              const std::string& info_hash,
                              uint8_t& peer_extension_id);

void download_piece(
    const std::string& peer_ip,
    int port,
    const std::string& info_hash,
    uint32_t piece_index,
    const std::string& output_file,
    const json& torrent,
    const std::string& peer_id
);

std::vector<uint8_t> receive_bitfield(int sock);

void read_exact(int sock, void* buffer, size_t size);

void send_interested(int sock);

void wait_for_unchoke(int sock);

void request_block(
    int sock,
    uint32_t piece_index,
    uint32_t begin,
    uint32_t block_length
);

std::vector<uint8_t> receive_piece_block(int sock);

void download_file(
    const std::string& peer_ip,
    int port,
    const std::string& info_hash,
    const std::string& output_file,
    const json& torrent,
    const std::string& peer_id
);

std::vector<uint8_t> download_piece_from_peer(
    int sock,
    uint32_t piece_index,
    const json& torrent
);