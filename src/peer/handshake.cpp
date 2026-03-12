#include "peer/handshake.hpp"

#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>

#include "utils/random.hpp"
#include "bencode/encode.hpp"
#include "bencode/decode.hpp"
#include "lib/nlohmann/json.hpp"
#include "peer/peer_connection.hpp"

using json = nlohmann::json;

std::string perform_base_handshake(int sock,
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

std::string perform_base_handshake_with_extensions(int sock, const std::string& info_hash, const std::string& peer_id) {
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
    int sock = establish_tcp_connection(peer_ip, port);

    std::string peer_id = generate_peer_id(20);

    std::string received_peer_id =
        perform_base_handshake(sock, info_hash, peer_id);

    close(sock);

    return received_peer_id;
}

void send_extension_handshake_message(int sock, uint8_t ut_metadata_id) {
    // Build the extension handshake payload
    json extension_msg;
    extension_msg["m"]["ut_metadata"] = ut_metadata_id;

    // Encode the payload as bencode
    std::string bencoded_payload = encode_bencoded_value(extension_msg);

    // Calculate message length: message_id (1) + extension_id (1) + payload
    uint32_t payload_length = 1 + 1 + bencoded_payload.size();
    uint32_t length_network = htonl(payload_length);

    // Build the complete message
    std::vector<uint8_t> message;
    
    // Add 4-byte length prefix
    message.insert(message.end(), 
                   reinterpret_cast<uint8_t*>(&length_network),
                   reinterpret_cast<uint8_t*>(&length_network) + 4);
    
    // Add message ID (20 for extension protocol)
    message.push_back(20);
    
    // Add extension message ID (0 for handshake)
    message.push_back(0);
    
    // Add bencoded payload
    message.insert(message.end(), 
                   bencoded_payload.begin(), 
                   bencoded_payload.end());

    // Send the message
    ssize_t sent = send(sock, message.data(), message.size(), 0);
    
    if (sent != static_cast<ssize_t>(message.size())) {
        throw std::runtime_error("Failed to send extension handshake");
    }

    std::cout << "Sent extension handshake with ut_metadata ID: " 
              << static_cast<int>(ut_metadata_id) << std::endl;
}

uint8_t receive_extension_handshake_message(int sock) {
    // Read message length (4 bytes)
    uint32_t length;
    read_exact(sock, &length, 4);
    length = ntohl(length);

    if (length == 0) {
        throw std::runtime_error("Received keep-alive instead of extension handshake");
    }

    // Read message ID (1 byte)
    uint8_t message_id;
    read_exact(sock, &message_id, 1);

    if (message_id != 20) {
        throw std::runtime_error("Expected extension message (ID 20), got: " + 
                                std::to_string(message_id));
    }

    // Read extension message ID (1 byte)
    uint8_t extension_id;
    read_exact(sock, &extension_id, 1);

    if (extension_id != 0) {
        throw std::runtime_error("Expected extension handshake (ext ID 0), got: " + 
                                std::to_string(extension_id));
    }

    // Read the bencoded payload
    size_t payload_size = length - 2;  // Subtract message_id and extension_id
    std::vector<char> payload(payload_size);
    read_exact(sock, payload.data(), payload_size);

    // Decode the bencoded payload
    std::string payload_str(payload.begin(), payload.end());
    json extension_data = decode_bencoded_value(payload_str);

    // Try to log the extension handshake (may contain binary data)
    try {
        std::cout << "Received extension handshake: " 
                  << extension_data.dump() << std::endl;
    } catch (const std::exception&) {
        std::cout << "Received extension handshake (contains binary data)" << std::endl;
    }

    // Extract the ut_metadata extension ID from peer's response
    if (extension_data.contains("m") && 
        extension_data["m"].contains("ut_metadata")) {
        return static_cast<uint8_t>(extension_data["m"]["ut_metadata"].get<int>());
    }

    throw std::runtime_error("Peer does not support ut_metadata extension");
}

void send_metadata_request(int sock, uint8_t peer_extension_id, uint32_t piece_index) {
    // Build the metadata request payload
    json request_msg;
    request_msg["msg_type"] = 0; // 0 for request
    request_msg["piece"] = piece_index;

    // Encode the payload as bencode
    std::string bencoded_payload = encode_bencoded_value(request_msg);

    // Calculate message length: message_id (1) + extension_id (1) + payload
    uint32_t payload_length = 1 + 1 + bencoded_payload.size();
    uint32_t length_network = htonl(payload_length);

    // Build the complete message
    std::vector<uint8_t> message;
    
    // Add 4-byte length prefix
    message.insert(message.end(), 
                   reinterpret_cast<uint8_t*>(&length_network),
                   reinterpret_cast<uint8_t*>(&length_network) + 4);
    
    // Add message ID (20 for extension protocol)
    message.push_back(20);
    
    // Add extension message ID (peer_extension_id)
    message.push_back(peer_extension_id);
    
    // Add bencoded payload
    message.insert(message.end(), 
                   bencoded_payload.begin(), 
                   bencoded_payload.end());

    // Send the message
    ssize_t sent = send(sock, message.data(), message.size(), 0);
    
    if (sent != static_cast<ssize_t>(message.size())) {
        throw std::runtime_error("Failed to send metadata request");
    }

    std::cout << "Sent metadata request for piece index: " 
              << piece_index << std::endl;
}

std::string receive_metadata_response(int sock) {
    // Read message length (4 bytes)
    uint32_t length;
    read_exact(sock, &length, 4);
    length = ntohl(length);

    if (length == 0) {
        throw std::runtime_error("Received keep-alive instead of metadata response");
    }

    // Read message ID (1 byte)
    uint8_t message_id;
    read_exact(sock, &message_id, 1);

    if (message_id != 20) {
        throw std::runtime_error("Expected extension message (ID 20), got: " + 
                                std::to_string(message_id));
    }

    // Read extension message ID (1 byte)
    uint8_t extension_id;
    read_exact(sock, &extension_id, 1);

    // Read the remaining payload
    size_t remaining_size = length - 2;  // Subtract message_id and extension_id
    std::vector<char> payload(remaining_size);
    read_exact(sock, payload.data(), remaining_size);

    // The payload consists of a bencoded dictionary followed by the metadata
    std::string payload_str(payload.begin(), payload.end());
    
    // Decode the bencoded dictionary and get where it ends
    auto [metadata_info, dict_end] = decode_bencoded_value_with_position(payload_str);

    // Try to log the metadata response (may contain binary data)
    try {
        std::cout << "Received metadata response: " 
                  << metadata_info.dump() << std::endl;
    } catch (const std::exception&) {
        std::cout << "Received metadata response (contains binary data)" << std::endl;
    }

    // Check msg_type
    if (metadata_info.contains("msg_type")) {
        int msg_type = metadata_info["msg_type"].get<int>();
        if (msg_type == 2) {
            throw std::runtime_error("Peer rejected metadata request");
        }
        if (msg_type != 1) {
            throw std::runtime_error("Unexpected msg_type: " + std::to_string(msg_type));
        }
    }

    // Extract the metadata (everything after the dictionary)
    std::string metadata = payload_str.substr(dict_end);
    
    return metadata;
}
