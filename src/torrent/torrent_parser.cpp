#include "torrent_parser.hpp"

#include "bencode/decode.hpp"
#include "bencode/encode.hpp"
#include "crypto/sha1.hpp"

json parse_torrent(const std::string& content) {
    return decode_bencoded_value(content);
}

std::string get_tracker_url(const json& torrent) {
    return torrent.at("announce").get<std::string>();
}

long long get_file_length(const json& torrent) {
    return torrent.at("info").at("length").get<long long>();
}

long long get_piece_length(const json& torrent) {
    return torrent.at("info").at("piece length").get<long long>();
}

std::string get_pieces_blob(const json& torrent) {
    return torrent.at("info").at("pieces").get<std::string>();
}

std::string compute_info_hash_raw(const json& torrent) {

    std::string bencoded_info = encode_dict(torrent.at("info"));

    return sha1_raw(bencoded_info);
}

std::string compute_info_hash_hex(const json& torrent) {

    std::string bencoded_info = encode_dict(torrent.at("info"));

    return sha1_hex(bencoded_info);
}