#include "parse_magnet.hpp"

#include <string>
#include <stdexcept>
#include <cstddef>

#include "lib/nlohmann/json.hpp"

using json = nlohmann::json;

json parse_magnet(std::string magnet_link) {
    if (magnet_link.rfind("magnet:?", 0) != 0) {
        throw std::runtime_error("Invalid magnet link");
    }

    std::string query = magnet_link.substr(8);

    json result;

    size_t start = 0;
    while (start < query.size()) {

        size_t end = query.find('&', start);
        if (end == std::string::npos) {
            end = query.size();
        }

        std::string param = query.substr(start, end - start);
        size_t eq = param.find('=');

        if (eq == std::string::npos) {
            throw std::runtime_error("Invalid magnet parameter");
        }

        std::string key = param.substr(0, eq);
        std::string value = param.substr(eq + 1);

        if (key == "xt") {
            const std::string prefix = "urn:btih:";
            if (value.rfind(prefix, 0) != 0) {
                throw std::runtime_error("Invalid xt field");
            }

            result["info_hash"] = value.substr(prefix.size());
        }
        else if (key == "dn") {
            result["name"] = value;
        }
        else if (key == "tr") {
            // URL-decoding will be handled later if needed
            result["tracker_url"] = value;
        }

        start = end + 1;
    }

    return result;
}