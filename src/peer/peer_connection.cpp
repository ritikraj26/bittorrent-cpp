#include "peer/peer_connection.hpp"
#include "peer/handshake.hpp"

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "utils/random.hpp"
#include "utils/hex.hpp"

void setup_tcp_connection(const std::string& peer_ip,
                          int port,
                          const std::string& info_hash)
{
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

    std::cout << "Peer ID: "
              << to_hex(received_peer_id)
              << std::endl;

    close(sock);
}