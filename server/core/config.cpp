#include "config.h"

#include <array>
#include <stdexcept>
#include <string>
#include <utility>

#include <toml++/toml.h>

#include "../../common/config.h"

namespace {
constexpr std::array<std::pair<const char*, Race>, 4> kRaceNames = {{
        {"human", Race::HUMAN},
        {"elf", Race::ELF},
        {"dwarf", Race::DWARF},
        {"gnome", Race::GNOME},
}};
constexpr std::array<std::pair<const char*, PlayerClass>, 4> kClassNames = {{
        {"mage", PlayerClass::MAGE},
        {"cleric", PlayerClass::CLERIC},
        {"paladin", PlayerClass::PALADIN},
        {"warrior", PlayerClass::WARRIOR},
}};
}  // namespace

std::vector<NpcTemplate> load_npc_templates(const std::string& path) {
    toml::table root = toml::parse_file(path);
    std::vector<NpcTemplate> templates;
    if (auto arr = root["npc"].as_array()) {
        for (const auto& node: *arr) {
            const auto* tbl = node.as_table();
            if (!tbl)
                continue;
            NpcTemplate t;
            t.name = toml_get_string(*tbl, "name", "Unknown");
            t.base_hp = static_cast<uint32_t>(toml_get_int(*tbl, "base_hp", 100));
            t.base_damage = static_cast<uint32_t>(toml_get_int(*tbl, "base_damage", 5));
            t.sprite_id = static_cast<uint16_t>(toml_get_int(*tbl, "sprite_id", 0));
            t.speed = static_cast<uint32_t>(toml_get_int(*tbl, "speed", 2));
            t.dungeon_only = toml_get_bool(*tbl, "dungeon_only", false);
            templates.push_back(std::move(t));
        }
    }
    return templates;
}

ServerConfig load_server_config(const std::string& path) {
    toml::table root = toml::parse_file(path);
    ServerConfig config;

    config.item_catalog.load_from_file("config/items.toml");
    config.npc_templates = load_npc_templates("config/npcs.toml");

    if (auto server = root["server"].as_table()) {
        config.tick_rate_hz = toml_get_int(*server, "tick_rate_hz", config.tick_rate_hz);
        config.save_interval_seconds =
                toml_get_int(*server, "save_interval_seconds", config.save_interval_seconds);
        config.cheats_enabled = toml_get_bool(*server, "cheats_enabled", config.cheats_enabled);
        config.npc_id_base =
                static_cast<uint16_t>(toml_get_int(*server, "npc_id_base", config.npc_id_base));
    }

    SharedConfig shared = load_shared_config("config/common.toml");
    config.port = shared.port;

    config.tilemap_configs = load_all_map_configs("config/map_list.toml");

    if (config.tilemap_configs.empty()) {
        throw std::runtime_error("server config requires at least one map in map_list.toml");
    }

    auto main_it = config.tilemap_configs.find("city");
    if (main_it != config.tilemap_configs.end()) {
        config.tilemap = main_it->second;
    } else {
        config.tilemap = config.tilemap_configs.begin()->second;
    }

    config.move_step = shared.move_step;

    if (auto sprite = root["sprite"].as_table()) {
        config.sprite_width = toml_get_int(*sprite, "width", config.sprite_width);
        config.sprite_height = toml_get_int(*sprite, "height", config.sprite_height);
    }

    if (auto attack = root["attack"].as_table()) {
        config.attack.base_damage = toml_get_int(*attack, "base_damage", config.attack.base_damage);
        config.attack.attack_range_px =
                toml_get_int(*attack, "attack_range_px", config.attack.attack_range_px);
        config.attack.damage_variance =
                toml_get_int(*attack, "damage_variance", config.attack.damage_variance);
        config.attack.cooldown_ticks =
                toml_get_int(*attack, "cooldown_ticks", config.attack.cooldown_ticks);
        config.attack.xp_per_level_kill =
                toml_get_int(*attack, "xp_per_level_kill", config.attack.xp_per_level_kill);
        config.attack.newbie_level =
                toml_get_int(*attack, "newbie_level", config.attack.newbie_level);
        config.attack.max_level_diff =
                toml_get_int(*attack, "max_level_diff", config.attack.max_level_diff);
        config.attack.clan_bonus_range_px =
                toml_get_int(*attack, "clan_bonus_range_px", config.attack.clan_bonus_range_px);
        config.attack.clan_bonus_per_member = toml_get_double(*attack, "clan_bonus_per_member",
                                                              config.attack.clan_bonus_per_member);
        config.attack.clan_bonus_max =
                toml_get_double(*attack, "clan_bonus_max", config.attack.clan_bonus_max);
        config.attack.critical_chance =
                toml_get_double(*attack, "critical_chance", config.attack.critical_chance);
        config.attack.critical_multiplier =
                toml_get_int(*attack, "critical_multiplier", config.attack.critical_multiplier);
        config.attack.dodge_threshold =
                toml_get_double(*attack, "dodge_threshold", config.attack.dodge_threshold);
        config.attack.spell_attack_range_px =
                toml_get_int(*attack, "spell_attack_range_px", config.attack.spell_attack_range_px);
    }

    if (auto inventory = root["inventory"].as_table()) {
        config.inventory.max_slots =
                toml_get_int(*inventory, "max_slots", config.inventory.max_slots);
        config.inventory.max_hp_potions =
                toml_get_int(*inventory, "max_hp_potions", config.inventory.max_hp_potions);
        config.inventory.max_mana_potions =
                toml_get_int(*inventory, "max_mana_potions", config.inventory.max_mana_potions);
        config.inventory.max_bank_slots =
                toml_get_int(*inventory, "max_bank_slots", config.inventory.max_bank_slots);
    }

    if (auto clan = root["clan"].as_table()) {
        config.clan.max_members = toml_get_int(*clan, "max_members", config.clan.max_members);
        config.clan.min_level_found =
                toml_get_int(*clan, "min_level_found", config.clan.min_level_found);
        config.clan.max_name_length =
                toml_get_int(*clan, "max_name_length", config.clan.max_name_length);
    }

    if (auto mob_spawn = root["mob_spawn"].as_table()) {
        config.mob_spawn.spawn_radius =
                toml_get_int(*mob_spawn, "spawn_radius", config.mob_spawn.spawn_radius);
        config.mob_spawn.world.min_level =
                toml_get_int(*mob_spawn, "min_level", config.mob_spawn.world.min_level);
        config.mob_spawn.world.max_level =
                toml_get_int(*mob_spawn, "max_level", config.mob_spawn.world.max_level);

        if (auto dungeon = (*mob_spawn)["dungeon"].as_table()) {
            config.mob_spawn.dungeon.min_level =
                    toml_get_int(*dungeon, "min_level", config.mob_spawn.dungeon.min_level);
            config.mob_spawn.dungeon.max_level =
                    toml_get_int(*dungeon, "max_level", config.mob_spawn.dungeon.max_level);
        }
    }

    if (auto balance = root["balance"].as_table()) {
        config.balance.starting_gold =
                toml_get_int(*balance, "starting_gold", config.balance.starting_gold);
        config.balance.starting_pos_x =
                toml_get_int(*balance, "starting_pos_x", config.balance.starting_pos_x);
        config.balance.starting_pos_y =
                toml_get_int(*balance, "starting_pos_y", config.balance.starting_pos_y);
        config.balance.starting_map =
                toml_get_string(*balance, "starting_map", config.balance.starting_map);
        config.balance.min_level = toml_get_int(*balance, "min_level", config.balance.min_level);
        config.balance.max_level = toml_get_int(*balance, "max_level", config.balance.max_level);
        config.balance.gold_per_level =
                toml_get_int(*balance, "gold_per_level", config.balance.gold_per_level);
        config.balance.level_exp_base =
                toml_get_int(*balance, "level_exp_base", config.balance.level_exp_base);
        config.balance.level_exp_exponent =
                toml_get_double(*balance, "level_exp_exponent", config.balance.level_exp_exponent);
        config.balance.gold_cap_base =
                toml_get_int(*balance, "gold_cap_base", config.balance.gold_cap_base);
        config.balance.gold_cap_exponent =
                toml_get_double(*balance, "gold_cap_exponent", config.balance.gold_cap_exponent);
        config.balance.exp_loss_on_death_ratio = toml_get_double(
                *balance, "exp_loss_on_death_ratio", config.balance.exp_loss_on_death_ratio);
        config.balance.gold_excess_ratio =
                toml_get_double(*balance, "gold_excess_ratio", config.balance.gold_excess_ratio);
        config.balance.experience_level_offset = toml_get_int(
                *balance, "experience_level_offset", config.balance.experience_level_offset);
        config.balance.npc_gold_reward_min_pct = toml_get_double(
                *balance, "npc_gold_reward_min_pct", config.balance.npc_gold_reward_min_pct);
        config.balance.npc_gold_reward_max_pct = toml_get_double(
                *balance, "npc_gold_reward_max_pct", config.balance.npc_gold_reward_max_pct);
        config.balance.extra_kill_exp_max_pct = toml_get_double(
                *balance, "extra_kill_exp_max_pct", config.balance.extra_kill_exp_max_pct);
        config.balance.npc_interaction_range_tiles =
                toml_get_int(*balance, "npc_interaction_range_tiles",
                             config.balance.npc_interaction_range_tiles);
        config.balance.default_resurrect_wait_seconds =
                toml_get_int(*balance, "default_resurrect_wait_seconds",
                             config.balance.default_resurrect_wait_seconds);
        config.balance.cheat_gold_amount =
                toml_get_int(*balance, "cheat_gold_amount", config.balance.cheat_gold_amount);
        config.balance.cheat_velocity_multiplier = toml_get_int(
                *balance, "cheat_velocity_multiplier", config.balance.cheat_velocity_multiplier);
        config.balance.npc_fallback_base_hp = static_cast<uint32_t>(toml_get_int(
                *balance, "npc_fallback_base_hp", config.balance.npc_fallback_base_hp));
        config.balance.npc_fallback_base_damage = static_cast<uint32_t>(toml_get_int(
                *balance, "npc_fallback_base_damage", config.balance.npc_fallback_base_damage));
        config.balance.default_spell_effect_id = static_cast<uint8_t>(toml_get_int(
                *balance, "default_spell_effect_id", config.balance.default_spell_effect_id));

        if (auto drop = (*balance)["npc_drop"].as_table()) {
            config.balance.npc_drop.gold_chance =
                    toml_get_double(*drop, "gold_chance", config.balance.npc_drop.gold_chance);
            config.balance.npc_drop.potion_chance =
                    toml_get_double(*drop, "potion_chance", config.balance.npc_drop.potion_chance);
            config.balance.npc_drop.item_chance =
                    toml_get_double(*drop, "item_chance", config.balance.npc_drop.item_chance);
        }

        // Defaults to the same odds as the open world unless overridden.
        config.balance.npc_drop_dungeon = config.balance.npc_drop;
        if (auto drop_dungeon = (*balance)["npc_drop_dungeon"].as_table()) {
            config.balance.npc_drop_dungeon.gold_chance = toml_get_double(
                    *drop_dungeon, "gold_chance", config.balance.npc_drop_dungeon.gold_chance);
            config.balance.npc_drop_dungeon.potion_chance = toml_get_double(
                    *drop_dungeon, "potion_chance", config.balance.npc_drop_dungeon.potion_chance);
            config.balance.npc_drop_dungeon.item_chance = toml_get_double(
                    *drop_dungeon, "item_chance", config.balance.npc_drop_dungeon.item_chance);
        }
    }

    if (auto tbl = root["recovery_rates"].as_table()) {
        for (auto [name, race]: kRaceNames) {
            auto& rs = config.balance.race_stat(race);
            rs.recovery = toml_get_double(*tbl, name, rs.recovery);
        }
    }

    if (auto tbl = root["constitution"].as_table()) {
        for (auto [name, race]: kRaceNames) {
            auto& rs = config.balance.race_stat(race);
            rs.constitution = toml_get_int(*tbl, name, rs.constitution);
        }
    }

    if (auto tbl = root["intelligence"].as_table()) {
        for (auto [name, race]: kRaceNames) {
            auto& rs = config.balance.race_stat(race);
            rs.intelligence = toml_get_int(*tbl, name, rs.intelligence);
        }
    }

    if (auto tbl = root["race_hp_factor"].as_table()) {
        for (auto [name, race]: kRaceNames) {
            auto& rs = config.balance.race_stat(race);
            rs.hp_factor = toml_get_double(*tbl, name, rs.hp_factor);
        }
    }

    if (auto tbl = root["race_mana_factor"].as_table()) {
        for (auto [name, race]: kRaceNames) {
            auto& rs = config.balance.race_stat(race);
            rs.mana_factor = toml_get_double(*tbl, name, rs.mana_factor);
        }
    }

    if (auto tbl = root["race_strength_factor"].as_table()) {
        for (auto [name, race]: kRaceNames) {
            auto& rs = config.balance.race_stat(race);
            rs.strength_factor = toml_get_double(*tbl, name, rs.strength_factor);
        }
    }

    if (auto tbl = root["race_agility_factor"].as_table()) {
        for (auto [name, race]: kRaceNames) {
            auto& rs = config.balance.race_stat(race);
            rs.agility_factor = toml_get_double(*tbl, name, rs.agility_factor);
        }
    }

    if (auto tbl = root["class_hp_factor"].as_table()) {
        for (auto [name, cls]: kClassNames) {
            auto& cs = config.balance.class_stat(cls);
            cs.hp_factor = toml_get_double(*tbl, name, cs.hp_factor);
        }
    }

    if (auto tbl = root["class_mana_factor"].as_table()) {
        for (auto [name, cls]: kClassNames) {
            auto& cs = config.balance.class_stat(cls);
            cs.mana_factor = toml_get_double(*tbl, name, cs.mana_factor);
        }
    }

    if (auto tbl = root["class_strength_factor"].as_table()) {
        for (auto [name, cls]: kClassNames) {
            auto& cs = config.balance.class_stat(cls);
            cs.strength_factor = toml_get_double(*tbl, name, cs.strength_factor);
        }
    }

    if (auto tbl = root["class_agility_factor"].as_table()) {
        for (auto [name, cls]: kClassNames) {
            auto& cs = config.balance.class_stat(cls);
            cs.agility_factor = toml_get_double(*tbl, name, cs.agility_factor);
        }
    }

    if (auto tbl = root["class_meditation_factor"].as_table()) {
        for (auto [name, cls]: kClassNames) {
            auto& cs = config.balance.class_stat(cls);
            cs.meditation_factor = toml_get_double(*tbl, name, cs.meditation_factor);
        }
    }

    if (auto si = root["starting_items"].as_table()) {
        auto parse_item_list = [](const toml::table& tbl, const std::string& key) {
            std::vector<ItemType> result;
            if (auto arr = tbl[key].as_array()) {
                for (const auto& elem: *arr) {
                    if (auto val = elem.value<std::string>()) {
                        ItemType type = parse_item_type(*val);
                        if (type != ItemType::NONE) {
                            result.push_back(type);
                        }
                    }
                }
            }
            return result;
        };
        config.balance.starting_items.by_class[PlayerClass::WARRIOR] =
                parse_item_list(*si, "warrior");
        config.balance.starting_items.by_class[PlayerClass::MAGE] = parse_item_list(*si, "mage");
        config.balance.starting_items.by_class[PlayerClass::PALADIN] =
                parse_item_list(*si, "paladin");
        config.balance.starting_items.by_class[PlayerClass::CLERIC] =
                parse_item_list(*si, "cleric");
    }

    if (auto merchant = root["merchant"].as_table()) {
        config.balance.merchant.interaction_range_tiles =
                toml_get_int(*merchant, "interaction_range_tiles",
                             config.balance.merchant.interaction_range_tiles);
    }
    config.balance.merchant.sell_price_ratio = shared.merchant_sell_price_ratio;

    if (auto vendors = root["vendors"].as_table()) {
        for (const auto& [vendor_name, value]: *vendors) {
            auto arr = value.as_array();
            if (!arr)
                continue;
            std::unordered_set<ItemType> sold;
            for (const auto& elem: *arr) {
                if (auto val = elem.value<std::string>()) {
                    ItemType type = parse_item_type(*val);
                    if (type != ItemType::NONE)
                        sold.insert(type);
                }
            }
            config.balance.vendors.by_vendor.emplace(std::string(vendor_name.str()),
                                                     std::move(sold));
        }
    }

    if (auto help = root["help"].as_table()) {
        if (auto arr = (*help)["lines"].as_array()) {
            for (const auto& elem: *arr) {
                if (auto val = elem.value<std::string>())
                    config.help_lines.push_back(*val);
            }
        }
    }

    if (auto npc = root["npc"].as_table()) {
        config.npc.vision_range_px = static_cast<uint32_t>(
                toml_get_int(*npc, "vision_range_px", config.npc.vision_range_px));
        config.npc.idle_move_min_ticks = static_cast<uint32_t>(
                toml_get_int(*npc, "idle_move_min_ticks", config.npc.idle_move_min_ticks));
        config.npc.idle_move_max_ticks = static_cast<uint32_t>(
                toml_get_int(*npc, "idle_move_max_ticks", config.npc.idle_move_max_ticks));
    }

    if (auto msgs = root["messages"].as_table()) {
        config.messages.attack_newbie_attacker = toml_get_string(
                *msgs, "attack_newbie_attacker", config.messages.attack_newbie_attacker);
        config.messages.attack_newbie_target = toml_get_string(
                *msgs, "attack_newbie_target", config.messages.attack_newbie_target);
        config.messages.attack_level_diff =
                toml_get_string(*msgs, "attack_level_diff", config.messages.attack_level_diff);
        config.messages.attack_same_clan =
                toml_get_string(*msgs, "attack_same_clan", config.messages.attack_same_clan);
        config.messages.attack_safe_zone =
                toml_get_string(*msgs, "attack_safe_zone", config.messages.attack_safe_zone);
        config.messages.attack_self =
                toml_get_string(*msgs, "attack_self", config.messages.attack_self);
        config.messages.warrior_no_magic =
                toml_get_string(*msgs, "warrior_no_magic", config.messages.warrior_no_magic);
        config.messages.no_weapon_equipped =
                toml_get_string(*msgs, "no_weapon_equipped", config.messages.no_weapon_equipped);
        config.messages.weapon_not_magic =
                toml_get_string(*msgs, "weapon_not_magic", config.messages.weapon_not_magic);
        config.messages.insufficient_mana =
                toml_get_string(*msgs, "insufficient_mana", config.messages.insufficient_mana);
        config.messages.spell_safe_zone =
                toml_get_string(*msgs, "spell_safe_zone", config.messages.spell_safe_zone);
        config.messages.ghost_cant_interact =
                toml_get_string(*msgs, "ghost_cant_interact", config.messages.ghost_cant_interact);
        config.messages.ghost_cant_pickup =
                toml_get_string(*msgs, "ghost_cant_pickup", config.messages.ghost_cant_pickup);
        config.messages.ghost_cant_drop =
                toml_get_string(*msgs, "ghost_cant_drop", config.messages.ghost_cant_drop);
        config.messages.ghost_cant_deposit =
                toml_get_string(*msgs, "ghost_cant_deposit", config.messages.ghost_cant_deposit);
        config.messages.ghost_cant_withdraw =
                toml_get_string(*msgs, "ghost_cant_withdraw", config.messages.ghost_cant_withdraw);
        config.messages.ghost_cant_be_healed = toml_get_string(
                *msgs, "ghost_cant_be_healed", config.messages.ghost_cant_be_healed);
        config.messages.ghost_cant_list =
                toml_get_string(*msgs, "ghost_cant_list", config.messages.ghost_cant_list);
        config.messages.inventory_full =
                toml_get_string(*msgs, "inventory_full", config.messages.inventory_full);
        config.messages.no_banker_nearby =
                toml_get_string(*msgs, "no_banker_nearby", config.messages.no_banker_nearby);
        config.messages.insufficient_gold =
                toml_get_string(*msgs, "insufficient_gold", config.messages.insufficient_gold);
        config.messages.bank_full = toml_get_string(*msgs, "bank_full", config.messages.bank_full);
        config.messages.insufficient_bank_gold = toml_get_string(
                *msgs, "insufficient_bank_gold", config.messages.insufficient_bank_gold);
        config.messages.nothing_to_pickup =
                toml_get_string(*msgs, "nothing_to_pickup", config.messages.nothing_to_pickup);
        config.messages.no_merchant_priest_banker = toml_get_string(
                *msgs, "no_merchant_priest_banker", config.messages.no_merchant_priest_banker);
        config.messages.no_merchant_priest =
                toml_get_string(*msgs, "no_merchant_priest", config.messages.no_merchant_priest);
        config.messages.no_merchant_nearby =
                toml_get_string(*msgs, "no_merchant_nearby", config.messages.no_merchant_nearby);
        config.messages.merchant_doesnt_buy =
                toml_get_string(*msgs, "merchant_doesnt_buy", config.messages.merchant_doesnt_buy);
        config.messages.no_priest_nearby =
                toml_get_string(*msgs, "no_priest_nearby", config.messages.no_priest_nearby);
        config.messages.warrior_cant_meditate = toml_get_string(
                *msgs, "warrior_cant_meditate", config.messages.warrior_cant_meditate);
        config.messages.not_dead = toml_get_string(*msgs, "not_dead", config.messages.not_dead);
        config.messages.already_resurrecting = toml_get_string(
                *msgs, "already_resurrecting", config.messages.already_resurrecting);
        config.messages.priest_resurrect =
                toml_get_string(*msgs, "priest_resurrect", config.messages.priest_resurrect);
        config.messages.priest_heal =
                toml_get_string(*msgs, "priest_heal", config.messages.priest_heal);
        config.messages.self_heal_success =
                toml_get_string(*msgs, "self_heal_success", config.messages.self_heal_success);
        config.messages.not_in_clan =
                toml_get_string(*msgs, "not_in_clan", config.messages.not_in_clan);
        config.messages.only_founder_review =
                toml_get_string(*msgs, "only_founder_review", config.messages.only_founder_review);
        config.messages.no_pending_requests =
                toml_get_string(*msgs, "no_pending_requests", config.messages.no_pending_requests);
        config.messages.usage_found_clan =
                toml_get_string(*msgs, "usage_found_clan", config.messages.usage_found_clan);
        config.messages.usage_join_clan =
                toml_get_string(*msgs, "usage_join_clan", config.messages.usage_join_clan);
        config.messages.usage_clan_accept =
                toml_get_string(*msgs, "usage_clan_accept", config.messages.usage_clan_accept);
        config.messages.usage_clan_reject =
                toml_get_string(*msgs, "usage_clan_reject", config.messages.usage_clan_reject);
        config.messages.usage_clan_ban =
                toml_get_string(*msgs, "usage_clan_ban", config.messages.usage_clan_ban);
        config.messages.usage_clan_kick =
                toml_get_string(*msgs, "usage_clan_kick", config.messages.usage_clan_kick);
        config.messages.usage_clan_unban =
                toml_get_string(*msgs, "usage_clan_unban", config.messages.usage_clan_unban);
        config.messages.usage_clan_chat =
                toml_get_string(*msgs, "usage_clan_chat", config.messages.usage_clan_chat);
        config.messages.cheat_inf_hp_on =
                toml_get_string(*msgs, "cheat_inf_hp_on", config.messages.cheat_inf_hp_on);
        config.messages.cheat_inf_hp_off =
                toml_get_string(*msgs, "cheat_inf_hp_off", config.messages.cheat_inf_hp_off);
        config.messages.cheat_inf_mana_on =
                toml_get_string(*msgs, "cheat_inf_mana_on", config.messages.cheat_inf_mana_on);
        config.messages.cheat_inf_mana_off =
                toml_get_string(*msgs, "cheat_inf_mana_off", config.messages.cheat_inf_mana_off);
        config.messages.cheat_died =
                toml_get_string(*msgs, "cheat_died", config.messages.cheat_died);
        config.messages.cheat_not_dead =
                toml_get_string(*msgs, "cheat_not_dead", config.messages.cheat_not_dead);
        config.messages.cheat_revived =
                toml_get_string(*msgs, "cheat_revived", config.messages.cheat_revived);
        config.messages.cheat_max_level =
                toml_get_string(*msgs, "cheat_max_level", config.messages.cheat_max_level);
        config.messages.cheat_min_level =
                toml_get_string(*msgs, "cheat_min_level", config.messages.cheat_min_level);
        config.messages.cheat_gold_reset =
                toml_get_string(*msgs, "cheat_gold_reset", config.messages.cheat_gold_reset);
        config.messages.cheat_mana_reset =
                toml_get_string(*msgs, "cheat_mana_reset", config.messages.cheat_mana_reset);
        config.messages.cheat_velocity_on =
                toml_get_string(*msgs, "cheat_velocity_on", config.messages.cheat_velocity_on);
        config.messages.cheat_velocity_off =
                toml_get_string(*msgs, "cheat_velocity_off", config.messages.cheat_velocity_off);
        config.messages.cheat_inventory_filled = toml_get_string(
                *msgs, "cheat_inventory_filled", config.messages.cheat_inventory_filled);
        config.messages.cheat_inventory_cleared = toml_get_string(
                *msgs, "cheat_inventory_cleared", config.messages.cheat_inventory_cleared);
        config.messages.item_not_found =
                toml_get_string(*msgs, "item_not_found", config.messages.item_not_found);
        config.messages.item_not_in_bank =
                toml_get_string(*msgs, "item_not_in_bank", config.messages.item_not_in_bank);
        config.messages.item_not_in_inventory = toml_get_string(
                *msgs, "item_not_in_inventory", config.messages.item_not_in_inventory);
        config.messages.item_not_on_ground =
                toml_get_string(*msgs, "item_not_on_ground", config.messages.item_not_on_ground);
        config.messages.vendor_doesnt_sell =
                toml_get_string(*msgs, "vendor_doesnt_sell", config.messages.vendor_doesnt_sell);
        config.messages.insufficient_gold_item = toml_get_string(
                *msgs, "insufficient_gold_item", config.messages.insufficient_gold_item);
        config.messages.ghost_cant_action =
                toml_get_string(*msgs, "ghost_cant_action", config.messages.ghost_cant_action);
        config.messages.usage_action_item =
                toml_get_string(*msgs, "usage_action_item", config.messages.usage_action_item);
        config.messages.npc_drop_inventory_full = toml_get_string(
                *msgs, "npc_drop_inventory_full", config.messages.npc_drop_inventory_full);
        config.messages.gold_stolen_from =
                toml_get_string(*msgs, "gold_stolen_from", config.messages.gold_stolen_from);
        config.messages.gold_stolen_by =
                toml_get_string(*msgs, "gold_stolen_by", config.messages.gold_stolen_by);
        config.messages.gold_lost_on_death =
                toml_get_string(*msgs, "gold_lost_on_death", config.messages.gold_lost_on_death);
        config.messages.npc_attacked_player =
                toml_get_string(*msgs, "npc_attacked_player", config.messages.npc_attacked_player);
        config.messages.player_killed =
                toml_get_string(*msgs, "player_killed", config.messages.player_killed);
        config.messages.player_not_found =
                toml_get_string(*msgs, "player_not_found", config.messages.player_not_found);
        config.messages.command_not_recognized = toml_get_string(
                *msgs, "command_not_recognized", config.messages.command_not_recognized);
        config.messages.resurrect_countdown =
                toml_get_string(*msgs, "resurrect_countdown", config.messages.resurrect_countdown);
        config.messages.clan_level_required =
                toml_get_string(*msgs, "clan_level_required", config.messages.clan_level_required);
    }

    return config;
}
