#ifndef CONFIG_H
#define CONFIG_H

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
    int critical_multiplier = 2;
    double dodge_threshold = 0.001;
    uint32_t spell_attack_range_px = 200;
    uint32_t npc_vision_range_px = 0;
    uint32_t npc_speed = 4;
};

struct ClanConfig {
    int max_members = 16;
    int min_level_found = 6;
    int max_name_length = 30;
};

struct RaceStats {
    int constitution = 20;
    double hp_factor = 1.0;
    int intelligence = 20;
    double mana_factor = 1.0;
    double strength_factor = 1.0;
    double agility_factor = 1.0;
    double recovery = 1.0;
};

struct ClassStats {
    double hp_factor = 1.0;
    double mana_factor = 1.0;
    double strength_factor = 1.0;
    double agility_factor = 1.0;
    double meditation_factor = 1.0;
};


struct StartingItemsConfig {
    std::unordered_map<PlayerClass, std::vector<ItemType>> by_class;
};

struct VendorsConfig {
    // npc_name, items
    std::unordered_map<std::string, std::unordered_set<ItemType>> by_vendor;
};

struct MerchantConfig {
    int interaction_range_tiles = 3;
    double sell_price_ratio = 0.5;
};

struct NpcDropConfig {
    double gold_chance = 8.0;
    double potion_chance = 1.0;
    double item_chance = 1.0;
};

struct NpcTemplate {
    std::string name;
    uint32_t base_hp = 0;
    uint32_t base_damage = 0;
    uint16_t sprite_id = 0;
    uint32_t speed = 2;
    bool dungeon_only = false;
};

struct MobLevelRange {
    int min_level = 1;
    int max_level = 5;
};

struct MobSpawnConfig {
    int spawn_radius = 10;
    MobLevelRange world;
    MobLevelRange dungeon{6, 15};
};

struct InventoryConfig {
    int max_slots = 20;
    int max_hp_potions = 10;
    int max_mana_potions = 10;
    int max_bank_slots = 40;
};

struct BalanceConfig {
    int starting_gold = 0;
    int starting_pos_x = 100;
    int starting_pos_y = 100;
    std::string starting_map = "city";
    int npc_interaction_range_tiles = 3;
    int default_resurrect_wait_seconds = 10;
    int cheat_gold_amount = 1000;
    int cheat_velocity_multiplier = 2;
    uint32_t npc_fallback_base_hp = 100;
    uint32_t npc_fallback_base_damage = 5;
    uint8_t default_spell_effect_id = 1;
    int min_level = 1;
    int max_level = 100;
    int gold_per_level = 100;
    int level_exp_base = 1000;
    double level_exp_exponent = 1.8;
    int gold_cap_base = 100;
    double gold_cap_exponent = 1.1;
    double exp_loss_on_death_ratio = 0.9;
    double gold_excess_ratio = 0.5;
    int experience_level_offset = 10;
    double npc_gold_reward_min_pct = 0.01;
    double npc_gold_reward_max_pct = 0.2;
    double extra_kill_exp_max_pct = 0.1;
    StartingItemsConfig starting_items;
    VendorsConfig vendors;
    MerchantConfig merchant;
    NpcDropConfig npc_drop;
    NpcDropConfig npc_drop_dungeon;

    const RaceStats&  race_stat(Race r)        const noexcept { return race_stats[static_cast<size_t>(r) - 1]; }
    RaceStats&        race_stat(Race r)               noexcept { return race_stats[static_cast<size_t>(r) - 1]; }
    const ClassStats& class_stat(PlayerClass c) const noexcept { return class_stats[static_cast<size_t>(c) - 1]; }
    ClassStats&       class_stat(PlayerClass c)       noexcept { return class_stats[static_cast<size_t>(c) - 1]; }

private:
    // Order: HUMAN, ELF, DWARF, GNOME
    std::array<RaceStats, 4> race_stats = {{
        {20, 1.0, 20, 1.0, 1.0, 0.9, 1.0},  // HUMAN
        {15, 0.8, 25, 1.3, 0.7, 1.5, 1.5},  // ELF
        {25, 1.2, 15, 0.7, 1.3, 0.6, 0.8},  // DWARF
        {20, 1.0, 23, 1.1, 0.9, 1.0, 1.2},  // GNOME
    }};
    // Order: MAGE, CLERIC, PALADIN, WARRIOR
    std::array<ClassStats, 4> class_stats = {{
        {0.8, 2.0, 0.5, 1.0, 2.0},  // MAGE
        {1.2, 1.3, 0.8, 1.1, 1.0},  // CLERIC
        {1.5, 0.5, 1.2, 0.9, 0.5},  // PALADIN
        {2.0, 0.0, 1.5, 0.7, 0.0},  // WARRIOR
    }};
};

struct MessagesConfig {
    // Combat
    std::string attack_newbie_attacker = "No puedes atacar siendo newbie";
    std::string attack_newbie_target = "No puedes atacar a un jugador newbie";
    std::string attack_level_diff = "No puedes atacar a un jugador con diferencia de niveles mayor a 10";
    std::string attack_same_clan = "No puedes atacar a un miembro de tu clan";
    std::string attack_safe_zone = "No puedes atacar en una zona segura";
    std::string attack_self = "No puedes atacarte a ti mismo";
    // Spell
    std::string warrior_no_magic = "Los guerreros no pueden usar magia";
    std::string no_weapon_equipped = "No tienes un arma equipada";
    std::string weapon_not_magic = "El arma equipada no es magica";
    std::string insufficient_mana = "Mana insuficiente";
    std::string spell_safe_zone = "No puedes lanzar hechizos en una zona segura";
    // Ghost restrictions
    std::string ghost_cant_interact = "Los fantasmas no pueden interactuar";
    std::string ghost_cant_pickup = "Los fantasmas no pueden recoger objetos";
    std::string ghost_cant_drop = "Los fantasmas no pueden tirar objetos";
    std::string ghost_cant_deposit = "Los fantasmas no pueden depositar";
    std::string ghost_cant_withdraw = "Los fantasmas no pueden retirar";
    std::string ghost_cant_be_healed = "Los fantasmas no pueden ser curados";
    std::string ghost_cant_list = "Los fantasmas no pueden listar";
    // Inventory / capacity
    std::string inventory_full = "Inventario lleno";
    // Bank
    std::string no_banker_nearby = "No hay un banquero cerca";
    std::string insufficient_gold = "Oro insuficiente";
    std::string bank_full = "El banco está lleno";
    std::string insufficient_bank_gold = "No tenés suficiente oro en el banco";
    // Ground items
    std::string nothing_to_pickup = "No hay nada para recoger aquí";
    // Merchant / NPC
    std::string no_merchant_priest_banker = "No hay un comerciante, sacerdote ni banquero cerca";
    std::string no_merchant_priest = "No hay un sacerdote ni un comerciante cerca";
    std::string no_merchant_nearby = "No hay un comerciante cerca";
    std::string merchant_doesnt_buy = "El comerciante no compra ese tipo de objeto";
    std::string no_priest_nearby = "No hay un sacerdote cerca";
    // Resurrect / heal
    std::string warrior_cant_meditate = "Los guerreros no pueden meditar";
    std::string not_dead = "No estás muerto";
    std::string already_resurrecting = "Ya estás resucitando, espera";
    std::string priest_resurrect = "Sacerdote: ¡Que la luz te devuelva a la vida!";
    std::string priest_heal = "Sacerdote: ¡Que la luz te sane!";
    std::string self_heal_success = "Te has curado!";
    // Clan
    std::string not_in_clan = "No perteneces a ningun clan";
    std::string only_founder_review = "Solo el fundador puede revisar el clan";
    std::string no_pending_requests = "No hay pedidos pendientes";
    std::string usage_found_clan = "Uso: /fundar-clan <nombre>";
    std::string usage_join_clan = "Uso: /unirse <nombre del clan>";
    std::string usage_clan_accept = "Uso: /clan-aceptar <nick>";
    std::string usage_clan_reject = "Uso: /clan-rechazar <nick>";
    std::string usage_clan_ban = "Uso: /clan-ban <nick>";
    std::string usage_clan_kick = "Uso: /clan-kick <nick>";
    std::string usage_clan_unban = "Uso: /clan-unban <nick>";
    std::string usage_clan_chat = "Uso: /c <mensaje>";
    // Cheats
    std::string cheat_inf_hp_on = "[Cheat] HP infinito: ON";
    std::string cheat_inf_hp_off = "[Cheat] HP infinito: OFF";
    std::string cheat_inf_mana_on = "[Cheat] Mana infinito: ON";
    std::string cheat_inf_mana_off = "[Cheat] Mana infinito: OFF";
    std::string cheat_died = "[Cheat] Moriste!";
    std::string cheat_not_dead = "[Cheat] No estás muerto";
    std::string cheat_revived = "[Cheat] Reviviste!";
    std::string cheat_max_level = "Ya estas en el nivel maximo";
    std::string cheat_min_level = "Ya estas en el nivel minimo";
    std::string cheat_gold_reset = "[Cheat] Oro reseteado a 0";
    std::string cheat_mana_reset = "[Cheat] Mana reseteado a 0";
    std::string cheat_velocity_on = "[Cheat] Velocidad: ON";
    std::string cheat_velocity_off = "[Cheat] Velocidad: OFF";
    std::string cheat_inventory_filled = "[Cheat] Inventario lleno con todos los items!";
    std::string cheat_inventory_cleared = "[Cheat] Inventario vaciado!";
};

struct ServerConfig {
    uint16_t port = 1234;
    int tick_rate_hz = 20;
    int save_interval_seconds = 30;
    bool cheats_enabled = false;
    uint16_t npc_id_base = 2000;
    TilemapConfig tilemap;
    std::unordered_map<std::string, TilemapConfig> tilemap_configs;
    int move_step = 4;
    int sprite_width = 27;
    int sprite_height = 48;
    InventoryConfig inventory;
    BalanceConfig balance;
    AttackConfig attack;
    ClanConfig clan;
    MobSpawnConfig mob_spawn;
    ItemCatalog item_catalog;
    std::vector<NpcTemplate> npc_templates;
    std::vector<std::string> help_lines;
    MessagesConfig messages;
};

std::vector<NpcTemplate> load_npc_templates(const std::string& path);
ServerConfig load_server_config(const std::string& path);

#endif  // CONFIG_H
