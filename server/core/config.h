#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>
#include <string>
#include <unordered_map>

#include "../../common/config.h"

struct AttackConfig {
    int base_damage = 10;
    int attack_range_px = 30;
    int damage_variance = 5;
    int cooldown_ticks = 10;
    int xp_per_level_kill = 100;
    int newbie_level = 12;
    int max_level_diff = 10;
    int clan_bonus_range_px = 200;
    double clan_bonus_per_member = 0.05;
    double clan_bonus_max = 0.25;
};

struct ClanConfig {
    int max_members = 16;
    int min_level_found = 6;
};

struct RaceRecoveryConfig {
    double human = 1.0;
    double elf = 1.5;
    double dwarf = 0.8;
    double gnome = 1.2;
};

struct BalanceConfig {
    int starting_hp = 100;
    int starting_mana = 50;
    int starting_gold = 0;
    int starting_pos_x = 100;
    int starting_pos_y = 100;
    int max_level = 100;
    int hp_per_level = 10;
    int mana_per_level = 5;
    int gold_per_level = 100;
    int level_exp_base = 1000;
    double level_exp_exponent = 1.8;
    int gold_cap_base = 100;
    double gold_cap_exponent = 1.1;
    RaceRecoveryConfig race_recovery;
};

struct ServerConfig {
    uint16_t port = 1234;
    int tick_rate_hz = 20;
    TilemapConfig tilemap;
    std::unordered_map<std::string, TilemapConfig> tilemap_configs;
    int move_step = 4;
    int sprite_width = 27;
    int sprite_height = 48;
    BalanceConfig balance;
    AttackConfig attack;
    ClanConfig clan;
};

ServerConfig load_server_config(const std::string& path);

#endif  // CONFIG_H
