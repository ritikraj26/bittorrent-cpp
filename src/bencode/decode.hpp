#pragma once

#include <string>
#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

json decode_bencoded_value(const std::string& encoded);
