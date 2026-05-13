#include "server_protocol.h"

#include <stdexcept>

#include "../common/socket.h"

ServerProtocol::ServerProtocol(Socket&& skt):
        skt(std::move(skt)),
        protocol(skt) {}

ClientCommand ServerProtocol::recv_command() {
    OpCode opcode = protocol.recv_opcode();

    switch (opcode) {
        case OpCode::MOVE:
            return recv_move();
        default:
            throw std::runtime_error("Unknown command opcode: " + std::to_string(static_cast<int>(opcode)));
    }
}

ClientCommand ServerProtocol::recv_move() {
    Direction dir = static_cast<Direction>(protocol.recv_uint8());
    return MoveCmd{dir};
}

/**
 * Creates a struct that inherits from all the lambdas passed to it. 
 * Each lambda has its own operator(), and using Ts::operator()...; 
 * brings all of them into scope. When std::visit calls operator() 
 * on the overloaded object, overload resolution picks the lambda whose parameter type matches.
 */
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
/**
 * Tells the compiler: "when you write overloaded{lambda1, lambda2, ...}, 
 * deduce the template arguments from the lambdas' types"
 */
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

void ServerProtocol::send_event(const ServerEvent&) {
    throw std::runtime_error("send_event not implemented");
}
