#include "client_protocol.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <sys/socket.h>

#include "../../common/socket.h"
#include "../../common/visit.h"

ClientProtocol::ClientProtocol(Socket&& skt): skt(std::move(skt)), protocol(this->skt) {}

void ClientProtocol::shutdown_from_other_thread() { skt.shutdown_from_other_thread(SHUT_RDWR); }

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

void ClientProtocol::send_cast_spell(const CastSpellCmd& cmd) {
    protocol.send_opcode(OpCode::CAST_SPELL);
    protocol.send_uint16(cmd.target_id);
}

void ClientProtocol::send_meditate() { protocol.send_opcode(OpCode::MEDITATE); }

void ClientProtocol::send_resurrect() { protocol.send_opcode(OpCode::RESURRECT); }

void ClientProtocol::send_npc_heal(const NpcHealCmd&) { protocol.send_opcode(OpCode::NPC_HEAL); }

void ClientProtocol::send_npc_list(const NpcListCmd&) {
    protocol.send_opcode(OpCode::NPC_LIST);
}

void ClientProtocol::send_npc_buy(const NpcBuyCmd& cmd) {
    protocol.send_opcode(OpCode::NPC_BUY);
    protocol.send_str(cmd.item_name);
}

void ClientProtocol::send_npc_sell(const NpcSellCmd& cmd) {
    protocol.send_opcode(OpCode::NPC_SELL);
    protocol.send_str(cmd.item_name);
}

void ClientProtocol::send_bank_deposit(const BankDepositCmd& cmd) {
    protocol.send_opcode(OpCode::BANK_DEPOSIT);
    protocol.send_uint8(cmd.is_gold ? 1 : 0);
    if (cmd.is_gold) {
        protocol.send_uint32(cmd.gold_amount);
    } else {
        protocol.send_str(cmd.item_name);
    }
}

void ClientProtocol::send_bank_withdraw(const BankWithdrawCmd& cmd) {
    protocol.send_opcode(OpCode::BANK_WITHDRAW);
    protocol.send_uint8(cmd.is_gold ? 1 : 0);
    if (cmd.is_gold) {
        protocol.send_uint32(cmd.gold_amount);
    } else {
        protocol.send_str(cmd.item_name);
    }
}

void ClientProtocol::send_pickup_item(const PickupItemCmd& cmd) {
    protocol.send_opcode(OpCode::PICKUP_ITEM);
    protocol.send_str(cmd.item_name);
}

void ClientProtocol::send_drop_item(const DropItemCmd& cmd) {
    protocol.send_opcode(OpCode::DROP_ITEM);
    protocol.send_str(cmd.item_name);
}

void ClientProtocol::send_cheat_infinite_hp() { protocol.send_opcode(OpCode::CHEAT_INFINITE_HP); }

void ClientProtocol::send_cheat_infinite_mana() {
    protocol.send_opcode(OpCode::CHEAT_INFINITE_MANA);
}

void ClientProtocol::send_cheat_die() { protocol.send_opcode(OpCode::CHEAT_DIE); }

void ClientProtocol::send_cheat_level_up() { protocol.send_opcode(OpCode::CHEAT_LEVEL_UP); }

void ClientProtocol::send_cheat_level_down() { protocol.send_opcode(OpCode::CHEAT_LEVEL_DOWN); }

void ClientProtocol::send_cheat_add_gold() { protocol.send_opcode(OpCode::CHEAT_ADD_GOLD); }

void ClientProtocol::send_cheat_reset_gold() { protocol.send_opcode(OpCode::CHEAT_RESET_GOLD); }

void ClientProtocol::send_cheat_velocity() { protocol.send_opcode(OpCode::CHEAT_VELOCITY); }

void ClientProtocol::send_cheat_revive() { protocol.send_opcode(OpCode::CHEAT_REVIVE); }

void ClientProtocol::send_cheat_fill_inventory() {
    protocol.send_opcode(OpCode::CHEAT_FILL_INVENTORY);
}

void ClientProtocol::send_cheat_clear_inventory() {
    protocol.send_opcode(OpCode::CHEAT_CLEAR_INVENTORY);
}

void ClientProtocol::send_cheat_reset_mana() { protocol.send_opcode(OpCode::CHEAT_RESET_MANA); }

void ClientProtocol::send_clan_found(const ClanFoundCmd& cmd) {
    protocol.send_opcode(OpCode::CLAN_FOUND);
    protocol.send_str(cmd.clan_name);
}

void ClientProtocol::send_clan_join_request(const ClanJoinRequestCmd& cmd) {
    protocol.send_opcode(OpCode::CLAN_JOIN_REQUEST);
    protocol.send_str(cmd.clan_name);
}

void ClientProtocol::send_clan_review() { protocol.send_opcode(OpCode::CLAN_REVIEW); }

void ClientProtocol::send_clan_accept(const ClanAcceptCmd& cmd) {
    protocol.send_opcode(OpCode::CLAN_ACCEPT);
    protocol.send_str(cmd.target_nick);
}

void ClientProtocol::send_clan_reject(const ClanRejectCmd& cmd) {
    protocol.send_opcode(OpCode::CLAN_REJECT);
    protocol.send_str(cmd.target_nick);
}

void ClientProtocol::send_clan_ban(const ClanBanCmd& cmd) {
    protocol.send_opcode(OpCode::CLAN_BAN);
    protocol.send_str(cmd.target_nick);
}

void ClientProtocol::send_clan_unban(const ClanUnbanCmd& cmd) {
    protocol.send_opcode(OpCode::CLAN_UNBAN);
    protocol.send_str(cmd.target_nick);
}

void ClientProtocol::send_clan_kick(const ClanKickCmd& cmd) {
    protocol.send_opcode(OpCode::CLAN_KICK);
    protocol.send_str(cmd.target_nick);
}

void ClientProtocol::send_clan_leave() { protocol.send_opcode(OpCode::CLAN_LEAVE); }

void ClientProtocol::send_change_map(const ChangeMapCmd& cmd) {
    protocol.send_opcode(OpCode::CHANGE_MAP);
    protocol.send_str(cmd.prop_name);
}

void ClientProtocol::send_equip_item(const EquipItemCmd& cmd) {
    protocol.send_opcode(OpCode::EQUIP_ITEM);
    protocol.send_uint8(cmd.slot_index);
}

void ClientProtocol::send_unequip_item(const UnequipItemCmd& cmd) {
    protocol.send_opcode(OpCode::UNEQUIP_ITEM);
    protocol.send_uint8(static_cast<uint8_t>(cmd.slot));
}

void ClientProtocol::send_command(const ClientCommand& cmd) {
    std::visit(overloaded{
                       [this](const MoveCmd& msg) { send_move(msg); },
                       [this](const LoginCmd& msg) { send_login(msg); },
                       [this](const CreateCharacterCmd& msg) { send_create_character(msg); },
                       [this](const AttackCmd& msg) { send_attack(msg); },
                       [this](const SendChatMsgCmd& msg) { send_chat_msg(msg); },
                       [this](const CastSpellCmd& msg) { send_cast_spell(msg); },
                       [this](const MeditateCmd&) { send_meditate(); },
                       [this](const ResurrectCmd&) { send_resurrect(); },
                       [this](const CheatInfiniteHpCmd&) { send_cheat_infinite_hp(); },
                       [this](const CheatInfiniteManaCmd&) { send_cheat_infinite_mana(); },
                       [this](const CheatDieCmd&) { send_cheat_die(); },
                       [this](const CheatLevelUpCmd&) { send_cheat_level_up(); },
                       [this](const CheatLevelDownCmd&) { send_cheat_level_down(); },
                       [this](const CheatAddGoldCmd&) { send_cheat_add_gold(); },
                       [this](const CheatResetGoldCmd&) { send_cheat_reset_gold(); },
                       [this](const CheatVelocityCmd&) { send_cheat_velocity(); },
                       [this](const CheatReviveCmd&) { send_cheat_revive(); },
                       [this](const CheatFillInventoryCmd&) { send_cheat_fill_inventory(); },
                       [this](const CheatClearInventoryCmd&) { send_cheat_clear_inventory(); },
                       [this](const CheatResetManaCmd&) { send_cheat_reset_mana(); },
                       [this](const ChangeMapCmd& msg) { send_change_map(msg); },
                       [this](const EquipItemCmd& msg) { send_equip_item(msg); },
                       [this](const UnequipItemCmd& msg) { send_unequip_item(msg); },
                       [this](const NpcHealCmd& msg) { send_npc_heal(msg); },
                       [this](const NpcListCmd& msg) { send_npc_list(msg); },
                       [this](const NpcBuyCmd& msg) { send_npc_buy(msg); },
                       [this](const NpcSellCmd& msg) { send_npc_sell(msg); },
                       [this](const BankDepositCmd& msg) { send_bank_deposit(msg); },
                       [this](const BankWithdrawCmd& msg) { send_bank_withdraw(msg); },
                       [this](const PickupItemCmd& msg) { send_pickup_item(msg); },
                       [this](const DropItemCmd& msg) { send_drop_item(msg); },
                       [this](const ClanFoundCmd& msg) { send_clan_found(msg); },
                       [this](const ClanJoinRequestCmd& msg) { send_clan_join_request(msg); },
                       [this](const ClanReviewCmd&) { send_clan_review(); },
                       [this](const ClanAcceptCmd& msg) { send_clan_accept(msg); },
                       [this](const ClanRejectCmd& msg) { send_clan_reject(msg); },
                       [this](const ClanBanCmd& msg) { send_clan_ban(msg); },
                       [this](const ClanUnbanCmd& msg) { send_clan_unban(msg); },
                       [this](const ClanKickCmd& msg) { send_clan_kick(msg); },
                       [this](const ClanLeaveCmd&) { send_clan_leave(); },
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
        case OpCode::DAMAGE_DEALT:
            return recv_damage_dealt();
        case OpCode::DAMAGE_RECEIVED:
            return recv_damage_received();
        case OpCode::ATTACK_DODGED:
            return AttackDodgedEvent{protocol.recv_uint16()};
        case OpCode::ENTITY_DIED:
            return recv_entity_died();
        case OpCode::PLAYER_RESPAWNED:
            return recv_player_respawned();
        case OpCode::MEDITATION_START:
            return MeditationStartEvent{};
        case OpCode::MEDITATION_STOP:
            return MeditationStopEvent{};
        case OpCode::INVENTORY_UPDATE: {
            uint8_t count = protocol.recv_uint8();
            std::vector<InventorySlot> slots;
            slots.reserve(count);
            for (uint8_t i = 0; i < count; ++i) {
                InventorySlot slot;
                slot.slot_index = protocol.recv_uint8();
                slot.item_type = static_cast<ItemType>(protocol.recv_uint8());
                slot.item_name = protocol.recv_str();
                slots.push_back(std::move(slot));
            }
            return InventoryUpdateEvent{std::move(slots)};
        }
        case OpCode::EQUIP_UPDATE: {
            auto read_slot = [this]() {
                InventorySlot s;
                s.item_type = static_cast<ItemType>(protocol.recv_uint8());
                s.item_name = protocol.recv_str();
                return s;
            };
            EquipUpdateEvent ev;
            ev.entity_id = protocol.recv_uint16();
            ev.weapon = read_slot();
            ev.armor = read_slot();
            ev.helmet = read_slot();
            ev.shield = read_slot();
            return ev;
        }
        case OpCode::CHAT_MSG:
            return recv_chat_msg();
        case OpCode::CLAN_NOTIFICATION:
            return recv_clan_notification();
        case OpCode::CLAN_UPDATE:
            return recv_clan_update();
        case OpCode::HEAL_RECEIVED:
            return recv_heal_received();
        case OpCode::SPELL_EFFECT:
            return recv_spell_effect();
        case OpCode::MAP_TRANSITION:
            return recv_map_transition();
        case OpCode::PLAYER_STATS:
            return recv_player_stats();
        case OpCode::GOLD_UPDATE:
            return GoldUpdateEvent{protocol.recv_uint32()};
        case OpCode::NPC_ITEM_LIST:
            return recv_npc_item_list();
        case OpCode::BANK_UPDATE:
            return recv_bank_update();
        case OpCode::ITEM_DROPPED:
            return recv_item_dropped();
        case OpCode::ITEM_PICKED:
            return recv_item_picked();
        default:
            throw std::runtime_error("Unknown event opcode: " +
                                     std::to_string(static_cast<int>(opcode)));
    }
}

ServerEvent ClientProtocol::recv_damage_dealt() {
    uint16_t target_id = protocol.recv_uint16();
    uint32_t damage = protocol.recv_uint32();
    return DamageDealtEvent{target_id, damage};
}

ServerEvent ClientProtocol::recv_damage_received() {
    uint16_t target_id = protocol.recv_uint16();
    uint16_t attacker_id = protocol.recv_uint16();
    uint32_t damage = protocol.recv_uint32();
    uint32_t hp_current = protocol.recv_uint32();
    uint32_t hp_max = protocol.recv_uint32();
    return DamageReceivedEvent{target_id, attacker_id, damage, hp_current, hp_max};
}

ServerEvent ClientProtocol::recv_entity_died() {
    uint16_t entity_id = protocol.recv_uint16();
    return EntityDiedEvent{entity_id};
}

ServerEvent ClientProtocol::recv_player_respawned() {
    uint16_t entity_id = protocol.recv_uint16();
    uint32_t hp_current = protocol.recv_uint32();
    uint32_t hp_max = protocol.recv_uint32();
    return PlayerRespawnedEvent{entity_id, hp_current, hp_max};
}

ServerEvent ClientProtocol::recv_chat_msg() {
    ChatMsgType type = static_cast<ChatMsgType>(protocol.recv_uint8());
    std::string sender = protocol.recv_str();
    std::string message = protocol.recv_str();
    uint16_t recipient_id = protocol.recv_uint16();
    uint16_t sender_id = protocol.recv_uint16();
    return ChatMsgEvent{type, sender, message, recipient_id, sender_id};
}

ServerEvent ClientProtocol::recv_clan_notification() {
    ClanNotifType type = static_cast<ClanNotifType>(protocol.recv_uint8());
    std::string username = protocol.recv_str();
    std::string clan_name = protocol.recv_str();
    return ClanNotificationEvent{type, username, clan_name};
}

ServerEvent ClientProtocol::recv_clan_update() {
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

ServerEvent ClientProtocol::recv_heal_received() {
    uint16_t player_id = protocol.recv_uint16();
    uint32_t hp_current = protocol.recv_uint32();
    uint32_t mana_current = protocol.recv_uint32();
    return HealReceivedEvent{player_id, hp_current, mana_current};
}

ServerEvent ClientProtocol::recv_spell_effect() {
    uint16_t target_id = protocol.recv_uint16();
    uint8_t effect_type = protocol.recv_uint8();
    return SpellEffectEvent{target_id, effect_type};
}

ServerEvent ClientProtocol::recv_player_stats() {
    PlayerStatsEvent ev;
    ev.level = protocol.recv_uint8();
    ev.experience = protocol.recv_uint32();
    ev.exp_to_next = protocol.recv_uint32();
    ev.hp_current = protocol.recv_uint32();
    ev.hp_max = protocol.recv_uint32();
    ev.mana_current = protocol.recv_uint32();
    ev.mana_max = protocol.recv_uint32();
    ev.crit_chance = protocol.recv_uint8();
    ev.damage_min = protocol.recv_uint16();
    ev.damage_max = protocol.recv_uint16();
    ev.defense_min = protocol.recv_uint16();
    ev.defense_max = protocol.recv_uint16();
    ev.dodge_chance = protocol.recv_uint8();
    ev.strength = protocol.recv_uint16();
    ev.agility = protocol.recv_uint16();
    return ev;
}

ServerEvent ClientProtocol::recv_map_transition() {
    MapTransitionEvent ev;
    ev.map_name = protocol.recv_str();
    ev.pos_x = protocol.recv_uint16();
    ev.pos_y = protocol.recv_uint16();
    return ev;
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
    ev.weapon_type = static_cast<ItemType>(protocol.recv_uint8());
    ev.armor_type = static_cast<ItemType>(protocol.recv_uint8());
    ev.helmet_type = static_cast<ItemType>(protocol.recv_uint8());
    ev.shield_type = static_cast<ItemType>(protocol.recv_uint8());
    ev.sprite_id = protocol.recv_uint16();
    ev.clan_name = protocol.recv_str();
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

ServerEvent ClientProtocol::recv_bank_update() {
    uint8_t count = protocol.recv_uint8();
    std::vector<InventorySlot> slots;
    slots.reserve(count);
    for (uint8_t i = 0; i < count; ++i) {
        InventorySlot slot;
        slot.slot_index = protocol.recv_uint8();
        slot.item_type = static_cast<ItemType>(protocol.recv_uint8());
        slot.item_name = protocol.recv_str();
        slots.push_back(std::move(slot));
    }
    uint32_t gold = protocol.recv_uint32();
    return BankUpdateEvent{std::move(slots), gold};
}

ServerEvent ClientProtocol::recv_item_dropped() {
    ItemDroppedEvent ev;
    ev.pos.x = protocol.recv_uint16();
    ev.pos.y = protocol.recv_uint16();
    ev.item_type = static_cast<ItemType>(protocol.recv_uint8());
    ev.item_name = protocol.recv_str();
    ev.amount = protocol.recv_uint32();
    return ev;
}

ServerEvent ClientProtocol::recv_item_picked() {
    ItemPickedEvent ev;
    ev.pos.x = protocol.recv_uint16();
    ev.pos.y = protocol.recv_uint16();
    ev.item_name = protocol.recv_str();
    ev.amount = protocol.recv_uint32();
    return ev;
}

ServerEvent ClientProtocol::recv_npc_item_list() {
    uint16_t count = protocol.recv_uint16();
    std::vector<NpcItemEntry> items;
    items.reserve(count);
    for (uint16_t i = 0; i < count; ++i) {
        NpcItemEntry entry;
        entry.item_name = protocol.recv_str();
        entry.item_type = static_cast<ItemType>(protocol.recv_uint8());
        entry.sprite_id = protocol.recv_uint8();
        entry.price = protocol.recv_uint32();
        items.push_back(std::move(entry));
    }
    return NpcItemListEvent{std::move(items)};
}
