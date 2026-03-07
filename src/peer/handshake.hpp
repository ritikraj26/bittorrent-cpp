#pragma once

#include <string>

std::string perform_handshake(int sock,
                              const std::string& info_hash,
                              const std::string& peer_id);