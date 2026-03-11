#include "peer/handshake.hpp"

#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>

#include "utils/random.hpp"

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

std::string perform_extension_handshake(int sock, const std::string& info_hash, const std::string& peer_id) {
    std::vector<unsigned char> handshake(68);

    handshake[0] = 19;

    const std::string protocol = "BitTorrent protocol";
    std::memcpy(&handshake[1], protocol.c_str(), 19);

    std::memset(&handshake[20], 0, 8);
    handshake[20 + 5] |= 0x10;

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

std::string perform_peer_handshake(
    const std::string& peer_ip,
    int port,
    const std::string& info_hash
) {

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        throw std::runtime_error("Socket creation failed");
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, peer_ip.c_str(),
                  &server_addr.sin_addr) <= 0) {

        close(sock);
        throw std::runtime_error("Invalid IP address");
    }

    if (connect(sock,
        (sockaddr*)&server_addr,
        sizeof(server_addr)) < 0) {

        close(sock);
        throw std::runtime_error("Connection failed");
    }

    std::string peer_id = generate_peer_id(20);

    std::string received_peer_id =
        perform_handshake(sock, info_hash, peer_id);

    close(sock);

    return received_peer_id;
}