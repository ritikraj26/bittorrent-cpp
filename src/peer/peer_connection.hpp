#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;


void setup_tcp_connection(
    const std::string& peer_ip,
    int port,
    const std::string& info_hash,
    uint32_t piece_index,
    const std::string& output_file,
    const json& torrent
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