#include "game.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "../../common/visit.h"

#include "basic_skeleton.h"
#include "big_spider.h"
#include "iron_golem.h"
#include "orc.h"
#include "small_spider.h"
#include "stone_golem.h"
#include "strong_goblin.h"
#include "warrior_skeleton.h"
#include "weak_goblin.h"
#include "zombie.h"

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

bool vendor_sells(const VendorsConfig& vendors, const std::string& vendor, ItemType type) {
    auto it = vendors.by_vendor.find(vendor);
    return it != vendors.by_vendor.end() && it->second.count(type) > 0;
}

}  // namespace

Game::Game(const ServerConfig& config, PlayerDataService& player_data_service,
           ClanPersistence& clan_persistence):
        player_data_service(player_data_service),
        clan_manager(clan_persistence, config.clan),
        clan_handler(clan_manager, players),
        tilemap_configs(config.tilemap_configs),
        move_step(config.move_step),
        sprite_width(config.sprite_width),
        sprite_height(config.sprite_height),
        balance(config.balance),
        inventory_config(config.inventory),
        item_catalog(config.item_catalog),
        rng(),
        combat_controller(config.attack, players, config.item_catalog, enemy_npcs,
                          config.balance.npc_drop),
        tick_rate_hz(config.tick_rate_hz),
        cheats_enabled(config.cheats_enabled) {
    for (const auto& [name, tc]: tilemap_configs) {
        maps.emplace(name, Map(tc));
    }
    combat_controller.set_clan_manager(clan_manager);

    auto spawn_initial = [&](uint16_t id, uint8_t level, auto maker) {
        auto city_it = maps.find("city");
        if (city_it == maps.end())
            return;
        auto pos_opt = city_it->second.find_random_mob_spawn_pos(rng);
        if (!pos_opt.has_value())
            return;
        Position pos{static_cast<uint16_t>(pos_opt->first),
                     static_cast<uint16_t>(pos_opt->second)};
        auto npc = maker(pos, rng, item_catalog, level);
        npc.set_current_map("city");
        enemy_npcs.emplace(id, std::move(npc));
    };

    spawn_initial(1001, 2, [](Position p, Rng& r, const ItemCatalog& c, uint8_t l) {
        return WeakGoblin(p, r, c, l);
    });
    spawn_initial(1002, 1, [](Position p, Rng& r, const ItemCatalog& c, uint8_t l) {
        return WeakGoblin(p, r, c, l);
    });
    spawn_initial(1003, 5, [](Position p, Rng& r, const ItemCatalog& c, uint8_t l) {
        return Orc(p, r, c, l);
    });
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

CommandResult Game::process_command(uint16_t player_id, const ClientCommand& cmd) {
    return std::visit(
            overloaded{
                    [&](const LoginCmd& cmd) { return handle_login(player_id, cmd); },
                    [&](const CreateCharacterCmd& cmd) {
                        return handle_create_character(player_id, cmd);
                    },
                    [&](const MoveCmd& cmd) { return handle_move(player_id, cmd); },
                    [&](const AttackCmd& cmd) { return handle_attack(player_id, cmd); },
                    [&](const SendChatMsgCmd& cmd) { return handle_send_chat_msg(player_id, cmd); },
                    [&](const MeditateCmd&) { return handle_meditate(player_id); },
                    [&](const ResurrectCmd&) { return handle_resurrect(player_id); },
                    [&](const CheatInfiniteHpCmd&) {
                        return cheats_enabled ? handle_cheat_infinite_hp(player_id) :
                                                CommandResult{};
                    },
                    [&](const CheatInfiniteManaCmd&) {
                        return cheats_enabled ? handle_cheat_infinite_mana(player_id) :
                                                CommandResult{};
                    },
                    [&](const CheatDieCmd&) {
                        return cheats_enabled ? handle_cheat_die(player_id) : CommandResult{};
                    },
                    [&](const CheatLevelUpCmd&) {
                        return cheats_enabled ? handle_cheat_level_up(player_id) : CommandResult{};
                    },
                    [&](const CheatLevelDownCmd&) {
                        return cheats_enabled ? handle_cheat_level_down(player_id) :
                                                CommandResult{};
                    },
                    [&](const CheatAddGoldCmd&) {
                        return cheats_enabled ? handle_cheat_add_gold(player_id) : CommandResult{};
                    },
                    [&](const CheatResetGoldCmd&) {
                        return cheats_enabled ? handle_cheat_reset_gold(player_id) :
                                                CommandResult{};
                    },
                    [&](const CheatVelocityCmd&) {
                        return cheats_enabled ? handle_cheat_velocity(player_id) : CommandResult{};
                    },
                    [&](const CheatReviveCmd&) {
                        return cheats_enabled ? handle_cheat_revive(player_id) : CommandResult{};
                    },
                    [&](const CheatFillInventoryCmd&) {
                        return cheats_enabled ? handle_cheat_fill_inventory(player_id) :
                                                CommandResult{};
                    },
                    [&](const CheatClearInventoryCmd&) {
                        return cheats_enabled ? handle_cheat_clear_inventory(player_id) :
                                                CommandResult{};
                    },
                    [&](const CheatResetManaCmd&) {
                        return cheats_enabled ? handle_cheat_reset_mana(player_id) :
                                                CommandResult{};
                    },
                    [&](const CastSpellCmd& cmd) { return handle_cast_spell(player_id, cmd); },
                    [&](const ChangeMapCmd& cmd) { return handle_change_map(player_id, cmd); },
                    [&](const EquipItemCmd& cmd) { return handle_equip(player_id, cmd); },
                    [&](const UnequipItemCmd& cmd) { return handle_unequip(player_id, cmd); },
                    [&](const NpcHealCmd&) { return handle_npc_heal(player_id); },
                    [](const auto&) { return CommandResult{}; },
            },
            cmd);
}

CommandResult Game::remove_player(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    std::string map_name = it->second.get_current_map();
    std::string clan_name = it->second.get_clan_name();
    std::string username = it->second.get_name();
    player_data_service.save_player(it->second);

    EntityDespawnEvent despawn{.entity_id = player_id};

    // Send despawn to all players on the same map
    CommandResult result;
    for (uint16_t pid: get_player_ids_on_map(map_name)) {
        if (pid == player_id)
            continue;
        result.targeted_events[pid].push_back(despawn);
    }

    pending_resurrections_.erase(player_id);
    players.erase(it);

    // Notify clan members of logout
    if (!clan_name.empty()) {
        ClanNotificationEvent notif{ClanNotifType::MEMBER_OFFLINE, username, clan_name};
        auto clan_result = clan_handler.notify_clan_members(notif.clan_name, notif, player_id);
        for (auto& ev: clan_result.targeted_events)
            for (auto& se: ev.second) result.targeted_events[ev.first].push_back(std::move(se));
    }

    return result;
}

double Game::recovery_rate_for(Race race) const {
    const auto& r = balance.race_recovery;
    switch (race) {
        case Race::HUMAN:
            return r.human;
        case Race::ELF:
            return r.elf;
        case Race::DWARF:
            return r.dwarf;
        case Race::GNOME:
            return r.gnome;
    }
    return r.human;
}

double Game::intelligence_for(Race race) const {
    const auto& m = balance.mana;
    switch (race) {
        case Race::HUMAN:
            return m.intelligence_human;
        case Race::ELF:
            return m.intelligence_elf;
        case Race::DWARF:
            return m.intelligence_dwarf;
        case Race::GNOME:
            return m.intelligence_gnome;
    }
    return m.intelligence_human;
}

double Game::meditation_factor_for(PlayerClass player_class) const {
    const auto& m = balance.mana;
    switch (player_class) {
        case PlayerClass::WARRIOR:
            return m.class_meditation_factor_warrior;
        case PlayerClass::PALADIN:
            return m.class_meditation_factor_paladin;
        case PlayerClass::CLERIC:
            return m.class_meditation_factor_cleric;
        case PlayerClass::MAGE:
            return m.class_meditation_factor_mage;
    }
    return m.class_meditation_factor_warrior;
}

CommandResult Game::apply_regen() {
    CommandResult result;
    const double dt = 1.0 / tick_rate_hz;

    for (auto& [id, player]: players) {
        if (player.is_dead())
            continue;

        bool changed = false;
        double rate = recovery_rate_for(player.get_race());

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

        double mana_rate = rate;
        if (player.get_is_meditating())
            mana_rate += meditation_factor_for(player.get_player_class()) *
                         intelligence_for(player.get_race());
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
    CommandResult res_result = process_pending_resurrections();

    for (auto& ev: res_result.targeted_events)
        for (auto& se: ev.second) result.targeted_events[ev.first].push_back(std::move(se));
    for (auto& ev: res_result.broadcast_events) result.broadcast_events.push_back(std::move(ev));

    CommandResult npc_result = combat_controller.update_npc_ai(tick_count);
    for (auto& ev: npc_result.targeted_events)
        for (auto& se: ev.second) result.targeted_events[ev.first].push_back(std::move(se));
    for (auto& ev: npc_result.broadcast_events) result.broadcast_events.push_back(std::move(ev));

    CommandResult spawn_result = spawn_mobs();
    for (auto& ev: spawn_result.broadcast_events) result.broadcast_events.push_back(std::move(ev));

    return result;
}

CommandResult Game::spawn_mobs() {
    ++mob_spawn_tick;
    CommandResult result;

    for (auto& [map_name, map]: maps) {
        const TilemapConfig& cfg = map.config();
        if (cfg.mob_spawn_limit <= 0 || cfg.mob_spawn_interval_ticks <= 0)
            continue;

        if (mob_spawn_tick % cfg.mob_spawn_interval_ticks != 0)
            continue;

        int npc_count = 0;
        for (const auto& [id, npc]: enemy_npcs) {
            if (!npc.is_dead() && npc.get_current_map() == map_name)
                ++npc_count;
        }

        if (npc_count >= cfg.mob_spawn_limit)
            continue;

        auto pos_opt = map.find_random_mob_spawn_pos(rng);
        if (!pos_opt.has_value())
            continue;

        Position spawn_pos{static_cast<uint16_t>(pos_opt->first),
                           static_cast<uint16_t>(pos_opt->second)};

        uint8_t level = static_cast<uint8_t>(rng.get_random_int(1, 5));
        EnemyNpc npc = create_random_npc(spawn_pos, level);
        uint16_t npc_id = next_npc_id++;
        npc.set_current_map(map_name);

        EntitySpawnEvent spawn_ev = make_npc_spawn(npc, npc_id);
        auto it = enemy_npcs.emplace(npc_id, std::move(npc)).first;
        (void)it;

        for (uint16_t pid: get_player_ids_on_map(map_name))
            result.broadcast_events.push_back(spawn_ev);
    }

    return result;
}

EnemyNpc Game::create_random_npc(Position pos, uint8_t level) {
    int roll = rng.get_random_int(0, 9);
    switch (roll) {
        case 0:
            return WeakGoblin(pos, rng, item_catalog, level);
        case 1:
            return StrongGoblin(pos, rng, item_catalog, level);
        case 2:
            return BasicSkeleton(pos, rng, item_catalog, level);
        case 3:
            return WarriorSkeleton(pos, rng, item_catalog, level);
        case 4:
            return Zombie(pos, rng, item_catalog, level);
        case 5:
            return Orc(pos, rng, item_catalog, level);
        case 6:
            return SmallSpider(pos, rng, item_catalog, level);
        case 7:
            return BigSpider(pos, rng, item_catalog, level);
        case 8:
            return StoneGolem(pos, rng, item_catalog, level);
        case 9:
        default:
            return IronGolem(pos, rng, item_catalog, level);
    }
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

            std::vector<ServerEvent> existing = make_existing_spawns(player_id, pending.target_map);
            existing.insert(existing.begin(),
                            MapTransitionEvent{pending.target_map, pending.target_pos.x,
                                               pending.target_pos.y});
            result.targeted_events[player_id] = std::move(existing);

            EntitySpawnEvent spawn = make_entity_spawn(player);
            PlayerRespawnedEvent respawn{player_id, player.get_hp_current(), player.get_hp_max()};
            for (uint16_t pid: get_player_ids_on_map(pending.target_map)) {
                if (pid == player_id)
                    continue;
                result.targeted_events[pid].push_back(spawn);
                result.targeted_events[pid].push_back(respawn);
            }
        } else {
            player.set_pos(pending.target_pos.x, pending.target_pos.y);
            player.resurrect();

            EntityMoveEvent move_ev{player_id, player.get_pos(), player.get_dir()};
            PlayerRespawnedEvent respawn{player_id, player.get_hp_current(), player.get_hp_max()};
            for (uint16_t pid: get_player_ids_on_map(pending.target_map)) {
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
    if (map.is_position_in_spawn_zone(it->second.pos_x(), it->second.pos_y())) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "No puedes atacar en una zona segura"};
        return {.private_events = {msg}};
    }
    result = combat_controller.melee_attack(player_id, cmd.target_id, tick_count);

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
    if (!map.is_position_in_spawn_zone(player.pos_x(), player.pos_y())) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "No puedes lanzar hechizos en una zona segura"};
        return {.private_events = {msg}};
    }

    player.use_mana(item->mana_consumed);
    player.set_meditating(false);

    if (weapon_slot.item_type == ItemType::ELVEN_FLUTE) {
        uint32_t heal_amount = player.get_hp_max() / 2;
        player.heal(heal_amount);
        HealReceivedEvent heal_ev{player_id, player.get_hp_current(), player.get_mana_current()};
        PlayerStatsEvent stats{.level = player.get_level(),
                               .experience = player.get_experience(),
                               .exp_to_next = player.exp_to_next_level(),
                               .hp_current = player.get_hp_current(),
                               .hp_max = player.get_hp_max(),
                               .mana_current = player.get_mana_current(),
                               .mana_max = player.get_mana_max()};
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

    CommandResult result;
    auto target_it = enemy_npcs.find(cmd.target_id);
    if (target_it == enemy_npcs.end())
        result = combat_controller.spell_attack_player(player_id, cmd.target_id, tick_count);
    else
        result = combat_controller.spell_attack_npc(player_id, cmd.target_id, tick_count);
    result.map_events = std::move(result.broadcast_events);
    uint8_t effect_type = item->spell_effect_id;
    if (effect_type == 0)
        effect_type = 1;
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
        if (text[0] == '/' && text.substr(0, 10) == "/resucitar")
            return handle_resurrect(player_id);
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
            for (const auto& [target_id, player]: players) {
                if (player.get_name() == target_nick) {
                    ChatMsgEvent chat_ev{ChatMsgType::PRIVATE, sender_name, msg, target_id,
                                         player_id};
                    std::map<uint16_t, std::vector<ServerEvent>> targeted;
                    targeted[target_id].push_back(chat_ev);
                    targeted[player_id].push_back(chat_ev);
                    return {.private_events = {},
                            .broadcast_events = {},
                            .targeted_events = std::move(targeted)};
                }
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

        if (cmd_name == "/meditar")
            return handle_meditate(player_id);
        if (cmd_name == "/resucitar")
            return handle_resurrect(player_id);
        if (cmd_name == "/curar")
            return handle_npc_heal(player_id);
        if (cmd_name == "/comprar")
            return handle_npc_buy(player_id, args);
        if (cmd_name == "/vender")
            return handle_npc_sell(player_id, args);
        if (cmd_name == "/equipar") {
            try {
                int idx = std::stoi(args);
                return handle_equip(player_id, EquipItemCmd{static_cast<uint8_t>(idx)});
            } catch (...) {
                return {};
            }
        }
        if (cmd_name == "/desequipar") {
            if (args == "weapon")
                return handle_unequip(player_id, UnequipItemCmd{EquipSlot::WEAPON});
            if (args == "armor")
                return handle_unequip(player_id, UnequipItemCmd{EquipSlot::ARMOR});
            if (args == "helmet")
                return handle_unequip(player_id, UnequipItemCmd{EquipSlot::HELMET});
            if (args == "shield")
                return handle_unequip(player_id, UnequipItemCmd{EquipSlot::SHIELD});
            return {};
        }

        ChatMsgEvent ev{ChatMsgType::SYSTEM, "", "Comando " + cmd_name + " no reconocido"};
        return {.private_events = {ev}, .broadcast_events = {}, .targeted_events = {}};
    }

    ChatMsgEvent broadcast_ev{ChatMsgType::SAY, sender_name, text};
    return {.private_events = {}, .broadcast_events = {broadcast_ev}, .targeted_events = {}};
}


CommandResult Game::handle_login(uint16_t player_id, const LoginCmd& cmd) {
    PlayerRecord rec;

    if (player_data_service.player_exists(cmd.username)) {
        rec = player_data_service.load_record(cmd.username);

        if (!rec.check_password(cmd.password)) {
            LoginErrorEvent err{LoginError::INVALID_CREDENTIALS, "Invalid password"};
            return {.private_events = {err}, .broadcast_events = {}, .targeted_events = {}};
        }

        if (is_username_logged_in(cmd.username)) {
            LoginErrorEvent err{LoginError::ALREADY_LOGGED_IN, "This user is already logged in"};
            return {.private_events = {err}, .broadcast_events = {}, .targeted_events = {}};
        }

        auto player_opt = player_data_service.load_player(player_id, cmd.username, rec);
        if (!player_opt.has_value()) {
            LoginErrorEvent err{LoginError::INVALID_CREDENTIALS, "Failed to load player"};
            return {.private_events = {err}, .broadcast_events = {}, .targeted_events = {}};
        }

        // Set clan membership
        std::string clan_name = clan_manager.get_clan_name(cmd.username);
        player_opt->set_clan_name(clan_name);

        auto it = players.emplace(player_id, std::move(*player_opt)).first;
        const Player& p = it->second;

        EntitySpawnEvent spawn = make_entity_spawn(p);
        std::vector<ServerEvent> private_events = {make_login_ok(p)};

        // Send inventory after login
        InventoryUpdateEvent inv_event{p.dump_inventory()};
        private_events.push_back(inv_event);

        // Send equipment state after login
        InventorySlot equipped_slots[EQUIP_SLOT_COUNT];
        p.dump_equipped(equipped_slots);
        EquipUpdateEvent equip_ev{player_id, equipped_slots[0], equipped_slots[1],
                                  equipped_slots[2], equipped_slots[3]};
        private_events.push_back(equip_ev);

        if (p.get_current_map() != "city") {
            private_events.push_back(MapTransitionEvent{
                    .map_name = p.get_current_map(),
                    .pos_x = p.pos_x(),
                    .pos_y = p.pos_y(),
            });
        }

        auto existing = make_existing_spawns(p.get_id(), p.get_current_map());
        private_events.insert(private_events.end(), existing.begin(), existing.end());

        // Notify clan members of login
        CommandResult login_result;
        login_result.private_events = std::move(private_events);
        login_result.map_events = {spawn};

        if (!clan_name.empty()) {
            ClanNotificationEvent notif{ClanNotifType::MEMBER_ONLINE, cmd.username, clan_name};
            auto clan_result = clan_handler.notify_clan_members(clan_name, notif, player_id);
            login_result.targeted_events = std::move(clan_result.targeted_events);
        }

        return login_result;
    }

    LoginErrorEvent err{LoginError::INVALID_CREDENTIALS, "Invalid username or password"};
    return {.private_events = {err}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult Game::handle_create_character(uint16_t player_id, const CreateCharacterCmd& cmd) {
    if (cmd.username.empty()) {
        CharacterErrorEvent err{CharacterError::INVALID_USERNAME,
                                "El nombre de usuario no puede estar vacio"};
        return {.private_events = {err}, .broadcast_events = {}, .targeted_events = {}};
    }

    PlayerRecord existing;
    if (player_data_service.player_exists(cmd.username)) {
        CharacterErrorEvent err{CharacterError::USERNAME_TAKEN,
                                "El usuario " + cmd.username + " ya existe"};
        return {.private_events = {err}, .broadcast_events = {}, .targeted_events = {}};
    }

    if (is_username_logged_in(cmd.username)) {
        CharacterErrorEvent err{CharacterError::USERNAME_TAKEN,
                                "El usuario " + cmd.username + " ya esta en juego"};
        return {.private_events = {err}, .broadcast_events = {}, .targeted_events = {}};
    }

    PlayerRecord rec;
    rec.set_username(cmd.username);
    rec.set_password(cmd.password);
    rec.race = static_cast<uint8_t>(cmd.race);
    rec.player_class = static_cast<uint8_t>(cmd.player_class);
    rec.level = 1;
    rec.experience = 0;
    rec.pos_x = static_cast<uint16_t>(balance.starting_pos_x);
    rec.pos_y = static_cast<uint16_t>(balance.starting_pos_y);
    rec.dir = static_cast<uint8_t>(Direction::SOUTH);
    rec.gold = static_cast<uint32_t>(balance.starting_gold);

    Player player(player_id, cmd.username, Position{rec.pos_x, rec.pos_y}, Direction::SOUTH,
                  cmd.race, cmd.player_class, balance, inventory_config.max_slots);
    player.set_current_map(balance.starting_map);
    rec.hp_current = player.get_hp_current();
    rec.hp_max = player.get_hp_max();
    rec.mana_current = player.get_mana_current();
    rec.mana_max = player.get_mana_max();

    const StartingItemsConfig& starting = balance.starting_items;
    const std::vector<ItemType>* items = nullptr;
    switch (cmd.player_class) {
        case PlayerClass::WARRIOR:
            items = &starting.warrior;
            break;
        case PlayerClass::MAGE:
            items = &starting.mage;
            break;
        case PlayerClass::CLERIC:
            items = &starting.cleric;
            break;
        case PlayerClass::PALADIN:
            items = &starting.paladin;
            break;
    }
    if (items) {
        for (ItemType type: *items) {
            const Item* def = item_catalog.find(type);
            if (def) {
                player.add_item(type, def->name);
            }
        }
    }

    player_data_service.save_new_player(cmd.username, rec);
    player_data_service.save_player(player);

    auto it = players.emplace(player_id, std::move(player)).first;
    const Player& p = it->second;

    CharacterCreatedEvent created{make_login_ok(p)};
    EntitySpawnEvent spawn = make_entity_spawn(p);
    std::vector<ServerEvent> private_events = {created};
    auto other_spawns = make_existing_spawns(p.get_id(), p.get_current_map());
    private_events.insert(private_events.end(), other_spawns.begin(), other_spawns.end());

    InventoryUpdateEvent inv_event{p.dump_inventory()};
    private_events.push_back(inv_event);

    InventorySlot equipped_slots[EQUIP_SLOT_COUNT];
    p.dump_equipped(equipped_slots);
    EquipUpdateEvent equip_ev{player_id, equipped_slots[0], equipped_slots[1], equipped_slots[2],
                              equipped_slots[3]};
    private_events.push_back(equip_ev);

    CommandResult r;
    r.private_events = std::move(private_events);
    r.map_events = {spawn};
    return r;
}

CommandResult Game::handle_cheat_infinite_hp(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};
    bool active = it->second.toggle_cheat_infinite_hp();
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                     active ? "[Cheat] HP infinito: ON" : "[Cheat] HP infinito: OFF"};
    return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult Game::handle_cheat_infinite_mana(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};
    Player& player = it->second;
    bool active = player.toggle_cheat_infinite_mana();
    if (active)
        player.restore_mana(player.get_mana_max());
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                     active ? "[Cheat] Mana infinito: ON" : "[Cheat] Mana infinito: OFF"};
    PlayerStatsEvent stats{.level = player.get_level(),
                           .experience = player.get_experience(),
                           .exp_to_next = player.exp_to_next_level(),
                           .hp_current = player.get_hp_current(),
                           .hp_max = player.get_hp_max(),
                           .mana_current = player.get_mana_current(),
                           .mana_max = player.get_mana_max()};
    return {.private_events = {msg, stats}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult Game::handle_cheat_die(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};
    it->second.kill();
    DamageReceivedEvent dmg{.target_id = player_id,
                            .attacker_id = player_id,
                            .damage = 0,
                            .hp_current = 0,
                            .hp_max = it->second.get_hp_max()};
    EntityDiedEvent died{.entity_id = player_id};
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "[Cheat] Moriste!"};
    CommandResult r;
    r.private_events = {msg};
    r.map_events = {dmg, died};
    return r;
}

CommandResult Game::handle_cheat_revive(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};
    Player& player = it->second;
    if (!player.is_dead()) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "[Cheat] No estás muerto"};
        return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
    }
    player.resurrect();
    PlayerRespawnedEvent respawn_ev{player_id, player.get_hp_current(), player.get_hp_max()};
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "[Cheat] Reviviste!"};
    CommandResult r;
    r.private_events = {msg};
    r.map_events = {respawn_ev};
    return r;
}

CommandResult Game::handle_cheat_level_up(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};
    Player& player = it->second;
    if (player.get_level() >= balance.max_level) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Ya estas en el nivel maximo"};
        return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
    }
    player.level_up();
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                     "[Cheat] Nivel subido a " + std::to_string(player.get_level())};
    PlayerStatsEvent stats{.level = player.get_level(),
                           .experience = player.get_experience(),
                           .exp_to_next = player.exp_to_next_level(),
                           .hp_current = player.get_hp_current(),
                           .hp_max = player.get_hp_max(),
                           .mana_current = player.get_mana_current(),
                           .mana_max = player.get_mana_max()};
    GoldUpdateEvent gold{player.get_gold()};
    return {.private_events = {msg, stats, gold}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult Game::handle_cheat_level_down(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};
    Player& player = it->second;
    if (player.get_level() <= balance.min_level) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Ya estas en el nivel minimo"};
        return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
    }
    player.level_down();
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                     "[Cheat] Nivel bajado a " + std::to_string(player.get_level())};
    PlayerStatsEvent stats{.level = player.get_level(),
                           .experience = player.get_experience(),
                           .exp_to_next = player.exp_to_next_level(),
                           .hp_current = player.get_hp_current(),
                           .hp_max = player.get_hp_max(),
                           .mana_current = player.get_mana_current(),
                           .mana_max = player.get_mana_max()};
    return {.private_events = {msg, stats}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult Game::handle_cheat_add_gold(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};
    Player& player = it->second;
    player.gain_gold(1000);
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                     "[Cheat] +1000 oro (total: " + std::to_string(player.get_gold()) + ")"};
    GoldUpdateEvent gold{player.get_gold()};
    return {.private_events = {msg, gold}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult Game::handle_cheat_reset_gold(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};
    Player& player = it->second;
    player.spend_gold(player.get_gold());
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "[Cheat] Oro reseteado a 0"};
    GoldUpdateEvent gold{player.get_gold()};
    return {.private_events = {msg, gold}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult Game::handle_cheat_reset_mana(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};
    Player& player = it->second;
    player.use_mana(player.get_mana_current());
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "[Cheat] Mana reseteado a 0"};
    PlayerStatsEvent stats{.level = player.get_level(),
                           .experience = player.get_experience(),
                           .exp_to_next = player.exp_to_next_level(),
                           .hp_current = player.get_hp_current(),
                           .hp_max = player.get_hp_max(),
                           .mana_current = player.get_mana_current(),
                           .mana_max = player.get_mana_max()};
    return {.private_events = {msg, stats}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult Game::handle_cheat_velocity(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};
    Player& player = it->second;
    bool active = player.toggle_cheat_fast_velocity();
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                     std::string("[Cheat] Velocidad: ") + (active ? "ON" : "OFF")};
    return {.private_events = {msg}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult Game::handle_cheat_fill_inventory(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};
    Player& player = it->second;
    for (const auto& item: item_catalog.all()) {
        if (item.type == ItemType::NONE || item.type == ItemType::GOLD_DROP)
            continue;
        player.add_item(item.type, item.name);
    }
    std::vector<InventorySlot> slots = player.dump_inventory();
    InventoryUpdateEvent inv{.slots = std::move(slots)};
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "[Cheat] Inventario lleno con todos los items!"};
    player_data_service.save_player(player);
    return {.private_events = {msg, inv}, .broadcast_events = {}, .targeted_events = {}};
}

CommandResult Game::handle_cheat_clear_inventory(uint16_t player_id) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};
    Player& player = it->second;
    player.clear_inventory();
    std::vector<InventorySlot> slots = player.dump_inventory();
    InventoryUpdateEvent inv{.slots = std::move(slots)};
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "[Cheat] Inventario vaciado!"};
    player_data_service.save_player(player);
    return {.private_events = {msg, inv}, .broadcast_events = {}, .targeted_events = {}};
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

LoginOkEvent Game::make_login_ok(const Player& p) const {
    return LoginOkEvent{
            .player_id = p.get_id(),
            .username = p.get_name(),
            .race = p.get_race(),
            .player_class = p.get_player_class(),
            .level = p.get_level(),
            .experience = p.get_experience(),
            .exp_to_next = p.exp_to_next_level(),
            .hp_current = p.get_hp_current(),
            .hp_max = p.get_hp_max(),
            .mana_current = p.get_mana_current(),
            .mana_max = p.get_mana_max(),
            .gold = p.get_gold(),
            .pos = p.get_pos(),
    };
}

EntitySpawnEvent Game::make_entity_spawn(const Player& p) const {
    const ItemType weapon_type = p.get_equipped(EquipSlot::WEAPON).item_type;
    const ItemType armor_type = p.get_equipped(EquipSlot::ARMOR).item_type;
    const ItemType helmet_type = p.get_equipped(EquipSlot::HELMET).item_type;
    const ItemType shield_type = p.get_equipped(EquipSlot::SHIELD).item_type;
    return EntitySpawnEvent{
            .entity_id = p.get_id(),
            .entity_type = EntityType::PLAYER,
            .entity_pos = p.get_pos(),
            .entity_dir = p.get_dir(),
            .entity_name = p.get_name(),
            .entity_race = p.get_race(),
            .entity_class = p.get_player_class(),
            .weapon_type = weapon_type,
            .armor_type = armor_type,
            .helmet_type = helmet_type,
            .shield_type = shield_type,
    };
}

EntitySpawnEvent Game::make_npc_spawn(const EnemyNpc& npc, uint16_t npc_id) const {
    return EntitySpawnEvent{
            .entity_id = npc_id,
            .entity_type = EntityType::NPC,
            .entity_pos = npc.get_pos(),
            .entity_dir = Direction::SOUTH,
            .entity_name = npc.get_name(),
            .entity_race = Race::HUMAN,
            .entity_class = PlayerClass::WARRIOR,
    };
}

std::vector<ServerEvent> Game::make_existing_spawns(uint16_t exclude_id) const {
    std::vector<ServerEvent> spawns;
    for (const auto& [id, player]: players) {
        if (id == exclude_id)
            continue;
        spawns.push_back(make_entity_spawn(player));
    }
    for (const auto& [id, npc]: enemy_npcs) {
        if (npc.is_dead())
            continue;
        spawns.push_back(make_npc_spawn(npc, id));
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
        spawns.push_back(make_entity_spawn(player));
    }
    for (const auto& [id, npc]: enemy_npcs) {
        if (npc.is_dead())
            continue;
        if (npc.get_current_map() != map_name)
            continue;
        spawns.push_back(make_npc_spawn(npc, id));
    }
    return spawns;
}

std::string Game::get_player_map_name(uint16_t player_id) const {
    auto it = players.find(player_id);
    if (it == players.end())
        return "city";
    return it->second.get_current_map();
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

bool Game::is_username_logged_in(const std::string& username) const {
    for (const auto& [id, player]: players) {
        if (player.get_name() == username)
            return true;
    }
    return false;
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
    const int range = tile_size * 3;

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


    int san_cx, san_cy;
    std::string target_map;
    uint32_t wait_ticks;

    if (map_it->second.prop_grid().find_nearest_center("sanadora", px, py, san_cx, san_cy)) {
        target_map = current_map;
        int dx = px - san_cx;
        int dy = py - san_cy;
        int dist_px = static_cast<int>(std::sqrt(static_cast<double>(dx * dx + dy * dy)));
        int dist_tiles = dist_px / tile_size;
        wait_ticks = static_cast<uint32_t>(dist_tiles * tick_rate_hz);
    } else {
        auto main_it = maps.find("city");
        if (main_it == maps.end() ||
            !main_it->second.prop_grid().find_nearest_center("sanadora", 0, 0, san_cx, san_cy))
            return {};
        target_map = "city";
        wait_ticks = static_cast<uint32_t>(10 * tick_rate_hz);
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

    ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Equipaste slot " + std::to_string(cmd.slot_index)};
    std::vector<ServerEvent> private_events{inv_ev, msg};
    if (consumed_potion)
        private_events.push_back(
                HealReceivedEvent{player_id, player.get_hp_current(), player.get_mana_current()});
    return {.private_events = private_events,
            .broadcast_events = {},
            .targeted_events = {},
            .map_events = {equip_ev}};
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
            .map_events = {equip_ev}};
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

    const int range = map_it->second.tile_size() * 3;
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

std::variant<Game::VendorContext, CommandResult> Game::resolve_vendor_ctx(
        uint16_t player_id, const std::string& item_name, const std::string& action) {
    auto it = players.find(player_id);
    if (it == players.end())
        return CommandResult{};

    Player& player = it->second;

    if (player.is_dead()) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Los fantasmas no pueden " + action};
        return CommandResult{.private_events = {msg}};
    }

    if (item_name.empty()) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Uso: /" + action + " <nombre del objeto>"};
        return CommandResult{.private_events = {msg}};
    }

    auto map_it = maps.find(player.get_current_map());
    if (map_it == maps.end())
        return CommandResult{};

    Map& map = map_it->second;
    return VendorContext{.player = &player,
                         .map = &map,
                         .px = static_cast<int>(player.pos_x()),
                         .py = static_cast<int>(player.pos_y()),
                         .range = map.tile_size() * balance.merchant.interaction_range_tiles};
}

CommandResult Game::handle_npc_buy(uint16_t player_id, const std::string& item_name) {
    auto ctx_or_err = resolve_vendor_ctx(player_id, item_name, "comprar");
    if (auto* err = std::get_if<CommandResult>(&ctx_or_err))
        return std::move(*err);
    VendorContext ctx = std::get<VendorContext>(ctx_or_err);
    Player& player = *ctx.player;

    const bool near_sacerdote =
            ctx.map->prop_grid().is_in_range_of("sacerdote", ctx.px, ctx.py, ctx.range);
    const bool near_comerciante =
            ctx.map->prop_grid().is_in_range_of("comerciante", ctx.px, ctx.py, ctx.range);

    if (!near_sacerdote && !near_comerciante) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "No hay un sacerdote ni un comerciante cerca"};
        return {.private_events = {msg}};
    }

    const Item* found = item_catalog.find_by_name(item_name);

    if (!found) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Objeto '" + item_name + "' no encontrado"};
        return {.private_events = {msg}};
    }

    const VendorsConfig& vendors = balance.vendors;
    const bool sacerdote_sells = near_sacerdote && vendor_sells(vendors, "sacerdote", found->type);
    const bool comerciante_sells =
            near_comerciante && vendor_sells(vendors, "comerciante", found->type);

    if (found->price == 0 || (!sacerdote_sells && !comerciante_sells)) {
        std::string vendor_label;
        if (near_sacerdote && near_comerciante) {
            vendor_label = "Ningún NPC cercano";
        } else if (near_comerciante) {
            vendor_label = "El comerciante";
        } else {
            vendor_label = "El sacerdote";
        }
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", vendor_label + " no vende ese objeto"};
        return {.private_events = {msg}};
    }

    if (player.get_gold() < found->price) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                         "Oro insuficiente. El " + found->name + " cuesta " +
                                 std::to_string(found->price) + " de oro"};
        return {.private_events = {msg}};
    }

    player.spend_gold(found->price);

    if (!player.add_item(found->type, found->name)) {
        player.gain_gold(found->price);
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Inventario lleno"};
        return {.private_events = {msg}};
    }

    InventoryUpdateEvent inv_ev{player.dump_inventory()};
    GoldUpdateEvent gold_ev{player.get_gold()};
    ChatMsgEvent msg{
            ChatMsgType::SYSTEM, "",
            "Compraste " + found->name + " por " + std::to_string(found->price) + " de oro"};
    return {.private_events = {msg, inv_ev, gold_ev}};
}

CommandResult Game::handle_npc_sell(uint16_t player_id, const std::string& item_name) {
    auto ctx_or_err = resolve_vendor_ctx(player_id, item_name, "vender");
    if (auto* err = std::get_if<CommandResult>(&ctx_or_err))
        return std::move(*err);
    VendorContext ctx = std::get<VendorContext>(ctx_or_err);
    Player& player = *ctx.player;

    if (!ctx.map->prop_grid().is_in_range_of("comerciante", ctx.px, ctx.py, ctx.range)) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "No hay un comerciante cerca"};
        return {.private_events = {msg}};
    }

    const Item* item_def = item_catalog.find_by_name(item_name);
    if (!item_def) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "Objeto '" + item_name + "' no encontrado"};
        return {.private_events = {msg}};
    }

    if (!vendor_sells(balance.vendors, "comerciante", item_def->type)) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "", "El comerciante no compra ese tipo de objeto"};
        return {.private_events = {msg}};
    }

    std::vector<InventorySlot> slots = player.dump_inventory();
    auto slot_it = std::find_if(slots.begin(), slots.end(), [&](const InventorySlot& slot) {
        return slot.item_type == item_def->type;
    });

    if (slot_it == slots.end()) {
        ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                         "No tenés '" + item_def->name + "' en el inventario"};
        return {.private_events = {msg}};
    }

    uint32_t sell_price =
            static_cast<uint32_t>(item_def->price * balance.merchant.sell_price_ratio);

    player.remove_inventory_item(static_cast<uint8_t>(slot_it->slot_index));
    player.gain_gold(sell_price);

    InventoryUpdateEvent inv_ev{player.dump_inventory()};
    GoldUpdateEvent gold_ev{player.get_gold()};
    ChatMsgEvent msg{ChatMsgType::SYSTEM, "",
                     "Vendiste " + item_name + " por " + std::to_string(sell_price) + " de oro"};
    return {.private_events = {msg, inv_ev, gold_ev}};
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

    const int final_dx = new_x - current_x;
    const int final_dy = new_y - current_y;
    if (final_dx == 0 && final_dy == 0)
        return {};

    player.apply_move(cmd.direction, final_dx, final_dy);

    CommandResult result;

    if (try_map_transition(player, result))
        return result;

    EntityMoveEvent move{
            .entity_id = player.get_id(),
            .entity_pos = player.get_pos(),
            .entity_dir = player.get_dir(),
    };
    result.map_events = {move};
    return result;
}

bool Game::try_map_transition(Player& player, CommandResult& result) {
    const std::string old_map_name = player.get_current_map();
    auto map_it = maps.find(old_map_name);
    if (map_it == maps.end())
        return false;

    const int foot_x = static_cast<int>(player.pos_x()) + sprite_width / 2;
    const int foot_y = static_cast<int>(player.pos_y()) + sprite_height;

    const PropDef* prop = map_it->second.prop_grid().find_transition_at(foot_x, foot_y);
    if (!prop)
        return false;

    do_transition(player, result, *prop, old_map_name);
    return true;
}

void Game::do_transition(Player& player, CommandResult& result, const PropDef& prop,
                         const std::string& old_map_name) {
    auto dest_it = maps.find(prop.transition_map);
    if (dest_it == maps.end())
        return;

    Position spawn = compute_spawn_position(dest_it->second, old_map_name, prop);

    despawn_player(result, player.get_id(), old_map_name);

    player.set_current_map(prop.transition_map);
    player.set_pos(spawn.x, spawn.y);

    player_data_service.save_player(player);

    notify_player_transition(result, player, prop.transition_map, spawn);
    notify_others_spawn(result, player, prop.transition_map);
}

Position Game::compute_spawn_position(const Map& dest_map, const std::string& old_map_name,
                                      const PropDef& source_prop) const {
    int cx, cy, hb_left, hb_bottom;
    if (dest_map.prop_grid().find_first_transition(old_map_name, cx, cy, hb_left, hb_bottom)) {
        int spawn_x = hb_left - dest_map.tile_size();
        int spawn_y = hb_bottom - dest_map.tile_size();
        return {static_cast<uint16_t>(spawn_x), static_cast<uint16_t>(spawn_y)};
    }
    int x = source_prop.transition_x;
    int y = source_prop.transition_y;
    if (x == 0 && y == 0) {
        x = dest_map.tile_size() * 2;
        y = dest_map.tile_size() * 2;
    }
    return {static_cast<uint16_t>(x), static_cast<uint16_t>(y)};
}

void Game::despawn_player(CommandResult& result, uint16_t player_id,
                          const std::string& old_map_name) const {
    EntityDespawnEvent despawn{.entity_id = player_id};
    for (uint16_t pid: get_player_ids_on_map(old_map_name)) {
        if (pid == player_id)
            continue;
        result.targeted_events[pid].push_back(despawn);
    }
}

void Game::notify_player_transition(CommandResult& result, const Player& player,
                                    const std::string& map_name, Position spawn) const {
    result.private_events.push_back(MapTransitionEvent{
            .map_name = map_name,
            .pos_x = spawn.x,
            .pos_y = spawn.y,
    });

    std::vector<ServerEvent> others = make_existing_spawns(player.get_id(), map_name);
    result.private_events.insert(result.private_events.end(), others.begin(), others.end());
}

void Game::notify_others_spawn(CommandResult& result, const Player& player,
                               const std::string& map_name) const {
    EntitySpawnEvent spawn = make_entity_spawn(player);
    for (uint16_t pid: get_player_ids_on_map(map_name)) {
        if (pid == player.get_id())
            continue;
        result.targeted_events[pid].push_back(spawn);
    }
}

CommandResult Game::handle_change_map(uint16_t player_id, const ChangeMapCmd& cmd) {
    auto it = players.find(player_id);
    if (it == players.end())
        return {};

    Player& player = it->second;
    const std::string& old_map_name = player.get_current_map();
    auto map_it = maps.find(old_map_name);
    if (map_it == maps.end())
        return {};

    const TilemapConfig& cfg = map_it->second.config();

    auto prop_it = cfg.props.find(cmd.prop_name);
    if (prop_it == cfg.props.end())
        return {};

    const PropDef& prop = prop_it->second;
    if (prop.transition_map.empty())
        return {};

    const int range = cfg.tile_size * 3;
    const int px = static_cast<int>(player.pos_x());
    const int py = static_cast<int>(player.pos_y());

    if (!map_it->second.prop_grid().is_in_range_of(cmd.prop_name, px, py, range))
        return {};

    CommandResult result;
    do_transition(player, result, prop, old_map_name);
    return result;
}
