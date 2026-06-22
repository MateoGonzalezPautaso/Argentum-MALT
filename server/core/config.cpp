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
        config.attack.npc_vision_range_px = static_cast<uint32_t>(
                toml_get_int(*attack, "npc_vision_range_px", config.attack.npc_vision_range_px));
        config.attack.npc_speed =
                static_cast<uint32_t>(toml_get_int(*attack, "npc_speed", config.attack.npc_speed));
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
        for (auto [name, race] : kRaceNames) {
            auto& rs = config.balance.race_stat(race);
            rs.recovery = toml_get_double(*tbl, name, rs.recovery);
        }
    }

    if (auto tbl = root["constitution"].as_table()) {
        for (auto [name, race] : kRaceNames) {
            auto& rs = config.balance.race_stat(race);
            rs.constitution = toml_get_int(*tbl, name, rs.constitution);
        }
    }

    if (auto tbl = root["intelligence"].as_table()) {
        for (auto [name, race] : kRaceNames) {
            auto& rs = config.balance.race_stat(race);
            rs.intelligence = toml_get_int(*tbl, name, rs.intelligence);
        }
    }

    if (auto tbl = root["race_hp_factor"].as_table()) {
        for (auto [name, race] : kRaceNames) {
            auto& rs = config.balance.race_stat(race);
            rs.hp_factor = toml_get_double(*tbl, name, rs.hp_factor);
        }
    }

    if (auto tbl = root["race_mana_factor"].as_table()) {
        for (auto [name, race] : kRaceNames) {
            auto& rs = config.balance.race_stat(race);
            rs.mana_factor = toml_get_double(*tbl, name, rs.mana_factor);
        }
    }

    if (auto tbl = root["race_strength_factor"].as_table()) {
        for (auto [name, race] : kRaceNames) {
            auto& rs = config.balance.race_stat(race);
            rs.strength_factor = toml_get_double(*tbl, name, rs.strength_factor);
        }
    }

    if (auto tbl = root["race_agility_factor"].as_table()) {
        for (auto [name, race] : kRaceNames) {
            auto& rs = config.balance.race_stat(race);
            rs.agility_factor = toml_get_double(*tbl, name, rs.agility_factor);
        }
    }

    if (auto tbl = root["class_hp_factor"].as_table()) {
        for (auto [name, cls] : kClassNames) {
            auto& cs = config.balance.class_stat(cls);
            cs.hp_factor = toml_get_double(*tbl, name, cs.hp_factor);
        }
    }

    if (auto tbl = root["class_mana_factor"].as_table()) {
        for (auto [name, cls] : kClassNames) {
            auto& cs = config.balance.class_stat(cls);
            cs.mana_factor = toml_get_double(*tbl, name, cs.mana_factor);
        }
    }

    if (auto tbl = root["class_strength_factor"].as_table()) {
        for (auto [name, cls] : kClassNames) {
            auto& cs = config.balance.class_stat(cls);
            cs.strength_factor = toml_get_double(*tbl, name, cs.strength_factor);
        }
    }

    if (auto tbl = root["class_agility_factor"].as_table()) {
        for (auto [name, cls] : kClassNames) {
            auto& cs = config.balance.class_stat(cls);
            cs.agility_factor = toml_get_double(*tbl, name, cs.agility_factor);
        }
    }

    if (auto tbl = root["class_meditation_factor"].as_table()) {
        for (auto [name, cls] : kClassNames) {
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

    return config;
}
