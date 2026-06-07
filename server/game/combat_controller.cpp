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

    uint32_t range = config.attack_range_px;
    const InventorySlot& weapon_slot2 = attacker.get_equipped(EquipSlot::WEAPON);
    const Item* weapon_def2 = item_catalog_.find(weapon_slot2.item_type);
    if (weapon_def2 && weapon_def2->attack_range > 0)
        range = weapon_def2->attack_range;

    if (!in_range(attacker.pos_x(), attacker.pos_y(), target.pos_x(), target.pos_y(), range))
        return {};

    uint32_t damage = calculate_damage(attacker);
    bool esquivado = false;
    if (is_critical_attack(attacker)) {
        damage *= 2;
    } else {
        esquivado = pow(rng.get_random_double(0, 1), target.get_agility()) < 0.001;
        if (esquivado)
            damage = 0;
    }

    uint32_t defense = calculate_defense(target);
    damage = damage > defense ? (damage - defense) : 0;

    target.take_damage(damage);
    attacker.gain_experience(damage * std::max(static_cast<int>(target.get_level()) -
                                                       static_cast<int>(attacker.get_level()) + 10,
                                               0));

    CommandResult result =
            notify_entity_attacked(attacker, target_id, damage, target.get_hp_current(),
                                   target.get_hp_max(), target.get_username(),
                                   target.get_clan_name(), target.is_dead(), target.get_level(),
                                   esquivado);

    if (target.is_dead()) {
        target.lose_experience_on_death();
        result.targeted_events[target_id].push_back(PlayerStatsEvent{
                .level = target.get_level(),
                .experience = target.get_experience(),
                .exp_to_next = target.exp_to_next_level(),
                .hp_current = target.get_hp_current(),
                .hp_max = target.get_hp_max(),
                .mana_current = target.get_mana_current(),
                .mana_max = target.get_mana_max(),
        });

        uint32_t excess = target.take_excess_gold();
        if (excess > 0) {
            attacker.gain_gold(excess);
            result.private_events.push_back(GoldUpdateEvent{attacker.get_gold()});
            result.targeted_events[target_id].push_back(GoldUpdateEvent{target.get_gold()});
        }
    }

    return result;
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

    uint32_t range = config.attack_range_px;
    const InventorySlot& weapon_slot2 = attacker.get_equipped(EquipSlot::WEAPON);
    const Item* weapon_def2 = item_catalog_.find(weapon_slot2.item_type);
    if (weapon_def2 && weapon_def2->attack_range > 0)
        range = weapon_def2->attack_range;

    if (!in_range(attacker.pos_x(), attacker.pos_y(), npc_target.pos_x(), npc_target.pos_y(), range))
        return {};

    uint32_t damage = calculate_damage(attacker);
    if (is_critical_attack(attacker)) {
        damage *= 2;
    }

    npc_target.take_damage(damage);
    attacker.gain_experience(damage * std::max(static_cast<int>(npc_target.get_level()) -
                                                       static_cast<int>(attacker.get_level()) + 10,
                                               0));

    bool esquivado_npc = false;
    CommandResult result =
            notify_entity_attacked(attacker, npc_target_id, damage, npc_target.get_hp_current(),
                                   npc_target.get_hp_max(), npc_target.get_name(), "",
                                   npc_target.is_dead(), npc_target.get_level(), esquivado_npc);

    if (npc_target.is_dead()) {
        EnemyDrop drop = npc_target.get_kill_reward();
        if (drop.gold > 0) {
            attacker.gain_gold(drop.gold);
            result.private_events.push_back(GoldUpdateEvent{attacker.get_gold()});
        }
    }

    return result;
}

CommandResult CombatController::spell_attack_player(uint16_t attacker_id, uint16_t target_id,
                                                    uint32_t) {
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

    if (!in_range(attacker.pos_x(), attacker.pos_y(), target.pos_x(), target.pos_y(),
                  config.spell_attack_range_px))
        return {};

    uint32_t damage = calculate_damage(attacker);
    bool esquivado = false;
    if (is_critical_attack(attacker)) {
        damage *= 2;
    } else {
        esquivado = pow(rng.get_random_double(0, 1), target.get_agility()) < 0.001;
        if (esquivado)
            damage = 0;
    }

    uint32_t defense = calculate_defense(target);
    damage = damage > defense ? (damage - defense) : 0;

    target.take_damage(damage);
    attacker.gain_experience(damage * std::max(static_cast<int>(target.get_level()) -
                                                       static_cast<int>(attacker.get_level()) + 10,
                                               0));

    CommandResult result =
            notify_entity_attacked(attacker, target_id, damage, target.get_hp_current(),
                                   target.get_hp_max(), target.get_username(),
                                   target.get_clan_name(), target.is_dead(), target.get_level(),
                                   esquivado);

    if (target.is_dead()) {
        target.lose_experience_on_death();
        result.targeted_events[target_id].push_back(PlayerStatsEvent{
                .level = target.get_level(),
                .experience = target.get_experience(),
                .exp_to_next = target.exp_to_next_level(),
                .hp_current = target.get_hp_current(),
                .hp_max = target.get_hp_max(),
                .mana_current = target.get_mana_current(),
                .mana_max = target.get_mana_max(),
        });

        uint32_t excess = target.take_excess_gold();
        if (excess > 0) {
            attacker.gain_gold(excess);
            result.private_events.push_back(GoldUpdateEvent{attacker.get_gold()});
            result.targeted_events[target_id].push_back(GoldUpdateEvent{target.get_gold()});
        }
    }

    return result;
}

CommandResult CombatController::spell_attack_npc(uint16_t attacker_id, uint16_t npc_target_id,
                                                 std::map<uint16_t, EnemyNpc>& npcs,
                                                 uint32_t) {
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

    if (!in_range(attacker.pos_x(), attacker.pos_y(), npc_target.pos_x(), npc_target.pos_y(),
                  config.spell_attack_range_px))
        return {};

    uint32_t damage = calculate_damage(attacker);
    if (is_critical_attack(attacker)) {
        damage *= 2;
    }

    npc_target.take_damage(damage);
    attacker.gain_experience(damage * std::max(static_cast<int>(npc_target.get_level()) -
                                                       static_cast<int>(attacker.get_level()) + 10,
                                               0));

    CommandResult result =
            notify_entity_attacked(attacker, npc_target_id, damage, npc_target.get_hp_current(),
                                   npc_target.get_hp_max(), npc_target.get_name(), "",
                                   npc_target.is_dead(), npc_target.get_level(), false);

    if (npc_target.is_dead()) {
        EnemyDrop drop = npc_target.get_kill_reward();
        if (drop.gold > 0) {
            attacker.gain_gold(drop.gold);
            result.private_events.push_back(GoldUpdateEvent{attacker.get_gold()});
        }
    }

    return result;
}

bool CombatController::is_critical_attack(const Player& attacker) {
    double critic_probability = attacker.get_strength() * config.critical_chance * 100;
    double random_number = rng.get_random_double(0, 99);

    if (random_number < critic_probability)
        return true;
    return false;
}

bool CombatController::in_range(uint16_t attacker_x, uint16_t attacker_y, uint16_t target_x,
                                uint16_t target_y) const {
    return in_range(attacker_x, attacker_y, target_x, target_y, config.attack_range_px);
}

bool CombatController::in_range(uint16_t attacker_x, uint16_t attacker_y, uint16_t target_x,
                                uint16_t target_y, uint32_t range_px) const {
    const int dx = static_cast<int>(target_x) - static_cast<int>(attacker_x);
    const int dy = static_cast<int>(target_y) - static_cast<int>(attacker_y);
    const int dist_sq = dx * dx + dy * dy;
    const int range_sq = static_cast<int>(range_px) * static_cast<int>(range_px);
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
    double bonus = get_clan_bonus(attacker);
    return static_cast<uint32_t>(std::round(base * (1.0 + bonus)));
}

uint32_t CombatController::calculate_defense(const Player& target) {
    const InventorySlot& armor_slot = target.get_equipped(EquipSlot::ARMOR);
    const InventorySlot& shield_slot = target.get_equipped(EquipSlot::SHIELD);
    const InventorySlot& helmet_slot = target.get_equipped(EquipSlot::HELMET);

    uint32_t base = calculate_object_defense(armor_slot) + calculate_object_defense(shield_slot) +
                    calculate_object_defense(helmet_slot);
    double bonus = get_clan_bonus(target);
    return static_cast<uint32_t>(std::round(base * (1.0 + bonus)));
}

uint32_t CombatController::calculate_object_defense(const InventorySlot& object_slot) {
    uint32_t object_defense = 0;
    if (object_slot.item_type != ItemType::NONE) {
        const Item* object = item_catalog_.find(object_slot.item_type);
        int defense = rng.get_random_int(object->min_defense, object->max_defense);
        object_defense = static_cast<uint32_t>(defense);
    }
    return object_defense;
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

double CombatController::get_clan_bonus(const Player& player) const {
    if (!clan_manager || player.get_clan_name().empty())
        return 0;
    int nearby_allies = count_nearby_clan_members(player);
    return std::min(config.clan_bonus_per_member * nearby_allies, config.clan_bonus_max);
}

CommandResult CombatController::notify_entity_attacked(Player& attacker, uint16_t target_id,
                                                       uint32_t damage, uint32_t target_hp_current,
                                                       uint32_t target_hp_max,
                                                       const std::string& target_name,
                                                       const std::string& target_clan_name,
                                                       bool target_is_dead, uint8_t target_level,
                                                       bool esquivado) {
    DamageDealtEvent dealt{attacker.get_id(), damage};
    std::vector<ServerEvent> broadcast;
    std::map<uint16_t, std::vector<ServerEvent>> targeted;

    if (esquivado) {
        AttackDodgedEvent dodged{target_id};
        targeted[target_id].push_back(dodged);
        targeted[attacker.get_id()].push_back(dodged);
    } else {
        DamageReceivedEvent received{target_id, attacker.get_id(), damage, target_hp_current,
                                     target_hp_max};
        ChatMsgEvent chat_msg{ChatMsgType::SYSTEM, "",
                              attacker.get_username() + " ataco a " + target_name + " por " +
                                      std::to_string(damage) + " de daño"};
        targeted[target_id].push_back(received);
        targeted[target_id].push_back(chat_msg);
        targeted[attacker.get_id()].push_back(chat_msg);

        if (target_is_dead) {
            EntityDiedEvent died{target_id};
            broadcast.push_back(died);
            broadcast.push_back(ChatMsgEvent{ChatMsgType::SYSTEM, "",
                                             attacker.get_username() + " mato a " + target_name});

            double random_double = rng.get_random_double(0, 0.1);
            attacker.gain_experience(random_double * target_hp_max *
                                     std::max(static_cast<int>(target_level) -
                                                      static_cast<int>(attacker.get_level()) + 10,
                                              0));
        }
    }

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
            .hp_current = attacker.get_hp_current(),
            .hp_max = attacker.get_hp_max(),
            .mana_current = attacker.get_mana_current(),
            .mana_max = attacker.get_mana_max(),
    };
    return {.private_events = {dealt, stats},
            .broadcast_events = std::move(broadcast),
            .targeted_events = std::move(targeted)};
}
