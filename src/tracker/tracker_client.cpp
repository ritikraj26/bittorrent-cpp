#include "tracker_client.hpp"

#include <curl/curl.h>
#include <string>
#include <stdexcept>

#include "utils/url_utils.hpp"
#include "utils/random.hpp"

#include "torrent/torrent_parser.hpp"
#include "bencode/decode.hpp"

size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {

    size_t total = size * nmemb;

    std::string* response = static_cast<std::string*>(userp);

    response->append(static_cast<char*>(contents), total);

    return total;
}

std::string request_peers(const json& torrent) {

    std::string info_hash = compute_info_hash_raw(torrent);

    std::string url = get_tracker_url(torrent);

    url += "?info_hash=" + url_encode(info_hash);
    url += "&peer_id=" + url_encode(generate_peer_id(20));
    url += "&port=6881";
    url += "&uploaded=0";
    url += "&downloaded=0";
    url += "&left=" + std::to_string(get_file_length(torrent));
    url += "&compact=1";

    curl_global_init(CURL_GLOBAL_DEFAULT);

    CURL* curl = curl_easy_init();

    if (!curl) {
        throw std::runtime_error("curl init failed");
    }

    std::string tracker_response;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &tracker_response);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error(curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();

    json tracker = decode_bencoded_value(tracker_response);

    return tracker["peers"];
}

std::string request_peers(const std::string& tracker_url, const std::string& info_hash, const std::string& peer_id) {
    std::string url = tracker_url;

    url += "?info_hash=" + url_encode(info_hash);
    url += "&peer_id=" + url_encode(peer_id);
    url += "&port=6881";
    url += "&uploaded=0";
    url += "&downloaded=0";
    url += "&left=1";
    url += "&compact=1";

    curl_global_init(CURL_GLOBAL_DEFAULT);

    CURL* curl = curl_easy_init();

    if (!curl) {
        throw std::runtime_error("curl init failed");
    }

    std::string tracker_response;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &tracker_response);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error(curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();

    json tracker = decode_bencoded_value(tracker_response);

    return tracker["peers"];
}