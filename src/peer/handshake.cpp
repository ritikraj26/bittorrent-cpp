#include "peer/handshake.hpp"

#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <stdexcept>

std::string perform_handshake(int sock,
                              const std::string& info_hash,
                              const std::string& peer_id)
{
    std::vector<unsigned char> handshake(68);

    handshake[0] = 19;

    const std::string protocol = "BitTorrent protocol";
    std::memcpy(&handshake[1], protocol.c_str(), 19);

    std::memset(&handshake[20], 0, 8);

    std::memcpy(&handshake[28], info_hash.data(), 20);

    std::memcpy(&handshake[48], peer_id.data(), 20);

    send(sock, handshake.data(), handshake.size(), 0);

    unsigned char response[68];

    int received = recv(sock, response, 68, MSG_WAITALL);

    if (received != 68) {
        throw std::runtime_error("Invalid handshake response");
    }

    return std::string((char*)&response[48], 20);
}