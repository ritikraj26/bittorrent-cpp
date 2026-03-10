#pragma once

#include <string>

#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

json parse_magnet(std::string magnet_link);