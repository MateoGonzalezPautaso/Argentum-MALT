#include "client_protocol.h"

#include <stdexcept>

#include "../common/socket.h"

ClientProtocol::ClientProtocol(Socket&& skt):
        skt(std::move(skt)),
        protocol(skt) {}

void ClientProtocol::send_move(const MoveCmd& cmd) {
    protocol.send_opcode(OpCode::MOVE);
    protocol.send_uint8(static_cast<uint8_t>(cmd.direction));
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

void ClientProtocol::send_command(const ClientCommand& cmd) {
    std::visit(overloaded{
        [this](const MoveCmd& msg) { send_move(msg); },
        [](const auto&) { throw std::runtime_error("Command not implemented"); },
    }, cmd);
}

ServerEvent ClientProtocol::recv_event() {
    throw std::runtime_error("recv_event not implemented");
}
