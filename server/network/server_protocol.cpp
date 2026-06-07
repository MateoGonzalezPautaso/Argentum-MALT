#include "server_protocol.h"

#include <stdexcept>
#include <utility>

#include <sys/socket.h>

#include "../../common/socket.h"
#include "../../common/visit.h"


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
        case OpCode::MEDITATE:
            return MeditateCmd{};
        case OpCode::RESURRECT:
            return ResurrectCmd{};
        case OpCode::NPC_HEAL:
            return NpcHealCmd{};
        case OpCode::CAST_SPELL:
            return recv_cast_spell();
        case OpCode::CHEAT_INFINITE_HP:
            return CheatInfiniteHpCmd{};
        case OpCode::CHEAT_INFINITE_MANA:
            return CheatInfiniteManaCmd{};
        case OpCode::CHEAT_DIE:
            return CheatDieCmd{};
        case OpCode::CHEAT_LEVEL_UP:
            return CheatLevelUpCmd{};
        case OpCode::CHEAT_LEVEL_DOWN:
            return CheatLevelDownCmd{};
        case OpCode::CHEAT_ADD_GOLD:
            return CheatAddGoldCmd{};
        case OpCode::CHEAT_RESET_GOLD:
            return CheatResetGoldCmd{};
        case OpCode::CHEAT_VELOCITY:
            return CheatVelocityCmd{};
        case OpCode::CHEAT_REVIVE:
            return CheatReviveCmd{};
        case OpCode::CHEAT_FILL_INVENTORY:
            return CheatFillInventoryCmd{};
        case OpCode::CHEAT_CLEAR_INVENTORY:
            return CheatClearInventoryCmd{};
        case OpCode::CHEAT_RESET_MANA:
            return CheatResetManaCmd{};
        case OpCode::CHANGE_MAP:
            return recv_change_map();
        case OpCode::EQUIP_ITEM:
            return recv_equip_item();
        case OpCode::UNEQUIP_ITEM:
            return recv_unequip_item();
        default:
            throw std::runtime_error("Unknown command opcode: " +
                                     std::to_string(static_cast<int>(opcode)));
    }
}

ClientCommand ServerProtocol::recv_cast_spell() {
    uint16_t target_id = protocol.recv_uint16();
    return CastSpellCmd{target_id};
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

ClientCommand ServerProtocol::recv_change_map() {
    ChangeMapCmd cmd;
    cmd.prop_name = protocol.recv_str();
    return cmd;
}

ClientCommand ServerProtocol::recv_equip_item() {
    uint8_t slot_index = protocol.recv_uint8();
    return EquipItemCmd{slot_index};
}

ClientCommand ServerProtocol::recv_unequip_item() {
    EquipSlot slot = static_cast<EquipSlot>(protocol.recv_uint8());
    return UnequipItemCmd{slot};
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
    protocol.send_uint8(static_cast<uint8_t>(ev.weapon_type));
    protocol.send_uint8(static_cast<uint8_t>(ev.armor_type));
    protocol.send_uint8(static_cast<uint8_t>(ev.helmet_type));
    protocol.send_uint8(static_cast<uint8_t>(ev.shield_type));
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

void ServerProtocol::send_attack_dodged(const AttackDodgedEvent& ev) {
    protocol.send_opcode(OpCode::ATTACK_DODGED);
    protocol.send_uint16(ev.player_id);
}

void ServerProtocol::send_damage_received(const DamageReceivedEvent& ev) {
    protocol.send_opcode(OpCode::DAMAGE_RECEIVED);
    protocol.send_uint16(ev.target_id);
    protocol.send_uint16(ev.attacker_id);
    protocol.send_uint32(ev.damage);
    protocol.send_uint32(ev.hp_current);
    protocol.send_uint32(ev.hp_max);
}

void ServerProtocol::send_entity_died(const EntityDiedEvent& ev) {
    protocol.send_opcode(OpCode::ENTITY_DIED);
    protocol.send_uint16(ev.entity_id);
}

void ServerProtocol::send_player_respawned(const PlayerRespawnedEvent& ev) {
    protocol.send_opcode(OpCode::PLAYER_RESPAWNED);
    protocol.send_uint16(ev.entity_id);
    protocol.send_uint32(ev.hp_current);
    protocol.send_uint32(ev.hp_max);
}

void ServerProtocol::send_meditation_start() { protocol.send_opcode(OpCode::MEDITATION_START); }

void ServerProtocol::send_meditation_stop() { protocol.send_opcode(OpCode::MEDITATION_STOP); }

void ServerProtocol::send_chat_msg(const ChatMsgEvent& ev) {
    protocol.send_opcode(OpCode::CHAT_MSG);
    protocol.send_uint8(static_cast<uint8_t>(ev.type));
    protocol.send_str(ev.sender_name);
    protocol.send_str(ev.message);
    protocol.send_uint16(ev.recipient_id);
    protocol.send_uint16(ev.sender_id);
}

void ServerProtocol::send_clan_notification(const ClanNotificationEvent& ev) {
    protocol.send_opcode(OpCode::CLAN_NOTIFICATION);
    protocol.send_uint8(static_cast<uint8_t>(ev.type));
    protocol.send_str(ev.username);
    protocol.send_str(ev.clan_name);
}

void ServerProtocol::send_map_transition(const MapTransitionEvent& ev) {
    protocol.send_opcode(OpCode::MAP_TRANSITION);
    protocol.send_str(ev.map_name);
    protocol.send_uint16(ev.pos_x);
    protocol.send_uint16(ev.pos_y);
}

void ServerProtocol::send_clan_update(const ClanUpdateEvent& ev) {
    protocol.send_opcode(OpCode::CLAN_UPDATE);
    protocol.send_str(ev.clan_name);
    uint8_t count = static_cast<uint8_t>(ev.members.size());
    protocol.send_uint8(count);
    for (const auto& m: ev.members) {
        protocol.send_str(m.username);
        protocol.send_bool(m.is_founder);
        protocol.send_bool(m.is_online);
    }
}

void ServerProtocol::send_player_stats(const PlayerStatsEvent& ev) {
    protocol.send_opcode(OpCode::PLAYER_STATS);
    protocol.send_uint8(ev.level);
    protocol.send_uint32(ev.experience);
    protocol.send_uint32(ev.exp_to_next);
    protocol.send_uint32(ev.hp_current);
    protocol.send_uint32(ev.hp_max);
    protocol.send_uint32(ev.mana_current);
    protocol.send_uint32(ev.mana_max);
}

void ServerProtocol::send_heal_received(const HealReceivedEvent& ev) {
    protocol.send_opcode(OpCode::HEAL_RECEIVED);
    protocol.send_uint16(ev.player_id);
    protocol.send_uint32(ev.hp_current);
    protocol.send_uint32(ev.mana_current);
}

void ServerProtocol::send_spell_effect(const SpellEffectEvent& ev) {
    protocol.send_opcode(OpCode::SPELL_EFFECT);
    protocol.send_uint16(ev.target_id);
    protocol.send_uint8(ev.effect_type);
}

void ServerProtocol::send_inventory_update(const InventoryUpdateEvent& ev) {
    protocol.send_opcode(OpCode::INVENTORY_UPDATE);
    protocol.send_uint8(static_cast<uint8_t>(ev.slots.size()));
    for (const auto& slot: ev.slots) {
        protocol.send_uint8(slot.slot_index);
        protocol.send_uint8(static_cast<uint8_t>(slot.item_type));
        protocol.send_str(slot.item_name);
    }
}

void ServerProtocol::send_gold_update(const GoldUpdateEvent& ev) {
    protocol.send_opcode(OpCode::GOLD_UPDATE);
    protocol.send_uint32(ev.gold);
}

void ServerProtocol::send_equip_update(const EquipUpdateEvent& ev) {
    protocol.send_opcode(OpCode::EQUIP_UPDATE);
    protocol.send_uint16(ev.entity_id);
    const InventorySlot* slots[EQUIP_SLOT_COUNT] = {&ev.weapon, &ev.armor, &ev.helmet, &ev.shield};
    for (uint8_t i = 0; i < EQUIP_SLOT_COUNT; ++i) {
        protocol.send_uint8(static_cast<uint8_t>(slots[i]->item_type));
        protocol.send_str(slots[i]->item_name);
    }
}

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
                       [this](const AttackDodgedEvent& msg) { send_attack_dodged(msg); },
                       [this](const EntityDiedEvent& msg) { send_entity_died(msg); },
                       [this](const PlayerRespawnedEvent& msg) { send_player_respawned(msg); },
                       [this](const MeditationStartEvent&) { send_meditation_start(); },
                       [this](const MeditationStopEvent&) { send_meditation_stop(); },
                       [this](const ChatMsgEvent& msg) { send_chat_msg(msg); },
                       [this](const ClanNotificationEvent& msg) { send_clan_notification(msg); },
                       [this](const ClanUpdateEvent& msg) { send_clan_update(msg); },
                       [this](const MapTransitionEvent& msg) { send_map_transition(msg); },
                       [this](const PlayerStatsEvent& msg) { send_player_stats(msg); },
                       [this](const HealReceivedEvent& msg) { send_heal_received(msg); },
                       [this](const SpellEffectEvent& msg) { send_spell_effect(msg); },
                       [this](const InventoryUpdateEvent& msg) { send_inventory_update(msg); },
                       [this](const EquipUpdateEvent& msg) { send_equip_update(msg); },
                       [this](const GoldUpdateEvent& msg) { send_gold_update(msg); },
                       [](const auto&) { throw std::runtime_error("Event type not implemented"); },
               },
               ev);
}
