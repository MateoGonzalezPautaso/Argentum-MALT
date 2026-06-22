#include "game.h"

#include <cmath>
#include <cstdlib>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include "../../common/visit.h"

#include "entity_event_factory.h"
#include "game_formulas.h"

namespace {

struct Delta {
    int dx;
    int dy;
};

Delta direction_to_delta(Direction dir, int step) {
    switch (dir) {
        case Direction::NORTH:
            return {0, -step};
        case Direction::SOUTH:
            return {0, step};
        case Direction::WEST:
            return {-step, 0};
        case Direction::EAST:
            return {step, 0};
    }
    return {0, 0};
}

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(' ');
    if (start == std::string::npos)
        return {};
    size_t end = s.find_last_not_of(' ');
    return s.substr(start, end - start + 1);
}

}  // namespace

Game::Game(const ServerConfig& config, PlayerDataService& player_data_service,
           ClanPersistence& clan_persistence):
        player_data_service(player_data_service),
        clan_manager(clan_persistence, config.clan),
        clan_handler(clan_manager, players, player_name_index_),
        tilemap_configs(config.tilemap_configs),
        move_step(config.move_step),
        sprite_width(config.sprite_width),
        sprite_height(config.sprite_height),
        balance(config.balance),
        inventory_config(config.inventory),
        mob_spawn(config.mob_spawn),
        item_catalog(config.item_catalog),
        rng(),
        next_npc_id(config.npc_id_base),
        combat_controller(config.attack, players, config.item_catalog, enemy_npcs,
                          config.balance.npc_drop, config.balance.npc_drop_dungeon, config.balance),
        bank_service(players, maps, config.item_catalog, config.balance),
        merchant_service(players, maps, config.item_catalog, config.balance, bank_service),
        spawn_service(enemy_npcs, maps, next_npc_id, rng, players, config.balance,
                      config.item_catalog, config.mob_spawn, world_npc_templates,
                      dungeon_npc_templates),
        ground_item_service(players, maps, config.item_catalog),
        map_transition_service(players, maps, enemy_npcs, player_data_service, config.balance,
                               config.sprite_width, config.sprite_height, ground_item_service),
        player_session_service(players, player_name_index_, player_data_service, maps, enemy_npcs,
                               config.balance, config.inventory, config.item_catalog, clan_manager,
                               clan_handler, combat_controller, ground_item_service),
        cheat_service(players, config.balance, config.item_catalog, player_data_service,
                      combat_controller, config.help_lines, config.cheats_enabled),
        tick_rate_hz(config.tick_rate_hz),
        cheats_enabled(config.cheats_enabled),
        help_lines(config.help_lines) {
    for (const auto& tmpl: config.npc_templates) {
        if (tmpl.dungeon_only)
            dungeon_npc_templates.push_back(tmpl);
        else
            world_npc_templates.push_back(tmpl);
    }
    for (const auto& [name, tc]: tilemap_configs) {
        maps.emplace(name, Map(tc));
    }
    combat_controller.set_clan_manager(clan_manager);
    combat_controller.set_maps(maps);
}

Map& Game::player_map(const Player& p) {
    auto it = maps.find(p.get_current_map());
    if (it == maps.end())
        return maps.begin()->second;
    return it->second;
}

const Map& Game::player_map(const Player& p) const {
    auto it = maps.find(p.get_current_map());
    if (it == maps.end())
        return maps.begin()->second;
    return it->second;
}

bool Game::target_in_safe_zone(uint16_t target_id) const {
    auto player_it = players.find(target_id);
    if (player_it != players.end())
        return player_map(player_it->second)
                .is_safe_zone(player_it->second.pos_x(), player_it->second.pos_y());

    auto npc_it = enemy_npcs.find(target_id);
    if (npc_it != enemy_npcs.end()) {
        auto map_it = maps.find(npc_it->second.get_current_map());
        if (map_it != maps.end())
            return map_it->second.is_safe_zone(npc_it->second.pos_x(), npc_it->second.pos_y());
    }

    return false;
}

CommandResult Game::process_command(uint16_t player_id, const ClientCommand& cmd) {
    return std::visit(
            overloaded{
                    [&](const LoginCmd& cmd) {
                        return player_session_service.handle_login(player_id, cmd);
                    },
                    [&](const CreateCharacterCmd& cmd) {
                        return player_session_service.handle_create_character(player_id, cmd);
                    },
                    [&](const MoveCmd& cmd) { return handle_move(player_id, cmd); },
                    [&](const AttackCmd& cmd) { return handle_attack(player_id, cmd); },
                    [&](const SendChatMsgCmd& cmd) { return handle_send_chat_msg(player_id, cmd); },
                    [&](const MeditateCmd&) { return handle_meditate(player_id); },
                    [&](const ResurrectCmd&) { return handle_resurrect(player_id); },
                    [&](const CheatInfiniteHpCmd&) {
                        return cheat_service.dispatch_cheat_infinite_hp(player_id);
                    },
                    [&](const CheatInfiniteManaCmd&) {
                        return cheat_service.dispatch_cheat_infinite_mana(player_id);
                    },
                    [&](const CheatDieCmd&) { return cheat_service.dispatch_cheat_die(player_id); },
                    [&](const CheatLevelUpCmd&) {
                        return cheat_service.dispatch_cheat_level_up(player_id);
                    },
                    [&](const CheatLevelDownCmd&) {
                        return cheat_service.dispatch_cheat_level_down(player_id);
                    },
                    [&](const CheatAddGoldCmd&) {
                        return cheat_service.dispatch_cheat_add_gold(player_id);
                    },
                    [&](const CheatResetGoldCmd&) {
                        return cheat_service.dispatch_cheat_reset_gold(player_id);
                    },
                    [&](const CheatVelocityCmd&) {
                        return cheat_service.dispatch_cheat_velocity(player_id);
                    },
                    [&](const CheatReviveCmd&) {
                        return cheat_service.dispatch_cheat_revive(player_id);
                    },
                    [&](const CheatFillInventoryCmd&) {
                        return cheat_service.dispatch_cheat_fill_inventory(player_id);
                    },
                    [&](const CheatClearInventoryCmd&) {
                        return cheat_service.dispatch_cheat_clear_inventory(player_id);
                    },
                    [&](const CheatResetManaCmd&) {
                        return cheat_service.dispatch_cheat_reset_mana(player_id);
                    },
                    [&](const CastSpellCmd& cmd) { return handle_cast_spell(player_id, cmd); },
                    [&](const ChangeMapCmd& cmd) {
                        return map_transition_service.handle_change_map(player_id, cmd);
                    },
                    [&](const EquipItemCmd& cmd) { return handle_equip(player_id, cmd); },
                    [&](const UnequipItemCmd& cmd) { return handle_unequip(player_id, cmd); },
                    [&](const NpcHealCmd&) { return handle_npc_heal(player_id); },
                    [&](const NpcListCmd&) { return merchant_service.handle_npc_list(player_id); },
                    [&](const BankDepositCmd& cmd) {
                        return bank_service.handle_bank_deposit(player_id, cmd);
                    },
                    [&](const BankWithdrawCmd& cmd) {
                        return bank_service.handle_bank_withdraw(player_id, cmd);
                    },
                    [&](const NpcBuyCmd& cmd) {
                        return merchant_service.handle_npc_buy(player_id, cmd);
                    },
                    [&](const NpcSellCmd& cmd) {
                        return merchant_service.handle_npc_sell(player_id, cmd);
                    },
                    [&](const PickupItemCmd& cmd) {
                        return ground_item_service.handle_pickup_item(player_id, cmd);
                    },
                    [&](const DropItemCmd& cmd) {
                        return ground_item_service.handle_drop_item(player_id, cmd);
                    },
                    [&](const ClanFoundCmd& cmd) {
                        return clan_handler.handle_found_clan(player_id, cmd.clan_name);
                    },
                    [&](const ClanJoinRequestCmd& cmd) {
                        return clan_handler.handle_join_clan(player_id, cmd.clan_name);
                    },
                    [&](const ClanReviewCmd&) {
                        return clan_handler.handle_clan_status(player_id);
                    },
                    [&](const ClanAcceptCmd& cmd) {
                        return clan_handler.handle_clan_accept(player_id, cmd.target_nick);
                    },
                    [&](const ClanRejectCmd& cmd) {
                        return clan_handler.handle_clan_reject(player_id, cmd.target_nick);
                    },
                    [&](const ClanBanCmd& cmd) {
                        return clan_handler.handle_clan_ban(player_id, cmd.target_nick);
                    },
                    [&](const ClanUnbanCmd& cmd) {
                        return clan_handler.handle_clan_unban(player_id, cmd.target_nick);
                    },
                    [&](const ClanKickCmd& cmd) {
                        return clan_handler.handle_clan_kick(player_id, cmd.target_nick);
                    },
                    [&](const ClanLeaveCmd&) { return clan_handler.handle_leave_clan(player_id); },
                    [](const auto&) { return CommandResult{}; },
            },
            cmd);
}

CommandResult Game::remove_player(uint16_t player_id) {
    pending_resurrections_.erase(player_id);
    return player_session_service.remove_player(player_id);
}

CommandResult Game::apply_regen() {
    CommandResult result;
    const double dt = 1.0 / tick_rate_hz;

    for (auto& [id, player]: players) {
        if (player.is_dead())
            continue;

        bool changed = false;
        double rate = GameFormulas::hp_regen_per_second(balance, player.get_race());

        // rate is in HP/second, but tick() runs 20 times per second.
        // Each tick only represents dt = 1/20 = 0.05 seconds,
        // so the per-tick gain is 1.0 * 0.05 = 0.05 HP
        hp_regen_accum[id] += rate * dt;
        if (hp_regen_accum[id] >= 1.0 && player.get_hp_current() < player.get_hp_max()) {
            uint32_t gain = static_cast<uint32_t>(hp_regen_accum[id]);
            player.heal(gain);
            hp_regen_accum[id] -= gain;
            changed = true;
        } else if (hp_regen_accum[id] >= 1.0) {
            hp_regen_accum[id] = 0.0;
        }

        double mana_rate = GameFormulas::mana_regen_per_second(balance, player.get_race());
        if (player.get_is_meditating())
            mana_rate += GameFormulas::meditation_mana_per_second(balance, player.get_race(),
                                                                  player.get_player_class());
        mana_regen_accum[id] += mana_rate * dt;
        if (mana_regen_accum[id] >= 1.0 && player.get_mana_current() < player.get_mana_max()) {
            uint32_t gain = static_cast<uint32_t>(mana_regen_accum[id]);
            player.restore_mana(gain);
            mana_regen_accum[id] -= gain;
            changed = true;
        } else if (mana_regen_accum[id] >= 1.0) {
            mana_regen_accum[id] = 0.0;
        }

        if (changed) {
            HealReceivedEvent ev{id, player.get_hp_current(), player.get_mana_current()};
            result.broadcast_events.push_back(ev);
        }
    }
    return result;
}

CommandResult Game::tick() {
    ++tick_count;
    CommandResult result = apply_regen();

    result.merge(process_pending_resurrections());

    CommandResult npc_result = combat_controller.update_npc_ai(tick_count);
    ground_item_service.commit_ground_drops(npc_result, npc_result.ground_drops);
    result.merge(std::move(npc_result));

    result.merge(spawn_service.spawn_mobs());

    return result;
}


CommandResult Game::process_pending_resurrections() {
    CommandResult result;

    for (auto it = pending_resurrections_.begin(); it != pending_resurrections_.end();) {
        auto& [player_id, pending] = *it;

        if (pending.remaining_ticks > 0) {
            --pending.remaining_ticks;
            ++it;
            continue;
        }

        auto player_it = players.find(player_id);
        if (player_it == players.end()) {
            it = pending_resurrections_.erase(it);
            continue;
        }

        Player& player = player_it->second;
        const std::string old_map = player.get_current_map();
        const bool needs_map_transition = (pending.target_map != old_map);

        if (needs_map_transition) {
            EntityDespawnEvent despawn{player_id};
            for (uint16_t pid: get_player_ids_on_map(old_map)) {
                if (pid != player_id)
                    result.targeted_events[pid].push_back(despawn);
            }

            player.set_current_map(pending.target_map);
            player.set_pos(pending.target_pos.x, pending.target_pos.y);

            player.resurrect();

            std::vector<ServerEvent> private_events;
            private_events.push_back(MapTransitionEvent{pending.target_map, pending.target_pos.x,
                                                        pending.target_pos.y});
            append_existing_entities(private_events, player_id, pending.target_map);
            result.targeted_events[player_id] = std::move(private_events);

            EntitySpawnEvent spawn = EntityEventFactory::make_entity_spawn(player);
            PlayerRespawnedEvent respawn{player_id, player.get_hp_current(), player.get_hp_max()};
            for (uint16_t pid: get_player_ids_on_map(pending.target_map)) {
                if (pid != player_id)
                    result.targeted_events[pid].push_back(spawn);
                result.targeted_events[pid].push_back(respawn);
            }
        } else {
            player.set_pos(pending.target_pos.x, pending.target_pos.y);
            player.resurrect();

            EntitySpawnEvent spawn = EntityEventFactory::make_entity_spawn(player);
            EntityMoveEvent move_ev{player_id, player.get_pos(), player.get_dir()};
            PlayerRespawnedEvent respawn{player_id, player.get_hp_current(), player.get_hp_max()};
            for (uint16_t pid: get_player_ids_on_map(pending.target_map)) {
                if (pid != player_id)
                    result.targeted_events[pid].push_back(spawn);
                result.targeted_events[pid].push_back(move_ev);
                result.targeted_events[pid].push_back(respawn);
            }
        }

        it = pending_resurrections_.erase(it);
    }

    return result;
}

CommandResult Game::handle_attack(uint16_t player_id, const AttackCmd& cmd) {
    auto it = players.find(player_id);
    if (it != players.end())
        it->second.set_meditating(false);
    CommandResult result;
    const Map& map = player_map(it->second);

    if (map.is_safe_zone(it->second.pos_x(), it->second.pos_y()) ||
        target_in_safe_zone(cmd.target_id)) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "No puedes atacar en una zona segura"};
        return {.private_events = {msg}};
    }
    result = combat_controller.melee_attack(player_id, cmd.target_id, tick_count);
    ground_item_service.commit_ground_drops(result, result.ground_drops);

    // Convert combat broadcasts to per-map events
    result.map_events = std::move(result.broadcast_events);
    return result;
}

CommandResult Game::handle_cast_spell(uint16_t player_id, const CastSpellCmd& cmd) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};
    Player& player = it->second;
    if (player.is_dead())
        return {};

    if (player.get_player_class() == PlayerClass::WARRIOR) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Los guerreros no pueden usar magia"};
        return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
    }

    const InventorySlot& weapon_slot = player.get_equipped(EquipSlot::WEAPON);
    if (weapon_slot.item_type == ItemType::NONE) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "No tienes un arma equipada"};
        return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
    }

    const Item* item = item_catalog.find(weapon_slot.item_type);
    if (!item || item->mana_consumed == 0) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "El arma equipada no es magica"};
        return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
    }

    if (player.get_mana_current() < item->mana_consumed) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Mana insuficiente"};
        return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
    }

    const Map& map = player_map(player);
    if (map.is_safe_zone(player.pos_x(), player.pos_y())) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "No puedes lanzar hechizos en una zona segura"};
        return {.private_events = {msg}};
    }

    player.use_mana(item->mana_consumed);
    player.set_meditating(false);

    if (weapon_slot.item_type == ItemType::ELVEN_FLUTE) {
        uint32_t heal_amount = GameFormulas::spell_self_heal(player.get_hp_max());
        player.heal(heal_amount);
        HealReceivedEvent heal_ev{player_id, player.get_hp_current(), player.get_mana_current()};
        PlayerStatsEvent stats = make_player_stats_event(player);
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Te has curado!"};
        DamageDealtEvent spell_ev{player_id, 0};
        SpellEffectEvent effect_ev{player_id, 0};
        return {.private_events = {msg, heal_ev, stats, spell_ev},
                .broadcast_events = {},
                .targeted_events = {},
                .map_events = {effect_ev}};
    }

    if (cmd.target_id == player_id) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "No puedes atacarte a ti mismo"};
        return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
    }

    if (target_in_safe_zone(cmd.target_id)) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "No puedes lanzar hechizos en una zona segura"};
        return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
    }

    CommandResult result;
    auto target_it = enemy_npcs.find(cmd.target_id);
    if (target_it == enemy_npcs.end())
        result = combat_controller.spell_attack_player(player_id, cmd.target_id, tick_count);
    else
        result = combat_controller.spell_attack_npc(player_id, cmd.target_id, tick_count);
    ground_item_service.commit_ground_drops(result, result.ground_drops);
    result.map_events = std::move(result.broadcast_events);
    uint8_t effect_type = item->spell_effect_id;
    if (effect_type == 0)
        effect_type = balance.default_spell_effect_id;
    result.map_events.push_back(SpellEffectEvent{cmd.target_id, effect_type});
    return result;
}

CommandResult Game::handle_send_chat_msg(uint16_t player_id, const SendChatMsgCmd& cmd) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    const std::string& text = cmd.text;
    if (text.empty())
        return {};

    if (it->second.is_dead()) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Los fantasmas no pueden interactuar"};
        return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
    }

    it->second.set_meditating(false);

    const std::string& sender_name = it->second.get_name();

    if (text[0] == '@') {
        size_t space_pos = text.find(' ');
        if (space_pos != std::string::npos) {
            std::string target_nick = text.substr(1, space_pos - 1);
            std::string msg = text.substr(space_pos + 1);
            auto target_id_opt = find_player_id_by_name(target_nick);
            if (target_id_opt) {
                uint16_t target_id = *target_id_opt;
                ChatMsgEvent chat_ev{ChatMsgType::PRIVATE, sender_name, msg, target_id, player_id};
                std::map<uint16_t, std::vector<ServerEvent>> targeted;
                targeted[target_id].push_back(chat_ev);
                targeted[player_id].push_back(chat_ev);
                return {.private_events = {},
                        .broadcast_events = {},
                        .targeted_events = std::move(targeted)};
            }
            ChatMsgEvent err{ChatMsgType::SYSTEM, "", "Jugador " + target_nick + " no encontrado"};
            return {.private_events = {err}, .broadcast_events = {}, .targeted_events = {}};
        }
    }

    if (text[0] == '/') {
        size_t space_pos = text.find(' ');
        std::string cmd_name = text.substr(0, space_pos);
        std::string args = (space_pos != std::string::npos) ? trim(text.substr(space_pos + 1)) : "";

        if (auto result = clan_handler.handle(player_id, cmd_name, args))
            return *result;

        if (cmd_name == "/help")
            return cheat_service.handle_help();

        ChatMsgEvent ev{ChatMsgType::SYSTEM, "", "Comando " + cmd_name + " no reconocido"};
        return {.private_events = {ev}, .broadcast_events = {}, .targeted_events = {}};
    }

    ChatMsgEvent broadcast_ev{ChatMsgType::SAY, sender_name, text};
    return {.private_events = {}, .broadcast_events = {broadcast_ev}, .targeted_events = {}};
}


CommandResult Game::handle_meditate(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    Player& player = it->second;

    if (player.is_dead())
        return {};

    if (player.get_player_class() == PlayerClass::WARRIOR) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Los guerreros no pueden meditar"};
        return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
    }

    if (player.get_is_meditating()) {
        player.set_meditating(false);
        return {.private_events = {MeditationStopEvent{}},
                .broadcast_events = {},
                .targeted_events = {}};
    } else {
        player.set_meditating(true);
        return {.private_events = {MeditationStartEvent{}},
                .broadcast_events = {},
                .targeted_events = {}};
    }
}

std::vector<ServerEvent> Game::make_existing_spawns(uint16_t exclude_id) const {
    std::vector<ServerEvent> spawns;
    for (const auto& [id, player]: players) {
        if (id == exclude_id)
            continue;
        spawns.push_back(EntityEventFactory::make_entity_spawn(player));
    }
    for (const auto& [id, npc]: enemy_npcs) {
        if (npc.is_dead())
            continue;
        spawns.push_back(EntityEventFactory::make_npc_spawn(npc, id));
    }
    return spawns;
}

std::vector<ServerEvent> Game::make_existing_spawns(uint16_t exclude_id,
                                                    const std::string& map_name) const {
    std::vector<ServerEvent> spawns;
    for (const auto& [id, player]: players) {
        if (id == exclude_id)
            continue;
        if (player.get_current_map() != map_name)
            continue;
        spawns.push_back(EntityEventFactory::make_entity_spawn(player));
    }
    for (const auto& [id, npc]: enemy_npcs) {
        if (npc.is_dead())
            continue;
        if (npc.get_current_map() != map_name)
            continue;
        spawns.push_back(EntityEventFactory::make_npc_spawn(npc, id));
    }
    return spawns;
}

std::string Game::get_player_map_name(uint16_t player_id) const {
    auto it = players.find(player_id);
    if (it == players.end())
        return balance.starting_map;
    return it->second.get_current_map();
}

void Game::append_existing_entities(std::vector<ServerEvent>& events, uint16_t exclude_id,
                                    const std::string& map_name) const {
    std::vector<ServerEvent> spawns = make_existing_spawns(exclude_id, map_name);
    events.insert(events.end(), spawns.begin(), spawns.end());

    std::vector<ServerEvent> items = ground_item_service.make_existing_ground_items(map_name);
    events.insert(events.end(), items.begin(), items.end());
}


std::vector<uint16_t> Game::get_player_ids_on_map(const std::string& map_name) const {
    std::vector<uint16_t> ids;
    for (const auto& [id, player]: players) {
        if (player.get_current_map() == map_name)
            ids.push_back(id);
    }
    return ids;
}

void Game::save_all_players() {
    for (const auto& [id, player]: players) {
        player_data_service.save_player(player);
    }
}

std::optional<uint16_t> Game::find_player_id_by_name(const std::string& name) const {
    auto it = player_name_index_.find(name);
    if (it == player_name_index_.end())
        return std::nullopt;
    return it->second;
}

std::tuple<uint16_t, uint16_t, uint16_t, uint16_t> Game::compute_combat_ranges(
        const Player& p) const {
    auto [dmg_min, dmg_max] = GameFormulas::display_damage_range(
            p, item_catalog, combat_controller.get_attack_config());
    auto [def_min, def_max] = GameFormulas::display_defense_range(p, item_catalog);
    return {dmg_min, dmg_max, def_min, def_max};
}

PlayerStatsEvent Game::make_player_stats_event(const Player& p) const {
    auto [dmg_min, dmg_max, def_min, def_max] = compute_combat_ranges(p);
    return {.level = p.get_level(),
            .experience = p.get_experience(),
            .exp_to_next = p.exp_to_next_level(),
            .hp_current = p.get_hp_current(),
            .hp_max = p.get_hp_max(),
            .mana_current = p.get_mana_current(),
            .mana_max = p.get_mana_max(),
            .crit_chance = combat_controller.crit_chance_for(p),
            .damage_min = dmg_min,
            .damage_max = dmg_max,
            .defense_min = def_min,
            .defense_max = def_max,
            .dodge_chance = combat_controller.dodge_chance_for(p),
            .strength = static_cast<uint16_t>(p.get_strength()),
            .agility = static_cast<uint16_t>(p.get_agility())};
}

CommandResult Game::handle_resurrect(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    Player& player = it->second;
    if (!player.is_dead()) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "No estás muerto"};
        return {.private_events = {msg}};
    }

    if (pending_resurrections_.contains(player_id)) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Ya estás resucitando, espera"};
        return {.private_events = {msg}};
    }

    const std::string& current_map = player.get_current_map();
    auto map_it = maps.find(current_map);
    if (map_it == maps.end())
        return {};

    const int tile_size = map_it->second.tile_size();
    const int px = static_cast<int>(player.pos_x());
    const int py = static_cast<int>(player.pos_y());
    const int range = tile_size * balance.npc_interaction_range_tiles;

    // If the ghost is near a sacerdote, resurrect immediately
    if (map_it->second.prop_grid().is_in_range_of("sacerdote", px, py, range)) {
        player.resurrect();
        PlayerRespawnedEvent respawn{player_id, player.get_hp_current(), player.get_hp_max()};
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Sacerdote: ¡Que la luz te devuelva a la vida!"};
        CommandResult result;
        result.private_events = {msg};
        result.map_events = {respawn};
        return result;
    }


    std::string target_map;
    uint32_t wait_ticks;
    int san_cx, san_cy;

    const PropGrid::Entry* san_entry = map_it->second.prop_grid().find_closest("sanadora", px, py);
    if (san_entry) {
        target_map = current_map;
        san_cx = san_entry->center_x;
        san_cy = san_entry->center_y;
        int dx = px - san_cx;
        int dy = py - san_cy;
        int dist_px = static_cast<int>(std::sqrt(static_cast<double>(dx * dx + dy * dy)));
        int dist_tiles = dist_px / tile_size;
        wait_ticks = static_cast<uint32_t>(dist_tiles * tick_rate_hz);
    } else {
        auto main_it = maps.find(balance.starting_map);
        if (main_it == maps.end())
            return {};
        san_entry = main_it->second.prop_grid().find_closest("sanadora", 0, 0);
        if (!san_entry)
            return {};
        target_map = balance.starting_map;
        san_cx = san_entry->center_x;
        san_cy = san_entry->center_y;
        wait_ticks = static_cast<uint32_t>(balance.default_resurrect_wait_seconds * tick_rate_hz);
    }

    Position target_pos{static_cast<uint16_t>(san_cx - sprite_width / 2),
                        static_cast<uint16_t>(san_cy - sprite_height)};

    pending_resurrections_[player_id] = {wait_ticks, target_map, target_pos};

    uint32_t remaining_tiles = wait_ticks / tick_rate_hz;
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                     "Resucitando en " + std::to_string(remaining_tiles) +
                             " segundos... Permanece inmóvil."};

    return {.private_events = {msg}};
}

CommandResult Game::handle_equip(uint16_t player_id, const EquipItemCmd& cmd) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    Player& player = it->second;
    player.set_meditating(false);

    bool consumed_potion = false;
    std::vector<InventorySlot> inv_before = player.dump_inventory();
    if (cmd.slot_index < inv_before.size()) {
        const Item* def = item_catalog.find(inv_before[cmd.slot_index].item_type);
        if (def && def->equip_slot == EquipSlot::CONSUMABLE &&
            (def->restore_hp_percent > 0 || def->restore_mana_percent > 0))
            consumed_potion = true;
    }

    bool changed = player.equip(cmd.slot_index, item_catalog);
    if (!changed)
        return {};

    InventorySlot equipped_slots[EQUIP_SLOT_COUNT];
    player.dump_equipped(equipped_slots);
    EquipUpdateEvent equip_ev{player_id, equipped_slots[0], equipped_slots[1], equipped_slots[2],
                              equipped_slots[3]};
    InventoryUpdateEvent inv_ev{player.dump_inventory()};

    std::vector<ServerEvent> private_events{inv_ev};
    if (consumed_potion)
        private_events.push_back(
                HealReceivedEvent{player_id, player.get_hp_current(), player.get_mana_current()});
    return {.private_events = private_events,
            .broadcast_events = {},
            .targeted_events = {},
            .map_events = {equip_ev, make_player_stats_event(player)}};
}

CommandResult Game::handle_unequip(uint16_t player_id, const UnequipItemCmd& cmd) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    Player& player = it->second;
    player.set_meditating(false);

    player.unequip(cmd.slot);

    InventorySlot equipped_slots[EQUIP_SLOT_COUNT];
    player.dump_equipped(equipped_slots);
    EquipUpdateEvent equip_ev{player_id, equipped_slots[0], equipped_slots[1], equipped_slots[2],
                              equipped_slots[3]};
    InventoryUpdateEvent inv_ev{player.dump_inventory()};

    return {.private_events = {inv_ev},
            .broadcast_events = {},
            .targeted_events = {},
            .map_events = {equip_ev, make_player_stats_event(player)}};
}

CommandResult Game::handle_npc_heal(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    Player& player = it->second;

    if (player.is_dead()) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Los fantasmas no pueden ser curados"};
        return {.private_events = {msg}};
    }

    const std::string& current_map = player.get_current_map();
    auto map_it = maps.find(current_map);
    if (map_it == maps.end())
        return {};

    const int range = map_it->second.tile_size() * balance.npc_interaction_range_tiles;
    const int px = static_cast<int>(player.pos_x());
    const int py = static_cast<int>(player.pos_y());

    if (!map_it->second.prop_grid().is_in_range_of("sacerdote", px, py, range)) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "No hay un sacerdote cerca"};
        return {.private_events = {msg}};
    }

    player.heal(player.get_hp_max());
    player.restore_mana(player.get_mana_max());

    HealReceivedEvent heal_ev{player_id, player.get_hp_current(), player.get_mana_current()};
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Sacerdote: ¡Que la luz te sane!"};
    return {.private_events = {heal_ev, msg}};
}


CommandResult Game::handle_move(uint16_t player_id, const MoveCmd& cmd) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    Player& player = it->second;
    player.set_meditating(false);

    if (pending_resurrections_.contains(player_id))
        return {};

    Map& cur_map = player_map(player);

    int effective_step = player.has_cheat_fast_velocity() ? move_step * 2 : move_step;
    auto [dx, dy] = direction_to_delta(cmd.direction, effective_step);

    const int current_x = static_cast<int>(player.pos_x());
    const int current_y = static_cast<int>(player.pos_y());
    const int new_x = cur_map.clamp_x(current_x + dx, sprite_width);
    const int new_y = cur_map.clamp_y(current_y + dy, sprite_height);

    if (!cur_map.is_walkable(new_x + sprite_width / 2, new_y + sprite_height))
        return {};

    for (const auto& [other_id, other]: players) {
        if (other_id == player_id)
            continue;
        if (other.get_current_map() != player.get_current_map())
            continue;
        const int ox = static_cast<int>(other.pos_x());
        const int oy = static_cast<int>(other.pos_y());
        const int hw = sprite_width / 2;
        const int hh = sprite_height / 2;
        bool already_overlapping = (std::abs(current_x - ox) < hw && std::abs(current_y - oy) < hh);
        if (already_overlapping)
            continue;
        if (std::abs(new_x - ox) < hw && std::abs(new_y - oy) < hh)
            return {};
    }

    for (const auto& [npc_id, npc]: enemy_npcs) {
        if (npc.is_dead())
            continue;
        if (npc.get_current_map() != player.get_current_map())
            continue;
        const int nx = static_cast<int>(npc.pos_x());
        const int ny = static_cast<int>(npc.pos_y());
        const int hw = sprite_width / 2;
        const int hh = sprite_height / 2;
        bool already_overlapping = (std::abs(current_x - nx) < hw && std::abs(current_y - ny) < hh);
        if (already_overlapping)
            continue;
        if (std::abs(new_x - nx) < hw && std::abs(new_y - ny) < hh)
            return {};
    }

    const int final_dx = new_x - current_x;
    const int final_dy = new_y - current_y;
    if (final_dx == 0 && final_dy == 0)
        return {};

    player.apply_move(cmd.direction, final_dx, final_dy);

    CommandResult result;

    if (map_transition_service.try_map_transition(player, result))
        return result;

    EntityMoveEvent move{
            .entity_id = player.get_id(),
            .entity_pos = player.get_pos(),
            .entity_dir = player.get_dir(),
    };
    result.map_events = {move};
    return result;
}
