#include "client_protocol.h"

#include <stdexcept>
#include <utility>

#include <sys/socket.h>

#include "../../common/socket.h"

ClientProtocol::ClientProtocol(Socket&& skt): skt(std::move(skt)), protocol(this->skt) {}

void ClientProtocol::shutdown() { skt.shutdown(SHUT_RDWR); }

void ClientProtocol::send_login(const LoginCmd& cmd) {
    protocol.send_opcode(OpCode::LOGIN);
    protocol.send_str(cmd.username);
    protocol.send_str(cmd.password);
}

void ClientProtocol::send_create_character(const CreateCharacterCmd& cmd) {
    uint8_t race = static_cast<uint8_t>(cmd.race);
    uint8_t player_class = static_cast<uint8_t>(cmd.player_class);

    protocol.send_opcode(OpCode::CREATE_CHARACTER);
    protocol.send_str(cmd.username);
    protocol.send_str(cmd.password);
    protocol.send_uint8(race);
    protocol.send_uint8(player_class);
}

void ClientProtocol::send_move(const MoveCmd& cmd) {
    protocol.send_opcode(OpCode::MOVE);
    protocol.send_uint8(static_cast<uint8_t>(cmd.direction));
}

void ClientProtocol::send_attack(const AttackCmd& cmd) {
    protocol.send_opcode(OpCode::ATTACK);
    protocol.send_uint16(cmd.target_id);
}

void ClientProtocol::send_chat_msg(const SendChatMsgCmd& cmd) {
    protocol.send_opcode(OpCode::SEND_CHAT);
    protocol.send_str(cmd.text);
}

#include "../../common/visit.h"

void ClientProtocol::send_meditate() { protocol.send_opcode(OpCode::MEDITATE); }

void ClientProtocol::send_resurrect() { protocol.send_opcode(OpCode::RESURRECT); }

void ClientProtocol::send_cheat_infinite_hp() {
    protocol.send_opcode(OpCode::CHEAT_INFINITE_HP);
}

void ClientProtocol::send_cheat_infinite_mana() {
    protocol.send_opcode(OpCode::CHEAT_INFINITE_MANA);
}

void ClientProtocol::send_cheat_die() { protocol.send_opcode(OpCode::CHEAT_DIE); }

void ClientProtocol::send_cheat_level_up() { protocol.send_opcode(OpCode::CHEAT_LEVEL_UP); }

void ClientProtocol::send_cheat_level_down() { protocol.send_opcode(OpCode::CHEAT_LEVEL_DOWN); }

void ClientProtocol::send_command(const ClientCommand& cmd) {
    std::visit(overloaded{
                       [this](const MoveCmd& msg) { send_move(msg); },
                       [this](const LoginCmd& msg) { send_login(msg); },
                       [this](const CreateCharacterCmd& msg) { send_create_character(msg); },
                       [this](const AttackCmd& msg) { send_attack(msg); },
                       [this](const SendChatMsgCmd& msg) { send_chat_msg(msg); },
                       [this](const MeditateCmd&) { send_meditate(); },
                       [this](const ResurrectCmd&) { send_resurrect(); },
                       [this](const CheatInfiniteHpCmd&) { send_cheat_infinite_hp(); },
                       [this](const CheatInfiniteManaCmd&) { send_cheat_infinite_mana(); },
                       [this](const CheatDieCmd&) { send_cheat_die(); },
                       [this](const CheatLevelUpCmd&) { send_cheat_level_up(); },
                       [this](const CheatLevelDownCmd&) { send_cheat_level_down(); },
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
        case OpCode::ENTITY_SPAWN:
            return recv_entity_spawn();
        case OpCode::ENTITY_DESPAWN:
            return recv_entity_despawn();
        case OpCode::ENTITY_MOVE:
            return recv_entity_move();
        case OpCode::DAMAGE_DEALT: {
            uint16_t target_id = protocol.recv_uint16();
            uint32_t damage = protocol.recv_uint32();
            return DamageDealtEvent{target_id, damage};
        }
        case OpCode::DAMAGE_RECEIVED: {
            uint16_t target_id = protocol.recv_uint16();
            uint16_t attacker_id = protocol.recv_uint16();
            uint32_t damage = protocol.recv_uint32();
            uint32_t hp_current = protocol.recv_uint32();
            uint32_t hp_max = protocol.recv_uint32();
            return DamageReceivedEvent{target_id, attacker_id, damage, hp_current, hp_max};
        }
        case OpCode::ENTITY_DIED: {
            uint16_t entity_id = protocol.recv_uint16();
            return EntityDiedEvent{entity_id};
        }
        case OpCode::PLAYER_RESPAWNED: {
            uint16_t entity_id = protocol.recv_uint16();
            uint32_t hp_current = protocol.recv_uint32();
            uint32_t hp_max = protocol.recv_uint32();
            return PlayerRespawnedEvent{entity_id, hp_current, hp_max};
        }
        case OpCode::MEDITATION_START:
            return MeditationStartEvent{};
        case OpCode::MEDITATION_STOP:
            return MeditationStopEvent{};
        case OpCode::CHAT_MSG: {
            ChatMsgType type = static_cast<ChatMsgType>(protocol.recv_uint8());
            std::string sender = protocol.recv_str();
            std::string message = protocol.recv_str();
            uint16_t recipient_id = protocol.recv_uint16();
            uint16_t sender_id = protocol.recv_uint16();
            return ChatMsgEvent{type, sender, message, recipient_id, sender_id};
        }
        case OpCode::CLAN_NOTIFICATION: {
            ClanNotifType type = static_cast<ClanNotifType>(protocol.recv_uint8());
            std::string username = protocol.recv_str();
            std::string clan_name = protocol.recv_str();
            return ClanNotificationEvent{type, username, clan_name};
        }
        case OpCode::CLAN_UPDATE: {
            ClanUpdateEvent ev;
            ev.clan_name = protocol.recv_str();
            uint8_t count = protocol.recv_uint8();
            ev.members.resize(count);
            for (uint8_t i = 0; i < count; ++i) {
                ev.members[i].username = protocol.recv_str();
                ev.members[i].is_founder = protocol.recv_bool();
                ev.members[i].is_online = protocol.recv_bool();
            }
            return ev;
        }
        default:
            throw std::runtime_error("Unknown event opcode: " +
                                     std::to_string(static_cast<int>(opcode)));
    }
}

ServerEvent ClientProtocol::recv_entity_spawn() {
    EntitySpawnEvent ev;
    ev.entity_id = protocol.recv_uint16();
    ev.entity_type = static_cast<EntityType>(protocol.recv_uint8());
    ev.entity_pos.x = protocol.recv_uint16();
    ev.entity_pos.y = protocol.recv_uint16();
    ev.entity_dir = static_cast<Direction>(protocol.recv_uint8());
    ev.entity_name = protocol.recv_str();
    ev.entity_race = static_cast<Race>(protocol.recv_uint8());
    ev.entity_class = static_cast<PlayerClass>(protocol.recv_uint8());
    return ev;
}

ServerEvent ClientProtocol::recv_entity_despawn() {
    EntityDespawnEvent ev;
    ev.entity_id = protocol.recv_uint16();
    return ev;
}

ServerEvent ClientProtocol::recv_entity_move() {
    EntityMoveEvent ev;
    ev.entity_id = protocol.recv_uint16();
    ev.entity_pos.x = protocol.recv_uint16();
    ev.entity_pos.y = protocol.recv_uint16();
    ev.entity_dir = static_cast<Direction>(protocol.recv_uint8());
    return ev;
}

LoginOkEvent ClientProtocol::recv_login_payload() {
    LoginOkEvent ev;
    ev.player_id = protocol.recv_uint16();
    ev.username = protocol.recv_str();
    ev.race = static_cast<Race>(protocol.recv_uint8());
    ev.player_class = static_cast<PlayerClass>(protocol.recv_uint8());
    ev.level = protocol.recv_uint8();
    ev.experience = protocol.recv_uint32();
    ev.exp_to_next = protocol.recv_uint32();
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
