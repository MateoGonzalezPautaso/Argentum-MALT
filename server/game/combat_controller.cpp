#include "combat_controller.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#include "clan_manager.h"

CombatController::CombatController(const AttackConfig& config, std::map<uint16_t, Player>& players,
                                   const ItemCatalog& catalog):
        config(config), players(players), item_catalog_(catalog) {}

void CombatController::set_clan_manager(ClanManager& mgr) { clan_manager = &mgr; }

CommandResult CombatController::melee_attack_player(uint16_t attacker_id, uint16_t target_id,
                                                    uint32_t current_tick) {
    if (attacker_id == target_id)
        return {};

    auto attacker_it = players.find(attacker_id);
    if (attacker_it == players.end())
        return {};

    auto target_it = players.find(target_id);
    if (target_it == players.end())
        return {};

    Player& attacker = attacker_it->second;
    Player& target = target_it->second;

    if (attacker.is_dead() || target.is_dead())
        return {};


    if (attacker.get_level() <= config.newbie_level) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "No puedes atacar siendo newbie"};
        return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
    }
    if (target.get_level() <= config.newbie_level) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "No puedes atacar a un jugador newbie"};
        return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
    }

    int level_diff =
            std::abs(static_cast<int>(attacker.get_level()) - static_cast<int>(target.get_level()));
    if (level_diff > config.max_level_diff) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                         "No puedes atacar a un jugador con diferencia de niveles mayor a 10"};
        return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
    }

    if (clan_manager && !attacker.get_clan_name().empty() && !target.get_clan_name().empty() &&
        attacker.get_clan_name() == target.get_clan_name()) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "No puedes atacar a un miembro de tu clan"};
        return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
    }

    if (!attacker.try_attack(current_tick, config.cooldown_ticks))
        return {};

    if (!in_range(attacker.pos_x(), attacker.pos_y(), target.pos_x(), target.pos_y()))
        return {};

    uint32_t damage = calculate_damage(attacker);
    target.take_damage(damage);
    attacker.gain_experience(damage * std::max(static_cast<int>(target.get_level()) -
                                                       static_cast<int>(attacker.get_level()) + 10,
                                               0));

    return notify_entity_attacked(attacker, target_id, damage, target.get_hp_current(),
                                  target.get_hp_max(), target.get_username(),
                                  target.get_clan_name(), target.is_dead(), target.get_level());
}

CommandResult CombatController::melee_attack_npc(uint16_t attacker_id, uint16_t npc_target_id,
                                                 std::map<uint16_t, EnemyNpc>& npcs,
                                                 uint32_t current_tick) {
    auto attacker_it = players.find(attacker_id);
    if (attacker_it == players.end())
        return {};

    auto npc_target_it = npcs.find(npc_target_id);
    if (npc_target_it == npcs.end())
        return {};

    Player& attacker = attacker_it->second;
    if (attacker.is_dead())
        return {};

    EnemyNpc& npc_target = npc_target_it->second;

    if (!attacker.try_attack(current_tick, config.cooldown_ticks))
        return {};

    if (!in_range(attacker.pos_x(), attacker.pos_y(), npc_target.pos_x(), npc_target.pos_y()))
        return {};

    uint32_t damage = calculate_damage(attacker);
    npc_target.take_damage(damage);
    attacker.gain_experience(damage * std::max(static_cast<int>(npc_target.get_level()) -
                                                       static_cast<int>(attacker.get_level()) + 10,
                                               0));

    return notify_entity_attacked(attacker, npc_target_id, damage, npc_target.get_hp_current(),
                                  npc_target.get_hp_max(), npc_target.get_name(), "",
                                  npc_target.is_dead(), npc_target.get_level());
}

bool CombatController::in_range(uint16_t attacker_x, uint16_t attacker_y, uint16_t target_x,
                                uint16_t target_y) const {
    const int dx = static_cast<int>(target_x) - static_cast<int>(attacker_x);
    const int dy = static_cast<int>(target_y) - static_cast<int>(attacker_y);
    const int dist_sq = dx * dx + dy * dy;
    const int range_sq = config.attack_range_px * config.attack_range_px;
    return dist_sq <= range_sq;
}

uint32_t CombatController::calculate_damage(const Player& attacker) {
    const InventorySlot& weapon_slot = attacker.get_equipped(EquipSlot::WEAPON);
    const Item* weapon = nullptr;
    if (weapon_slot.item_type != ItemType::NONE)
        weapon = item_catalog_.find(weapon_slot.item_type);
    uint32_t base;
    if (weapon && weapon->max_damage > 0) {
        int roll = rng.get_random_int(weapon->min_damage, weapon->max_damage);
        base = attacker.get_strength() * static_cast<uint32_t>(roll);
    } else {
        int variance =
                config.damage_variance > 0 ? rng.get_random_int(0, config.damage_variance) : 0;
        base = static_cast<uint32_t>(config.base_damage + variance);
    }
    double bonus = get_clan_damage_bonus(attacker);
    return static_cast<uint32_t>(std::round(base * (1.0 + bonus)));
}

int CombatController::count_nearby_clan_members(const Player& player) const {
    if (!clan_manager)
        return 0;
    int count = 0;
    for (const auto& [pid, p]: players) {
        if (pid == player.get_id())
            continue;
        if (p.get_clan_name() != player.get_clan_name())
            continue;
        if (p.is_dead())
            continue;
        const int dx = static_cast<int>(p.pos_x()) - static_cast<int>(player.pos_x());
        const int dy = static_cast<int>(p.pos_y()) - static_cast<int>(player.pos_y());
        const int dist_sq = dx * dx + dy * dy;
        if (dist_sq <= config.clan_bonus_range_px * config.clan_bonus_range_px) {
            ++count;
        }
    }
    return count;
}

// Clan proximity bonus: increase attack damage based on nearby clan members
double CombatController::get_clan_damage_bonus(const Player& attacker) const {
    double bonus = 0;
    if (clan_manager && !attacker.get_clan_name().empty()) {
        int nearby_allies = count_nearby_clan_members(attacker);
        bonus = std::min(config.clan_bonus_per_member * nearby_allies, config.clan_bonus_max);
    }
    return bonus;
}

CommandResult CombatController::notify_entity_attacked(Player& attacker, uint16_t target_id,
                                                       uint32_t damage, uint32_t target_hp_current,
                                                       uint32_t target_hp_max,
                                                       const std::string& target_name,
                                                       const std::string& target_clan_name,
                                                       bool target_is_dead, uint8_t target_level) {
    DamageDealtEvent dealt{attacker.get_id(), damage};
    DamageReceivedEvent received{target_id, attacker.get_id(), damage, target_hp_current,
                                 target_hp_max};
    ChatMsgEvent chat_msg{ChatMsgType::SYSTEM, "",
                          attacker.get_username() + " ataco a " + target_name + " por " +
                                  std::to_string(damage) + " de daño"};
    std::vector<ServerEvent> broadcast = {received, chat_msg};

    if (target_is_dead) {
        EntityDiedEvent died{target_id};
        broadcast.push_back(died);

        double random_double = rng.get_random_double(0, 0.1);
        attacker.gain_experience(random_double * target_hp_max *
                                 std::max(static_cast<int>(target_level) -
                                                  static_cast<int>(attacker.get_level()) + 10,
                                          0));
    }

    std::map<uint16_t, std::vector<ServerEvent>> targeted;
    // Notify clan members when someone is attacked
    if (clan_manager && !target_clan_name.empty()) {
        ClanNotificationEvent notif{ClanNotifType::MEMBER_ATTACKED, target_name,
                                    std::string(target_clan_name)};
        for (const auto& [pid, p]: players) {
            if (pid == attacker.get_id() || pid == target_id)
                continue;
            if (p.get_clan_name() == target_clan_name) {
                targeted[pid].push_back(notif);
            }
        }
    }

    PlayerStatsEvent stats{
            .level = attacker.get_level(),
            .experience = attacker.get_experience(),
            .exp_to_next = attacker.exp_to_next_level(),
            .hp_max = attacker.get_hp_max(),
            .mana_max = attacker.get_mana_max(),
    };
    return {.private_events = {dealt, stats},
            .broadcast_events = std::move(broadcast),
            .targeted_events = std::move(targeted)};
}
