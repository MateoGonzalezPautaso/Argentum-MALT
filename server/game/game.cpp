#include "game.h"

#include <cmath>
#include <cstdlib>
#include <format>
#include <string>
#include <utility>
#include <variant>
#include <vector>


#include "../../common/error_logger.h"
#include "../../common/visit.h"

#include "entity_event_factory.h"
#include "player_registry.h"
#include "prop_names.h"

namespace {

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
        clan_handler(clan_manager, players, player_name_index_, config.messages),
        move_step(config.move_step),
        sprite_width(config.sprite_width),
        sprite_height(config.sprite_height),
        balance(config.balance),
        inventory_config(config.inventory),
        mob_spawn(config.mob_spawn),
        item_catalog(config.item_catalog),
        rng(),
        next_npc_id(config.npc_id_base),
        msgs_(config.messages),
        combat_controller(config.attack, players, config.item_catalog, enemy_npcs,
                          config.balance.npc_drop, config.balance.npc_drop_dungeon, config.balance,
                          clan_manager, maps, msgs_),
        bank_service(players, maps, config.item_catalog, config.balance, msgs_),
        merchant_service(players, maps, config.item_catalog, config.balance, bank_service, msgs_),
        spawn_service(enemy_npcs, maps, next_npc_id, rng, players, config.balance,
                      config.item_catalog, config.mob_spawn, world_npc_templates,
                      dungeon_npc_templates),
        ground_item_service(players, maps, config.item_catalog, msgs_),
        map_transition_service(players, maps, enemy_npcs, player_data_service, config.balance,
                               config.sprite_width, config.sprite_height, ground_item_service),
        player_session_service(players, player_name_index_, player_data_service, maps, enemy_npcs,
                               config.balance, config.inventory, config.item_catalog, clan_manager,
                               clan_handler, combat_controller, ground_item_service),
        cheat_service(players, config.balance, config.item_catalog, player_data_service,
                      combat_controller, config.help_lines, config.cheats_enabled, msgs_),
        resurrection_service_(players, maps, enemy_npcs, pending_resurrections_,
                              ground_item_service, config.balance, config.messages,
                              config.sprite_width, config.sprite_height, config.tick_rate_hz),
        movement_service_(players, maps, enemy_npcs, pending_resurrections_, config.balance,
                          config.move_step, config.sprite_width, config.sprite_height,
                          map_transition_service),
        regen_service_(players, config.balance, config.tick_rate_hz,
                       hp_regen_accum, mana_regen_accum),
        spell_service_(players, enemy_npcs, maps, config.item_catalog, combat_controller,
                       ground_item_service, config.balance, config.messages),
        tick_rate_hz(config.tick_rate_hz),
        cheats_enabled(config.cheats_enabled),
        help_lines(config.help_lines) {
    for (const auto& tmpl: config.npc_templates) {
        if (tmpl.dungeon_only)
            dungeon_npc_templates.push_back(tmpl);
        else
            world_npc_templates.push_back(tmpl);
    }
    for (const auto& [name, tc]: config.tilemap_configs) {
        maps.emplace(name, Map(tc));
    }

}

Map& Game::player_map(const Player& p) {
    auto it = maps.find(p.get_current_map());
    if (it == maps.end()) {
        ErrorLogger::log("[Game] player_map: map '" + p.get_current_map() +
                         "' not found for player '" + p.get_name() + "' (id=" +
                         std::to_string(p.get_id()) + "), falling back to an arbitrary map");
        return maps.begin()->second;
    }
    return it->second;
}

const Map& Game::player_map(const Player& p) const {
    auto it = maps.find(p.get_current_map());
    if (it == maps.end()) {
        ErrorLogger::log("[Game] player_map: map '" + p.get_current_map() +
                         "' not found for player '" + p.get_name() + "' (id=" +
                         std::to_string(p.get_id()) + "), falling back to an arbitrary map");
        return maps.begin()->second;
    }
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
                    [&](const MoveCmd& cmd) { return movement_service_.handle_move(player_id, cmd); },
                    [&](const AttackCmd& cmd) { return handle_attack(player_id, cmd); },
                    [&](const SendChatMsgCmd& cmd) { return handle_send_chat_msg(player_id, cmd); },
                    [&](const MeditateCmd&) { return handle_meditate(player_id); },
                    [&](const ResurrectCmd&) { return resurrection_service_.handle_resurrect(player_id); },
                    [&](const CheatInfiniteHpCmd&) {
                        return cheat_service.dispatch(CheatType::INFINITE_HP, player_id);
                    },
                    [&](const CheatInfiniteManaCmd&) {
                        return cheat_service.dispatch(CheatType::INFINITE_MANA, player_id);
                    },
                    [&](const CheatDieCmd&) {
                        return cheat_service.dispatch(CheatType::DIE, player_id);
                    },
                    [&](const CheatLevelUpCmd&) {
                        return cheat_service.dispatch(CheatType::LEVEL_UP, player_id);
                    },
                    [&](const CheatLevelDownCmd&) {
                        return cheat_service.dispatch(CheatType::LEVEL_DOWN, player_id);
                    },
                    [&](const CheatAddGoldCmd&) {
                        return cheat_service.dispatch(CheatType::ADD_GOLD, player_id);
                    },
                    [&](const CheatResetGoldCmd&) {
                        return cheat_service.dispatch(CheatType::RESET_GOLD, player_id);
                    },
                    [&](const CheatVelocityCmd&) {
                        return cheat_service.dispatch(CheatType::VELOCITY, player_id);
                    },
                    [&](const CheatReviveCmd&) {
                        return cheat_service.dispatch(CheatType::REVIVE, player_id);
                    },
                    [&](const CheatFillInventoryCmd&) {
                        return cheat_service.dispatch(CheatType::FILL_INVENTORY, player_id);
                    },
                    [&](const CheatClearInventoryCmd&) {
                        return cheat_service.dispatch(CheatType::CLEAR_INVENTORY, player_id);
                    },
                    [&](const CheatResetManaCmd&) {
                        return cheat_service.dispatch(CheatType::RESET_MANA, player_id);
                    },
                    [&](const CastSpellCmd& cmd) { return spell_service_.handle_cast_spell(player_id, cmd, tick_count); },
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

CommandResult Game::tick() {
    ++tick_count;
    CommandResult result = regen_service_.apply_regen();

    result.merge(resurrection_service_.process_pending_resurrections());

    CommandResult npc_result = combat_controller.update_npc_ai(tick_count);
    ground_item_service.commit_ground_drops(npc_result, npc_result.ground_drops);
    result.merge(std::move(npc_result));

    result.merge(spawn_service.spawn_mobs());

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
        return CommandResult::with_msg(msgs_.attack_safe_zone);
    }
    result = combat_controller.melee_attack(player_id, cmd.target_id, tick_count);
    ground_item_service.commit_ground_drops(result, result.ground_drops);

    // Convert combat broadcasts to per-map events
    result.map_events = std::move(result.broadcast_events);
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
        return CommandResult::with_msg(msgs_.ghost_cant_interact);
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
            return CommandResult::with_msg(
                    std::vformat(msgs_.player_not_found, std::make_format_args(target_nick)));
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

        return CommandResult::with_msg(
                std::vformat(msgs_.command_not_recognized, std::make_format_args(cmd_name)));
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
        return CommandResult::with_msg(msgs_.warrior_cant_meditate);
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
    return PlayerRegistry(players).ids_on_map(map_name);
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

PlayerStatsEvent Game::make_player_stats_event(const Player& p) const {
    PlayerStatsEvent ev{};
    combat_controller.fill_player_stats_event(ev, p);
    return ev;
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

    EquipUpdateEvent equip_ev = EntityEventFactory::make_equip_update(player_id, player);
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

    EquipUpdateEvent equip_ev = EntityEventFactory::make_equip_update(player_id, player);
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
        return CommandResult::with_msg(msgs_.ghost_cant_be_healed);
    }

    const std::string& current_map = player.get_current_map();
    auto map_it = maps.find(current_map);
    if (map_it == maps.end())
        return {};

    const int range = map_it->second.tile_size() * balance.npc_interaction_range_tiles;
    const int px = static_cast<int>(player.pos_x());
    const int py = static_cast<int>(player.pos_y());

    if (!map_it->second.prop_grid().is_in_range_of(std::string(PropNames::PRIEST), px, py, range)) {
        return CommandResult::with_msg(msgs_.no_priest_nearby);
    }

    player.heal(player.get_hp_max());
    player.restore_mana(player.get_mana_max());

    HealReceivedEvent heal_ev{player_id, player.get_hp_current(), player.get_mana_current()};
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "", msgs_.priest_heal};
    return {.private_events = {heal_ev, msg}};
}


