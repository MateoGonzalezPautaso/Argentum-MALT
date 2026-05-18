#include "client_protocol.h"

#include <stdexcept>
#include <utility>

#include "../common/socket.h"

ClientProtocol::ClientProtocol(Socket&& skt): skt(std::move(skt)), protocol(this->skt) {}

void ClientProtocol::send_login(const LoginCmd& cmd) {
    protocol.send_opcode(OpCode::LOGIN);
    protocol.send_str(cmd.username);
    protocol.send_str(cmd.password);
}

void ClientProtocol::send_create_character(const CreateCharacterCmd& cmd) {
    uint8_t race = static_cast<uint8_t>(cmd.race);
    uint8_t class_ = static_cast<uint8_t>(cmd.class_);

    protocol.send_opcode(OpCode::CREATE_CHARACTER);
    protocol.send_str(cmd.username);
    protocol.send_str(cmd.password);
    protocol.send_uint8(race);
    protocol.send_uint8(class_);
}

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
template <class... Ts>
struct overloaded: Ts... {
    using Ts::operator()...;
};
/**
 * Tells the compiler: "when you write overloaded{lambda1, lambda2, ...},
 * deduce the template arguments from the lambdas' types"
 */
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

void ClientProtocol::send_command(const ClientCommand& cmd) {
    std::visit(overloaded{
                       [this](const MoveCmd& msg) { send_move(msg); },
                       [this](const LoginCmd& msg) { send_login(msg); },
                       [this](const CreateCharacterCmd& msg) { send_create_character(msg); },
                       [](const auto&) { throw std::runtime_error("Command not implemented"); },
               },
               cmd);
}

ServerEvent ClientProtocol::recv_event() {
    OpCode opcode = protocol.recv_opcode();

    switch (opcode) {
        case OpCode::LOGIN_OK:
            return recv_login_ok();
        case OpCode::LOGIN_ERROR:
            return recv_login_error();
        case OpCode::CHARACTER_CREATED:
            return recv_character_created();
        case OpCode::CHARACTER_ERROR:
            return recv_character_error();
        default:
            throw std::runtime_error("Unknown event opcode: " +
                                     std::to_string(static_cast<int>(opcode)));
    }
}

LoginOkEvent ClientProtocol::recv_login_payload() {
    LoginOkEvent ev;
    ev.player_id = protocol.recv_uint16();
    ev.username = protocol.recv_str();
    ev.race = static_cast<Race>(protocol.recv_uint8());
    ev.class_ = static_cast<Class>(protocol.recv_uint8());
    ev.level = protocol.recv_uint8();
    ev.experience = protocol.recv_uint32();
    ev.hp_current = protocol.recv_uint32();
    ev.hp_max = protocol.recv_uint32();
    ev.mana_current = protocol.recv_uint32();
    ev.mana_max = protocol.recv_uint32();
    ev.gold = protocol.recv_uint32();
    ev.pos.x = protocol.recv_uint16();
    ev.pos.y = protocol.recv_uint16();
    return ev;
}

ServerEvent ClientProtocol::recv_login_ok() { return recv_login_payload(); }

ServerEvent ClientProtocol::recv_login_error() {
    LoginErrorEvent ev;
    ev.error_code = static_cast<LoginError>(protocol.recv_uint8());
    ev.message = protocol.recv_str();
    return ev;
}

ServerEvent ClientProtocol::recv_character_created() {
    return CharacterCreatedEvent{recv_login_payload()};
}

ServerEvent ClientProtocol::recv_character_error() {
    CharacterErrorEvent ev;
    ev.error_code = static_cast<CharacterError>(protocol.recv_uint8());
    ev.message = protocol.recv_str();
    return ev;
}
