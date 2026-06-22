#include "combat_controller.h"

#include <cmath>
#include <utility>
#include <vector>

#include "clan_manager.h"
#include "entity_event_factory.h"
#include "game_formulas.h"
#include "player_registry.h"

CombatController::CombatController(const AttackConfig& config, std::map<uint16_t, Player>& players,
                                   const ItemCatalog& catalog,
                                   std::map<uint16_t, EnemyNpc>& enemy_npcs,
                                   const NpcDropConfig& drop_config,
                                   const NpcDropConfig& drop_config_dungeon,
                                   const BalanceConfig& balance):
        config(config),
        balance(balance),
        players(players),
        item_catalog_(catalog),
        enemy_npcs(enemy_npcs),
        npc_drop_config(drop_config),
        npc_drop_config_dungeon(drop_config_dungeon) {}

const NpcDropConfig& CombatController::drop_config_for(const EnemyNpc& npc) const {
    if (!maps)
        return npc_drop_config;
    auto it = maps->find(npc.get_current_map());
    if (it == maps->end())
        return npc_drop_config;
    return it->second.config().map_type == MapType::DUNGEON ? npc_drop_config_dungeon :
                                                              npc_drop_config;
}

void CombatController::set_clan_manager(ClanManager& mgr) { clan_manager = &mgr; }

std::optional<CommandResult> CombatController::validate_pvp(const Player& attacker,
                                                            const Player& target) const {
    if (attacker.get_level() <= config.newbie_level) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "No puedes atacar siendo newbie"};
        return CommandResult{.private_events = {msg}};
    }
    if (target.get_level() <= config.newbie_level) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "No puedes atacar a un jugador newbie"};
        return CommandResult{.private_events = {msg}};
    }

    int level_diff =
            std::abs(static_cast<int>(attacker.get_level()) - static_cast<int>(target.get_level()));
    if (level_diff > config.max_level_diff) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                         "No puedes atacar a un jugador con diferencia de niveles mayor a 10"};
        return CommandResult{.private_events = {msg}};
    }

    if (clan_manager && !attacker.get_clan_name().empty() && !target.get_clan_name().empty() &&
        attacker.get_clan_name() == target.get_clan_name()) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "No puedes atacar a un miembro de tu clan"};
        return CommandResult{.private_events = {msg}};
    }

    return std::nullopt;
}

CommandResult CombatController::resolve_player_attack(Player& attacker, Player& target,
                                                      uint16_t target_id, uint32_t range_px) {
    if (!in_range(attacker.pos_x(), attacker.pos_y(), target.pos_x(), target.pos_y(), range_px))
        return {};

    uint32_t damage = calculate_damage(attacker);
    bool esquivado = false;
    if (is_critical_attack(attacker)) {
        damage *= static_cast<uint32_t>(config.critical_multiplier);
    } else {
        esquivado = GameFormulas::is_dodged(config, target.get_agility(), rng);
        if (esquivado)
            damage = 0;
    }

    uint32_t defense = calculate_defense(target);
    damage = damage > defense ? (damage - defense) : 0;

    target.take_damage(damage);
    attacker.gain_experience(GameFormulas::attack_experience(
            damage, attacker.get_level(), target.get_level(), balance.experience_level_offset));

    CommandResult result =
            notify_entity_attacked(attacker, target_id, damage, target.get_hp_current(),
                                   target.get_hp_max(), target.get_name(), target.get_clan_name(),
                                   target.is_dead(), target.get_level(), esquivado);

    if (target.is_dead()) {
        on_player_death(target, target_id, &attacker, result.targeted_events[target_id],
                        result.ground_drops, &result.private_events);
    }

    return result;
}

CommandResult CombatController::resolve_npc_attack(Player& attacker, EnemyNpc& npc_target,
                                                   uint16_t npc_target_id, uint32_t range_px,
                                                   bool include_item_drop) {
    if (!in_range(attacker.pos_x(), attacker.pos_y(), npc_target.pos_x(), npc_target.pos_y(),
                  range_px))
        return {};

    uint32_t damage = calculate_damage(attacker);
    if (is_critical_attack(attacker)) {
        damage *= static_cast<uint32_t>(config.critical_multiplier);
    }

    npc_target.take_damage(damage);
    attacker.gain_experience(GameFormulas::attack_experience(
            damage, attacker.get_level(), npc_target.get_level(), balance.experience_level_offset));

    CommandResult result = notify_entity_attacked(
            attacker, npc_target_id, damage, npc_target.get_hp_current(), npc_target.get_hp_max(),
            npc_target.get_name(), "", npc_target.is_dead(), npc_target.get_level(), false);

    if (npc_target.is_dead()) {
        EnemyDrop drop = npc_target.get_kill_reward(drop_config_for(npc_target), balance);
        if (drop.gold > 0) {
            attacker.gain_gold(drop.gold);
            result.private_events.push_back(GoldUpdateEvent{attacker.get_gold()});
        }
        if (include_item_drop && drop.item.has_value()) {
            const Item& item = drop.item.value();
            if (attacker.add_item(item.type, item.name)) {
                result.private_events.push_back(InventoryUpdateEvent{attacker.dump_inventory()});
            } else {
                result.private_events.push_back(
                        ChatMsgEvent{ChatMsgType::SYSTEM, "",
                                     npc_target.get_name() + " solto " + item.name +
                                             " pero tu inventario esta lleno"});
            }
        }
    }

    return result;
}

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

    if (auto rejection = validate_pvp(attacker, target))
        return *rejection;

    if (!attacker.try_attack(current_tick, config.cooldown_ticks))
        return {};

    uint32_t range = config.attack_range_px;
    const InventorySlot& weapon_slot = attacker.get_equipped(EquipSlot::WEAPON);
    const Item* weapon_def = item_catalog_.find(weapon_slot.item_type);
    if (weapon_def && weapon_def->attack_range > 0)
        range = weapon_def->attack_range;

    return resolve_player_attack(attacker, target, target_id, range);
}

CommandResult CombatController::melee_attack(uint16_t attacker_id, uint16_t target_id,
                                             uint32_t current_tick) {
    auto target_it = enemy_npcs.find(target_id);
    if (target_it == enemy_npcs.end())
        return melee_attack_player(attacker_id, target_id, current_tick);
    else
        return melee_attack_npc(attacker_id, target_id, current_tick);
}

CommandResult CombatController::melee_attack_npc(uint16_t attacker_id, uint16_t npc_target_id,
                                                 uint32_t current_tick) {
    auto attacker_it = players.find(attacker_id);
    if (attacker_it == players.end())
        return {};

    auto npc_target_it = enemy_npcs.find(npc_target_id);
    if (npc_target_it == enemy_npcs.end())
        return {};

    Player& attacker = attacker_it->second;
    if (attacker.is_dead())
        return {};

    if (!attacker.try_attack(current_tick, config.cooldown_ticks))
        return {};

    uint32_t range = config.attack_range_px;
    const InventorySlot& weapon_slot = attacker.get_equipped(EquipSlot::WEAPON);
    const Item* weapon_def = item_catalog_.find(weapon_slot.item_type);
    if (weapon_def && weapon_def->attack_range > 0)
        range = weapon_def->attack_range;

    return resolve_npc_attack(attacker, npc_target_it->second, npc_target_id, range);
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

    if (auto rejection = validate_pvp(attacker, target))
        return *rejection;

    return resolve_player_attack(attacker, target, target_id, config.spell_attack_range_px);
}

CommandResult CombatController::spell_attack_npc(uint16_t attacker_id, uint16_t npc_target_id,
                                                 uint32_t) {
    auto attacker_it = players.find(attacker_id);
    if (attacker_it == players.end())
        return {};

    auto npc_target_it = enemy_npcs.find(npc_target_id);
    if (npc_target_it == enemy_npcs.end())
        return {};

    Player& attacker = attacker_it->second;
    if (attacker.is_dead())
        return {};

    return resolve_npc_attack(attacker, npc_target_it->second, npc_target_id,
                              config.spell_attack_range_px, /*include_item_drop=*/false);
}

Player* CombatController::get_nearest_player(const EnemyNpc& npc) {
    Player* nearest = nullptr;
    uint32_t nearest_dist_sq = UINT32_MAX;

    for (auto& [pid, player]: players) {
        if (player.is_dead() || player.get_current_map() != npc.get_current_map())
            continue;

        int dx = static_cast<int>(npc.pos_x()) - static_cast<int>(player.pos_x());
        int dy = static_cast<int>(npc.pos_y()) - static_cast<int>(player.pos_y());
        uint32_t dist_sq = static_cast<uint32_t>(dx * dx + dy * dy);

        if (dist_sq < nearest_dist_sq) {
            nearest_dist_sq = dist_sq;
            nearest = &player;
        }
    }

    return nearest;
}

void CombatController::drop_inventory_on_death(
        Player& target, std::map<std::string, std::vector<ItemDroppedEvent>>& drops,
        std::vector<ServerEvent>& target_events) {
    Position pos{target.pos_x(), target.pos_y()};
    auto& map_drops = drops[target.get_current_map()];
    for (const auto& slot: target.dump_inventory()) {
        if (slot.item_type == ItemType::NONE)
            continue;
        map_drops.push_back(ItemDroppedEvent{pos, slot.item_type, slot.item_name, 0});
    }
    target.clear_inventory();

    InventorySlot equipped[EQUIP_SLOT_COUNT];
    target.dump_equipped(equipped);
    for (const auto& slot: equipped) {
        if (slot.item_type == ItemType::NONE)
            continue;
        map_drops.push_back(ItemDroppedEvent{pos, slot.item_type, slot.item_name, 0});
    }
    target.clear_equipped();

    target_events.push_back(InventoryUpdateEvent{target.dump_inventory()});
    target_events.push_back(EntityEventFactory::make_equip_update(target.get_id(), target));
}

void CombatController::on_player_death(
        Player& victim, uint16_t /*victim_id*/, Player* killer,
        std::vector<ServerEvent>& victim_events,
        std::map<std::string, std::vector<ItemDroppedEvent>>& drops,
        std::vector<ServerEvent>* killer_events) {
    victim.lose_experience_on_death();

    PlayerStatsEvent stats{};
    fill_player_stats_event(stats, victim);
    victim_events.push_back(stats);

    uint32_t excess = victim.take_excess_gold();
    if (excess > 0) {
        if (killer) {
            killer->gain_gold(excess);
            if (killer_events) {
                killer_events->push_back(GoldUpdateEvent{killer->get_gold()});
                killer_events->push_back(
                        ChatMsgEvent{ChatMsgType::SYSTEM, "",
                                     "Le robaste " + std::to_string(excess) + " de oro a " +
                                             victim.get_name()});
            }
            victim_events.push_back(GoldUpdateEvent{victim.get_gold()});
            victim_events.push_back(
                    ChatMsgEvent{ChatMsgType::SYSTEM, "",
                                 killer->get_name() + " te robó " + std::to_string(excess) +
                                         " de oro"});
        } else {
            drops[victim.get_current_map()].push_back(
                    ItemDroppedEvent{Position{victim.pos_x(), victim.pos_y()}, ItemType::GOLD_DROP,
                                     "Oro", excess});
            victim_events.push_back(ChatMsgEvent{ChatMsgType::SYSTEM, "",
                                                 "Perdiste " + std::to_string(excess) +
                                                         " de oro al morir"});
        }
    }

    if (!killer)
        victim_events.push_back(GoldUpdateEvent{victim.get_gold()});

    drop_inventory_on_death(victim, drops, victim_events);
}

CommandResult CombatController::update_npc_ai(uint32_t current_tick) {
    std::vector<ServerEvent> broadcast;
    std::map<uint16_t, std::vector<ServerEvent>> targeted;
    std::map<std::string, std::vector<ItemDroppedEvent>> ground_drops;

    for (auto& [npc_id, npc]: enemy_npcs) {
        if (npc.is_dead())
            continue;

        Player* target = get_nearest_player(npc);
        if (!target)
            continue;

        bool in_attack_range = in_range(npc.pos_x(), npc.pos_y(), target->pos_x(), target->pos_y(),
                                        config.attack_range_px);
        bool in_vision_range = in_range(npc.pos_x(), npc.pos_y(), target->pos_x(), target->pos_y(),
                                        config.npc_vision_range_px);

        if (!in_vision_range)
            continue;

        const Map* map = nullptr;
        if (maps) {
            auto it = maps->find(npc.get_current_map());
            if (it != maps->end())
                map = &it->second;
        }

        bool player_in_safe_zone = map && map->is_safe_zone(target->pos_x(), target->pos_y());

        // Chase: move toward player if not in attack range or cooldown active, and player not in
        // safe zone
        bool should_chase = in_vision_range && !player_in_safe_zone;
        if (should_chase && !in_attack_range) {
            int dx = static_cast<int>(target->pos_x()) - static_cast<int>(npc.pos_x());
            int dy = static_cast<int>(target->pos_y()) - static_cast<int>(npc.pos_y());

            Direction move_dir;
            int step_x = 0, step_y = 0;
            int speed = static_cast<int>(npc.get_speed());

            if (std::abs(dx) > std::abs(dy)) {
                step_x = (dx > 0) ? speed : -speed;
                move_dir = (dx > 0) ? Direction::EAST : Direction::WEST;
            } else {
                step_y = (dy > 0) ? speed : -speed;
                move_dir = (dy > 0) ? Direction::SOUTH : Direction::NORTH;
            }

            int new_x = static_cast<int>(npc.pos_x()) + step_x;
            int new_y = static_cast<int>(npc.pos_y()) + step_y;

            if (new_x < 0)
                new_x = 0;
            if (new_y < 0)
                new_y = 0;

            // Don't enter safe zones or non-walkable tiles
            if (map && (map->is_safe_zone(new_x, new_y) || !map->is_walkable(new_x, new_y)))
                continue;

            npc.set_pos(static_cast<uint16_t>(new_x), static_cast<uint16_t>(new_y));
            npc.set_dir(move_dir);

            EntityMoveEvent move_ev{npc_id, npc.get_pos(), move_dir};
            for (uint16_t pid: PlayerRegistry(players).ids_on_map(npc.get_current_map())) {
                targeted[pid].push_back(move_ev);
            }
        }

        // Attack if in range, target not in a safe zone, and cooldown OK
        if (!in_attack_range || player_in_safe_zone)
            continue;

        if (!npc.try_attack(current_tick, config.cooldown_ticks))
            continue;

        uint16_t target_id = target->get_id();
        bool esquivado = GameFormulas::is_dodged(config, target->get_agility(), rng);

        if (esquivado) {
            AttackDodgedEvent dodged{target_id};
            targeted[target_id].push_back(dodged);
        } else {
            uint32_t damage = npc.get_damage();
            uint32_t defense = calculate_defense(*target);
            damage = damage > defense ? (damage - defense) : 0;

            target->take_damage(damage);

            DamageReceivedEvent received{target_id, npc_id, damage, target->get_hp_current(),
                                         target->get_hp_max()};
            targeted[target_id].push_back(received);

            ChatMsgEvent chat_msg{ChatMsgType::SYSTEM, "",
                                  npc.get_name() + " ataco a " + target->get_name() + " por " +
                                          std::to_string(damage) + " de dano"};
            targeted[target_id].push_back(chat_msg);
        }

        if (target->is_dead()) {
            on_player_death(*target, target_id, nullptr, targeted[target_id], ground_drops);
            broadcast.push_back(EntityDiedEvent{target_id});
        }
    }

    return {.private_events = {},
            .broadcast_events = std::move(broadcast),
            .targeted_events = std::move(targeted),
            .map_events = {},
            .ground_drops = std::move(ground_drops)};
}

bool CombatController::is_critical_attack(const Player& attacker) {
    return GameFormulas::is_critical(config, attacker.get_strength(), rng);
}

uint8_t CombatController::crit_chance_for(const Player& p) const {
    return GameFormulas::crit_chance_percent(config, p.get_strength());
}

std::pair<uint16_t, uint16_t> CombatController::unarmed_damage_range() const {
    return GameFormulas::unarmed_damage_range(config);
}

uint8_t CombatController::dodge_chance_for(const Player& p) const {
    return GameFormulas::dodge_chance_percent(config, p.get_agility());
}

void CombatController::fill_player_stats_event(PlayerStatsEvent& ev, const Player& p) const {
    ev.level = p.get_level();
    ev.experience = p.get_experience();
    ev.exp_to_next = p.exp_to_next_level();
    ev.hp_current = p.get_hp_current();
    ev.hp_max = p.get_hp_max();
    ev.mana_current = p.get_mana_current();
    ev.mana_max = p.get_mana_max();
    ev.crit_chance = crit_chance_for(p);

    auto [dmg_min, dmg_max] = GameFormulas::display_damage_range(p, item_catalog_, config);
    auto [def_min, def_max] = GameFormulas::display_defense_range(p, item_catalog_);
    ev.damage_min = dmg_min;
    ev.damage_max = dmg_max;
    ev.defense_min = def_min;
    ev.defense_max = def_max;
    ev.dodge_chance = dodge_chance_for(p);
    ev.strength = static_cast<uint16_t>(p.get_strength());
    ev.agility = static_cast<uint16_t>(p.get_agility());
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
        base = GameFormulas::weapon_damage(attacker.get_strength(), weapon->min_damage,
                                           weapon->max_damage, rng);
    } else {
        base = GameFormulas::unarmed_damage(config, rng);
    }
    return GameFormulas::apply_clan_bonus(base, get_clan_bonus(attacker));
}

uint32_t CombatController::calculate_defense(const Player& target) {
    const InventorySlot& armor_slot = target.get_equipped(EquipSlot::ARMOR);
    const InventorySlot& shield_slot = target.get_equipped(EquipSlot::SHIELD);
    const InventorySlot& helmet_slot = target.get_equipped(EquipSlot::HELMET);

    uint32_t base = calculate_object_defense(armor_slot) + calculate_object_defense(shield_slot) +
                    calculate_object_defense(helmet_slot);
    return GameFormulas::apply_clan_bonus(base, get_clan_bonus(target));
}

uint32_t CombatController::calculate_object_defense(const InventorySlot& object_slot) {
    if (object_slot.item_type == ItemType::NONE)
        return 0;
    const Item* object = item_catalog_.find(object_slot.item_type);
    return GameFormulas::object_defense(object->min_defense, object->max_defense, rng);
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
    return GameFormulas::clan_bonus(config, nearby_allies);
}

CommandResult CombatController::notify_entity_attacked(
        Player& attacker, uint16_t target_id, uint32_t damage, uint32_t target_hp_current,
        uint32_t target_hp_max, const std::string& target_name, const std::string& target_clan_name,
        bool target_is_dead, uint8_t target_level, bool esquivado) {
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
                              attacker.get_name() + " ataco a " + target_name + " por " +
                                      std::to_string(damage) + " de daño"};
        targeted[target_id].push_back(received);
        targeted[target_id].push_back(chat_msg);
        targeted[attacker.get_id()].push_back(chat_msg);

        if (target_is_dead) {
            EntityDiedEvent died{target_id};
            broadcast.push_back(died);
            broadcast.push_back(ChatMsgEvent{ChatMsgType::SYSTEM, "",
                                             attacker.get_name() + " mato a " + target_name});

            attacker.gain_experience(GameFormulas::bonus_kill_experience(
                    balance, target_hp_max, attacker.get_level(), target_level, rng));
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

    PlayerStatsEvent stats{};
    fill_player_stats_event(stats, attacker);
    return {.private_events = {dealt, stats},
            .broadcast_events = std::move(broadcast),
            .targeted_events = std::move(targeted)};
}
