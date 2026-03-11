#pragma once

#include <string>

std::string perform_handshake(int sock,
                              const std::string& info_hash,
                              const std::string& peer_id);

std::string perform_extension_handshake(int sock,
                              const std::string& info_hash,
                              const std::string& peer_id);

std::string perform_peer_handshake(
    const std::string& peer_ip,
    int port,
    const std::string& info_hash
);