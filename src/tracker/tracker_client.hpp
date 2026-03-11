#pragma once

#include <string>
#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

std::string request_peers(const json& torrent);

std::string request_peers(const std::string& tracker_url, const std::string& info_hash, const std::string& peer_id);