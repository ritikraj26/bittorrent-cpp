#pragma once

#include <string>

void setup_tcp_connection(const std::string& peer_ip,
                          int port,
                          const std::string& info_hash);