#include "config.h"

#include <stdexcept>
#include <string>

#include <toml++/toml.h>

#include "../../common/config.h"

ServerConfig load_server_config(const std::string& path) {
    toml::table root = toml::parse_file(path);
    ServerConfig config;

    if (auto server = root["server"].as_table()) {
        config.port =
                static_cast<uint16_t>(toml_get_int(*server, "port", static_cast<int>(config.port)));
        config.tick_rate_hz = toml_get_int(*server, "tick_rate_hz", config.tick_rate_hz);
    }

    config.tilemap_configs = load_all_map_configs("config/map_list.toml");

    if (config.tilemap_configs.empty()) {
        throw std::runtime_error("server config requires at least one map in map_list.toml");
    }

    config.tilemap = config.tilemap_configs.begin()->second;

    if (auto movement = root["movement"].as_table()) {
        config.move_step = toml_get_int(*movement, "move_step", config.move_step);
    }

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
        config.attack.clan_bonus_per_member =
                toml_get_double(*attack, "clan_bonus_per_member", config.attack.clan_bonus_per_member);
        config.attack.clan_bonus_max =
                toml_get_double(*attack, "clan_bonus_max", config.attack.clan_bonus_max);
    }

    if (auto clan = root["clan"].as_table()) {
        config.clan.max_members = toml_get_int(*clan, "max_members", config.clan.max_members);
        config.clan.min_level_found =
                toml_get_int(*clan, "min_level_found", config.clan.min_level_found);
    }

    if (auto balance = root["balance"].as_table()) {
        config.balance.starting_hp =
                toml_get_int(*balance, "starting_hp", config.balance.starting_hp);
        config.balance.starting_mana =
                toml_get_int(*balance, "starting_mana", config.balance.starting_mana);
        config.balance.starting_gold =
                toml_get_int(*balance, "starting_gold", config.balance.starting_gold);
        config.balance.starting_pos_x =
                toml_get_int(*balance, "starting_pos_x", config.balance.starting_pos_x);
        config.balance.starting_pos_y =
                toml_get_int(*balance, "starting_pos_y", config.balance.starting_pos_y);
        config.balance.max_level = toml_get_int(*balance, "max_level", config.balance.max_level);
        config.balance.hp_per_level =
                toml_get_int(*balance, "hp_per_level", config.balance.hp_per_level);
        config.balance.mana_per_level =
                toml_get_int(*balance, "mana_per_level", config.balance.mana_per_level);
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
    }

    return config;
}
