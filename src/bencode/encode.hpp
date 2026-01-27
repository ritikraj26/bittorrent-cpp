#pragma once

#include <string>
#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

std::string encode_bencoded_value(const json& value);

std::string encode_dict(const json& dict);

