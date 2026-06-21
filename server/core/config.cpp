#include "config.h"

#include <stdexcept>
#include <string>
#include <utility>

#include <toml++/toml.h>

#include "../../common/config.h"

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

    if (auto recovery = root["recovery_rates"].as_table()) {
        config.balance.race_recovery.human =
                toml_get_double(*recovery, "human", config.balance.race_recovery.human);
        config.balance.race_recovery.elf =
                toml_get_double(*recovery, "elf", config.balance.race_recovery.elf);
        config.balance.race_recovery.dwarf =
                toml_get_double(*recovery, "dwarf", config.balance.race_recovery.dwarf);
        config.balance.race_recovery.gnome =
                toml_get_double(*recovery, "gnome", config.balance.race_recovery.gnome);
    }

    if (auto c = root["constitution"].as_table()) {
        config.balance.hp.constitution_human =
                toml_get_int(*c, "human", config.balance.hp.constitution_human);
        config.balance.hp.constitution_elf =
                toml_get_int(*c, "elf", config.balance.hp.constitution_elf);
        config.balance.hp.constitution_dwarf =
                toml_get_int(*c, "dwarf", config.balance.hp.constitution_dwarf);
        config.balance.hp.constitution_gnome =
                toml_get_int(*c, "gnome", config.balance.hp.constitution_gnome);
    }

    if (auto r = root["race_hp_factor"].as_table()) {
        config.balance.hp.race_hp_factor_human =
                toml_get_double(*r, "human", config.balance.hp.race_hp_factor_human);
        config.balance.hp.race_hp_factor_elf =
                toml_get_double(*r, "elf", config.balance.hp.race_hp_factor_elf);
        config.balance.hp.race_hp_factor_dwarf =
                toml_get_double(*r, "dwarf", config.balance.hp.race_hp_factor_dwarf);
        config.balance.hp.race_hp_factor_gnome =
                toml_get_double(*r, "gnome", config.balance.hp.race_hp_factor_gnome);
    }

    if (auto cl = root["class_hp_factor"].as_table()) {
        config.balance.hp.class_hp_factor_warrior =
                toml_get_double(*cl, "warrior", config.balance.hp.class_hp_factor_warrior);
        config.balance.hp.class_hp_factor_paladin =
                toml_get_double(*cl, "paladin", config.balance.hp.class_hp_factor_paladin);
        config.balance.hp.class_hp_factor_cleric =
                toml_get_double(*cl, "cleric", config.balance.hp.class_hp_factor_cleric);
        config.balance.hp.class_hp_factor_mage =
                toml_get_double(*cl, "mage", config.balance.hp.class_hp_factor_mage);
    }

    if (auto i = root["intelligence"].as_table()) {
        config.balance.mana.intelligence_human =
                toml_get_int(*i, "human", config.balance.mana.intelligence_human);
        config.balance.mana.intelligence_elf =
                toml_get_int(*i, "elf", config.balance.mana.intelligence_elf);
        config.balance.mana.intelligence_dwarf =
                toml_get_int(*i, "dwarf", config.balance.mana.intelligence_dwarf);
        config.balance.mana.intelligence_gnome =
                toml_get_int(*i, "gnome", config.balance.mana.intelligence_gnome);
    }

    if (auto r = root["race_mana_factor"].as_table()) {
        config.balance.mana.race_mana_factor_human =
                toml_get_double(*r, "human", config.balance.mana.race_mana_factor_human);
        config.balance.mana.race_mana_factor_elf =
                toml_get_double(*r, "elf", config.balance.mana.race_mana_factor_elf);
        config.balance.mana.race_mana_factor_dwarf =
                toml_get_double(*r, "dwarf", config.balance.mana.race_mana_factor_dwarf);
        config.balance.mana.race_mana_factor_gnome =
                toml_get_double(*r, "gnome", config.balance.mana.race_mana_factor_gnome);
    }

    if (auto cl = root["class_mana_factor"].as_table()) {
        config.balance.mana.class_mana_factor_warrior =
                toml_get_double(*cl, "warrior", config.balance.mana.class_mana_factor_warrior);
        config.balance.mana.class_mana_factor_paladin =
                toml_get_double(*cl, "paladin", config.balance.mana.class_mana_factor_paladin);
        config.balance.mana.class_mana_factor_cleric =
                toml_get_double(*cl, "cleric", config.balance.mana.class_mana_factor_cleric);
        config.balance.mana.class_mana_factor_mage =
                toml_get_double(*cl, "mage", config.balance.mana.class_mana_factor_mage);
    }

    if (auto cl = root["class_meditation_factor"].as_table()) {
        config.balance.mana.class_meditation_factor_warrior = toml_get_double(
                *cl, "warrior", config.balance.mana.class_meditation_factor_warrior);
        config.balance.mana.class_meditation_factor_paladin = toml_get_double(
                *cl, "paladin", config.balance.mana.class_meditation_factor_paladin);
        config.balance.mana.class_meditation_factor_cleric =
                toml_get_double(*cl, "cleric", config.balance.mana.class_meditation_factor_cleric);
        config.balance.mana.class_meditation_factor_mage =
                toml_get_double(*cl, "mage", config.balance.mana.class_meditation_factor_mage);
    }

    if (auto r = root["race_strength_factor"].as_table()) {
        config.balance.strength.race_strength_factor_human =
                toml_get_double(*r, "human", config.balance.strength.race_strength_factor_human);
        config.balance.strength.race_strength_factor_elf =
                toml_get_double(*r, "elf", config.balance.strength.race_strength_factor_elf);
        config.balance.strength.race_strength_factor_dwarf =
                toml_get_double(*r, "dwarf", config.balance.strength.race_strength_factor_dwarf);
        config.balance.strength.race_strength_factor_gnome =
                toml_get_double(*r, "gnome", config.balance.strength.race_strength_factor_gnome);
    }

    if (auto cs = root["class_strength_factor"].as_table()) {
        config.balance.strength.class_strength_factor_warrior = toml_get_double(
                *cs, "warrior", config.balance.strength.class_strength_factor_warrior);
        config.balance.strength.class_strength_factor_paladin = toml_get_double(
                *cs, "paladin", config.balance.strength.class_strength_factor_paladin);
        config.balance.strength.class_strength_factor_cleric = toml_get_double(
                *cs, "cleric", config.balance.strength.class_strength_factor_cleric);
        config.balance.strength.class_strength_factor_mage =
                toml_get_double(*cs, "mage", config.balance.strength.class_strength_factor_mage);
    }

    if (auto r = root["race_agility_factor"].as_table()) {
        config.balance.agility.race_agility_factor_human =
                toml_get_double(*r, "human", config.balance.agility.race_agility_factor_human);
        config.balance.agility.race_agility_factor_elf =
                toml_get_double(*r, "elf", config.balance.agility.race_agility_factor_elf);
        config.balance.agility.race_agility_factor_dwarf =
                toml_get_double(*r, "dwarf", config.balance.agility.race_agility_factor_dwarf);
        config.balance.agility.race_agility_factor_gnome =
                toml_get_double(*r, "gnome", config.balance.agility.race_agility_factor_gnome);
    }

    if (auto ca = root["class_agility_factor"].as_table()) {
        config.balance.agility.class_agility_factor_warrior = toml_get_double(
                *ca, "warrior", config.balance.agility.class_agility_factor_warrior);
        config.balance.agility.class_agility_factor_paladin = toml_get_double(
                *ca, "paladin", config.balance.agility.class_agility_factor_paladin);
        config.balance.agility.class_agility_factor_cleric =
                toml_get_double(*ca, "cleric", config.balance.agility.class_agility_factor_cleric);
        config.balance.agility.class_agility_factor_mage =
                toml_get_double(*ca, "mage", config.balance.agility.class_agility_factor_mage);
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
        config.balance.starting_items.warrior = parse_item_list(*si, "warrior");
        config.balance.starting_items.mage = parse_item_list(*si, "mage");
        config.balance.starting_items.paladin = parse_item_list(*si, "paladin");
        config.balance.starting_items.cleric = parse_item_list(*si, "cleric");
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
