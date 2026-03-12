#pragma once

#include <string>
#include <utility>
#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

json decode_bencoded_value(const std::string& encoded);

// Returns decoded value and the position where decoding ended
std::pair<json, size_t> decode_bencoded_value_with_position(const std::string& encoded);
