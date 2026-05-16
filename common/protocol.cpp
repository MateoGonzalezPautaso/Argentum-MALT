#include "protocol.h"

#include <stdexcept>
#include <vector>

#include <arpa/inet.h>

Protocol::Protocol(Socket& skt): skt(skt) {}

void Protocol::send_uint8(uint8_t value) { skt.sendall(&value, sizeof(value)); }

uint8_t Protocol::recv_uint8() {
    uint8_t value;
    int bytes = this->skt.recvall(&value, sizeof(value));

    if (bytes <= 0)
        throw std::runtime_error("Closed socket or error while receiving uint8_t");

    return value;
}

void Protocol::send_uint16(uint16_t value) {
    uint16_t net_value = htons(value);  // Host to network
    this->skt.sendall(&net_value, sizeof(net_value));
}

uint16_t Protocol::recv_uint16() {
    uint16_t net_value;
    int bytes = this->skt.recvall(&net_value, sizeof(net_value));

    if (bytes <= 0)
        throw std::runtime_error("Closed socket or error while receiving uint16_t");

    return ntohs(net_value);  // Network to host
}

void Protocol::send_str(const std::string& message) {
    uint16_t len_str = static_cast<uint16_t>(message.length());
    send_uint16(len_str);
    this->skt.sendall(message.c_str(), message.length());
}

std::string Protocol::recv_str() {
    uint16_t length = recv_uint16();
    if (length == 0)
        return "";

    // saves every byte, does not care if they are printable or null
    // it is binary protocol, not text protocol ending with \n
    std::vector<char> buffer(length);
    int bytes = this->skt.recvall(buffer.data(), length);

    if (bytes <= 0)
        throw std::runtime_error("Closed socket or error while receiving string");

    return std::string(buffer.begin(), buffer.end());
}

void Protocol::send_opcode(OpCode opcode) { send_uint8(static_cast<uint8_t>(opcode)); }

OpCode Protocol::recv_opcode() {
    uint8_t opcode_byte = recv_uint8();
    return static_cast<OpCode>(opcode_byte);
}

void Protocol::send_uint32(uint32_t value) {
    uint32_t net_value = htonl(value);
    this->skt.sendall(&net_value, sizeof(net_value));
}

uint32_t Protocol::recv_uint32() {
    uint32_t net_value;
    int bytes = this->skt.recvall(&net_value, sizeof(net_value));

    if (bytes <= 0)
        throw std::runtime_error("Closed socket or error while receiving uint32_t");

    return ntohl(net_value);  // Network to host
}

void Protocol::send_bool(bool value) { this->skt.sendall(&value, sizeof(value)); }

bool Protocol::recv_bool() {
    bool value;
    int bytes = this->skt.recvall(&value, sizeof(value));

    if (bytes <= 0)
        throw std::runtime_error("Closed socket or error while receiving bool");

    return value;
}
