#include "server_protocol.h"

#include <stdexcept>
#include <utility>

#include <sys/socket.h>

#include "../../common/socket.h"

ServerProtocol::ServerProtocol(Socket&& skt): skt(std::move(skt)), protocol(this->skt) {}

void ServerProtocol::shutdown() { skt.shutdown(SHUT_RDWR); }

ClientCommand ServerProtocol::recv_command() {
    OpCode opcode = protocol.recv_opcode();

    switch (opcode) {
        case OpCode::MOVE:
            return recv_move();
        case OpCode::LOGIN:
            return recv_login();
        case OpCode::CREATE_CHARACTER:
            return recv_create_character();
        case OpCode::ATTACK:
            return recv_attack();
        case OpCode::SEND_CHAT:
            return recv_send_chat_msg();
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
    cmd.player_class = static_cast<PlayerClass>(protocol.recv_uint8());
    return cmd;
}

ClientCommand ServerProtocol::recv_attack() {
    uint16_t target_id = protocol.recv_uint16();
    return AttackCmd{target_id};
}

ClientCommand ServerProtocol::recv_send_chat_msg() {
    SendChatMsgCmd cmd;
    cmd.text = protocol.recv_str();
    return cmd;
}

void ServerProtocol::send_login_payload(const LoginOkEvent& ev) {
    protocol.send_uint16(ev.player_id);
    protocol.send_str(ev.username);
    protocol.send_uint8(static_cast<uint8_t>(ev.race));
    protocol.send_uint8(static_cast<uint8_t>(ev.player_class));
    protocol.send_uint8(ev.level);
    protocol.send_uint32(ev.experience);
    protocol.send_uint32(ev.exp_to_next);
    protocol.send_uint32(ev.hp_current);
    protocol.send_uint32(ev.hp_max);
    protocol.send_uint32(ev.mana_current);
    protocol.send_uint32(ev.mana_max);
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

void ServerProtocol::send_entity_spawn(const EntitySpawnEvent& ev) {
    protocol.send_opcode(OpCode::ENTITY_SPAWN);
    protocol.send_uint16(ev.entity_id);
    protocol.send_uint8(static_cast<uint8_t>(ev.entity_type));
    protocol.send_uint16(ev.entity_pos.x);
    protocol.send_uint16(ev.entity_pos.y);
    protocol.send_uint8(static_cast<uint8_t>(ev.entity_dir));
    protocol.send_str(ev.entity_name);
    protocol.send_uint8(static_cast<uint8_t>(ev.entity_race));
    protocol.send_uint8(static_cast<uint8_t>(ev.entity_class));
}

void ServerProtocol::send_entity_despawn(const EntityDespawnEvent& ev) {
    protocol.send_opcode(OpCode::ENTITY_DESPAWN);
    protocol.send_uint16(ev.entity_id);
}

void ServerProtocol::send_entity_move(const EntityMoveEvent& ev) {
    protocol.send_opcode(OpCode::ENTITY_MOVE);
    protocol.send_uint16(ev.entity_id);
    protocol.send_uint16(ev.entity_pos.x);
    protocol.send_uint16(ev.entity_pos.y);
    protocol.send_uint8(static_cast<uint8_t>(ev.entity_dir));
}

void ServerProtocol::send_damage_dealt(const DamageDealtEvent& ev) {
    protocol.send_opcode(OpCode::DAMAGE_DEALT);
    protocol.send_uint16(ev.target_id);
    protocol.send_uint32(ev.damage);
}

void ServerProtocol::send_damage_received(const DamageReceivedEvent& ev) {
    protocol.send_opcode(OpCode::DAMAGE_RECEIVED);
    protocol.send_uint16(ev.attacker_id);
    protocol.send_uint32(ev.damage);
}

void ServerProtocol::send_entity_died(const EntityDiedEvent& ev) {
    protocol.send_opcode(OpCode::ENTITY_DIED);
    protocol.send_uint16(ev.entity_id);
}

void ServerProtocol::send_chat_msg(const ChatMsgEvent& ev) {
    protocol.send_opcode(OpCode::CHAT_MSG);
    protocol.send_uint8(static_cast<uint8_t>(ev.type));
    protocol.send_str(ev.sender_name);
    protocol.send_str(ev.message);
}

#include "../../common/visit.h"

void ServerProtocol::send_event(const ServerEvent& ev) {
    std::visit(overloaded{
                       [this](const LoginOkEvent& msg) { send_login_ok(msg); },
                       [this](const LoginErrorEvent& msg) { send_login_error(msg); },
                       [this](const CharacterCreatedEvent& msg) { send_character_created(msg); },
                       [this](const CharacterErrorEvent& msg) { send_character_error(msg); },
                       [this](const EntitySpawnEvent& msg) { send_entity_spawn(msg); },
                       [this](const EntityDespawnEvent& msg) { send_entity_despawn(msg); },
                       [this](const EntityMoveEvent& msg) { send_entity_move(msg); },
                       [this](const DamageDealtEvent& msg) { send_damage_dealt(msg); },
                       [this](const DamageReceivedEvent& msg) { send_damage_received(msg); },
                        [this](const EntityDiedEvent& msg) { send_entity_died(msg); },
                        [this](const ChatMsgEvent& msg) { send_chat_msg(msg); },
                        [](const auto&) { throw std::runtime_error("Event type not implemented"); },
               },
               ev);
}
