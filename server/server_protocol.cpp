#include "server_protocol.h"

#include <stdexcept>
#include <utility>

#include "../common/socket.h"

ServerProtocol::ServerProtocol(Socket&& skt): skt(std::move(skt)), protocol(this->skt) {}

ClientCommand ServerProtocol::recv_command() {
    OpCode opcode = protocol.recv_opcode();

    switch (opcode) {
        case OpCode::MOVE:
            return recv_move();
        case OpCode::LOGIN:
            return recv_login();
        case OpCode::CREATE_CHARACTER:
            return recv_create_character();
        default:
            throw std::runtime_error("Unknown command opcode: " +
                                     std::to_string(static_cast<int>(opcode)));
    }
}

ClientCommand ServerProtocol::recv_move() {
    Direction dir = static_cast<Direction>(protocol.recv_uint8());
    return MoveCmd{dir};
}

ClientCommand ServerProtocol::recv_login() {
    LoginCmd cmd;
    cmd.username = protocol.recv_str();
    cmd.password = protocol.recv_str();
    return cmd;
}

ClientCommand ServerProtocol::recv_create_character() {
    CreateCharacterCmd cmd;
    cmd.username = protocol.recv_str();
    cmd.password = protocol.recv_str();
    cmd.race = static_cast<Race>(protocol.recv_uint8());
    cmd.class_ = static_cast<Class>(protocol.recv_uint8());
    return cmd;
}

void ServerProtocol::send_login_payload(const LoginOkEvent& ev) {
    protocol.send_uint16(ev.player_id);
    protocol.send_str(ev.username);
    protocol.send_uint8(static_cast<uint8_t>(ev.race));
    protocol.send_uint8(static_cast<uint8_t>(ev.class_));
    protocol.send_uint8(ev.level);
    protocol.send_uint32(ev.experience);
    protocol.send_uint16(ev.hp_current);
    protocol.send_uint16(ev.hp_max);
    protocol.send_uint16(ev.mana_current);
    protocol.send_uint16(ev.mana_max);
    protocol.send_uint32(ev.gold);
    protocol.send_uint16(ev.pos.x);
    protocol.send_uint16(ev.pos.y);
}

void ServerProtocol::send_login_ok(const LoginOkEvent& ev) {
    protocol.send_opcode(OpCode::LOGIN_OK);
    send_login_payload(ev);
}

void ServerProtocol::send_login_error(const LoginErrorEvent& ev) {
    protocol.send_opcode(OpCode::LOGIN_ERROR);
    protocol.send_uint8(static_cast<uint8_t>(ev.error_code));
    protocol.send_str(ev.message);
}

void ServerProtocol::send_character_created(const CharacterCreatedEvent& ev) {
    protocol.send_opcode(OpCode::CHARACTER_CREATED);
    send_login_payload(ev.data);
}

void ServerProtocol::send_character_error(const CharacterErrorEvent& ev) {
    protocol.send_opcode(OpCode::CHARACTER_ERROR);
    protocol.send_uint8(static_cast<uint8_t>(ev.error_code));
    protocol.send_str(ev.message);
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

void ServerProtocol::send_event(const ServerEvent& ev) {
    std::visit(overloaded{
                       [this](const LoginOkEvent& msg) { send_login_ok(msg); },
                       [this](const LoginErrorEvent& msg) { send_login_error(msg); },
                       [this](const CharacterCreatedEvent& msg) { send_character_created(msg); },
                       [this](const CharacterErrorEvent& msg) { send_character_error(msg); },
                       [](const auto&) {
                           throw std::runtime_error("Event type not implemented");
                       },
               },
               ev);
}
