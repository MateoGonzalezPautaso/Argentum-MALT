#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>
#include <string>

#include "socket.h"

// TODO: Replace with full protocol definition.
enum class OpCode : uint8_t {
    Unknown = 0
};

class Protocol {
protected:
    Socket& skt;

    /* TODO
    uint32_t    recv_uint32();
    std::string recv_string();
    bool        recv_bool();

    hacer las versiones send tambien
    */

    void send_uint8(uint8_t value);
    uint8_t recv_uint8();

    void send_uint16(uint16_t value);
    uint16_t recv_uint16();

    void send_str(const std::string& message);
    std::string recv_str(uint16_t length);

    void send_opcode(OpCode opcode);
    OpCode recv_opcode();

public:
    explicit Protocol(Socket& skt);
    virtual ~Protocol() = default;

    // No copiable
    Protocol(const Protocol&) = delete;
    Protocol& operator=(const Protocol&) = delete;
};

#endif
