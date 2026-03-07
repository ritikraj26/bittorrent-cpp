#pragma once

#include <string>
#include "../lib/nlohmann/json.hpp"

using json = nlohmann::json;

std::string request_peers(const json& torrent);