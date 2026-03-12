#pragma once

#include <string>
#include <cstdint>

std::string perform_base_handshake(int sock,
                              const std::string& info_hash,
                              const std::string& peer_id);

std::string perform_base_handshake_with_extensions(int sock,
                              const std::string& info_hash,
                              const std::string& peer_id);

std::string perform_peer_handshake(
    const std::string& peer_ip,
    int port,
    const std::string& info_hash
);

void send_extension_handshake_message(int sock, uint8_t ut_metadata_id);

uint8_t receive_extension_handshake_message(int sock);