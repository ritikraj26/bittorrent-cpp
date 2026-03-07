#pragma once

#include <string>
#include "../lib/nlohmann/json.hpp"

using json = nlohmann::json;

json parse_torrent(const std::string& content);

std::string get_tracker_url(const json& torrent);

long long get_file_length(const json& torrent);

long long get_piece_length(const json& torrent);

std::string get_pieces_blob(const json& torrent);

std::string compute_info_hash_raw(const json& torrent);

std::string compute_info_hash_hex(const json& torrent);