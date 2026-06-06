#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>
#include <string>
#include <unordered_map>

#include "../../common/config.h"
#include "../../common/item_catalog.h"

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
    double critical_chance = 0.003;
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

struct ManaConfig {
    int intelligence_human = 20;
    int intelligence_elf = 25;
    int intelligence_dwarf = 15;
    int intelligence_gnome = 23;
    double race_mana_factor_human = 1.0;
    double race_mana_factor_elf = 1.3;
    double race_mana_factor_dwarf = 0.7;
    double race_mana_factor_gnome = 1.1;
    double class_mana_factor_warrior = 0.0;
    double class_mana_factor_paladin = 0.5;
    double class_mana_factor_cleric = 1.3;
    double class_mana_factor_mage = 2.0;
    double class_meditation_factor_warrior = 0.0;
    double class_meditation_factor_paladin = 0.5;
    double class_meditation_factor_cleric = 1.0;
    double class_meditation_factor_mage = 2.0;
};

struct StrengthConfig {
    double race_strength_factor_human = 1.0;
    double race_strength_factor_elf = 0.7;
    double race_strength_factor_dwarf = 1.3;
    double race_strength_factor_gnome = 0.9;
    double class_strength_factor_warrior = 1.5;
    double class_strength_factor_paladin = 1.2;
    double class_strength_factor_cleric = 0.8;
    double class_strength_factor_mage = 0.5;
};

struct AgilityConfig {
    double race_agility_factor_human = 0.9;
    double race_agility_factor_elf = 1.5;
    double race_agility_factor_dwarf = 0.6;
    double race_agility_factor_gnome = 1.0;
    double class_agility_factor_warrior = 0.7;
    double class_agility_factor_paladin = 0.9;
    double class_agility_factor_cleric = 1.1;
    double class_agility_factor_mage = 1.0;
};

struct HpConfig {
    int constitution_human = 20;
    int constitution_elf = 15;
    int constitution_dwarf = 25;
    int constitution_gnome = 20;
    double race_hp_factor_human = 1.0;
    double race_hp_factor_elf = 0.8;
    double race_hp_factor_dwarf = 1.2;
    double race_hp_factor_gnome = 1.0;
    double class_hp_factor_warrior = 2.0;
    double class_hp_factor_paladin = 1.5;
    double class_hp_factor_cleric = 1.2;
    double class_hp_factor_mage = 0.8;
};

struct InventoryConfig {
    int max_slots = 20;
};

struct BalanceConfig {
    int starting_gold = 0;
    int starting_pos_x = 100;
    int starting_pos_y = 100;
    int min_level = 1;
    int max_level = 100;
    int gold_per_level = 100;
    int level_exp_base = 1000;
    double level_exp_exponent = 1.8;
    int gold_cap_base = 100;
    double gold_cap_exponent = 1.1;
    RaceRecoveryConfig race_recovery;
    HpConfig hp;
    ManaConfig mana;
    StrengthConfig strength;
    AgilityConfig agility;
};

struct ServerConfig {
    uint16_t port = 1234;
    int tick_rate_hz = 20;
    int save_interval_seconds = 30;
    bool cheats_enabled = false;
    TilemapConfig tilemap;
    std::unordered_map<std::string, TilemapConfig> tilemap_configs;
    int move_step = 4;
    int sprite_width = 27;
    int sprite_height = 48;
    InventoryConfig inventory;
    BalanceConfig balance;
    AttackConfig attack;
    ClanConfig clan;
    ItemCatalog item_catalog;
};

ServerConfig load_server_config(const std::string& path);

#endif  // CONFIG_H
