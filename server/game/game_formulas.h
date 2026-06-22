#ifndef GAME_FORMULAS_H
#define GAME_FORMULAS_H

#include <cstdint>
#include <utility>

#include "../../common/item_catalog.h"
#include "../../common/messages.h"
#include "../../common/rng.h"
#include "../core/config.h"
#include "player.h"

class GameFormulas {
public:
    // Stats del personaje (Vida/Mana/Fuerza/Agilidad maximos)
    static uint32_t max_hp(const BalanceConfig& balance, Race race, PlayerClass cls,
                           uint8_t level);
    static uint32_t max_mana(const BalanceConfig& balance, Race race, PlayerClass cls,
                             uint8_t level);
    static uint32_t strength(const BalanceConfig& balance, Race race, PlayerClass cls,
                             uint8_t level);
    static uint32_t agility(const BalanceConfig& balance, Race race, PlayerClass cls,
                            uint8_t level);

    // Progresion
    static uint32_t exp_threshold(const BalanceConfig& balance, uint8_t level);
    static uint32_t experience_loss_on_death(const BalanceConfig& balance, uint32_t experience);
    static uint32_t gold_cap(const BalanceConfig& balance, uint8_t level);

    // Combate
    static uint32_t weapon_damage(uint32_t strength, uint16_t weapon_min, uint16_t weapon_max,
                                  Rng& rng);
    static uint32_t unarmed_damage(const AttackConfig& config, Rng& rng);
    static uint32_t apply_clan_bonus(uint32_t base, double bonus_pct);
    static uint32_t object_defense(uint16_t min_defense, uint16_t max_defense, Rng& rng);
    static bool is_dodged(const AttackConfig& config, uint32_t agility, Rng& rng);
    static bool is_critical(const AttackConfig& config, uint32_t strength, Rng& rng);
    static uint8_t crit_chance_percent(const AttackConfig& config, uint32_t strength);
    static uint8_t dodge_chance_percent(const AttackConfig& config, uint32_t agility);
    static double clan_bonus(const AttackConfig& config, int nearby_allies);

    // Recompensas
    static uint32_t npc_gold_reward(const BalanceConfig& balance, uint32_t hp_max, Rng& rng);
    static uint32_t bonus_kill_experience(const BalanceConfig& balance, uint32_t target_hp_max,
                                          uint8_t attacker_level, uint8_t target_level, Rng& rng);
    static uint32_t level_up_gold(uint32_t gold_per_level, uint8_t level);

    // NPC stat scaling: HP = HP_base * Nivel, Daño = Daño_base * Nivel
    static uint32_t npc_hp(uint32_t base_hp, uint8_t level);
    static uint32_t npc_damage(uint32_t base_damage, uint8_t level);

    // Experiencia de ataque: Daño * max(NivelDelOtro - Nivel + level_diff_offset, 0)
    static uint32_t attack_experience(uint32_t damage, uint8_t attacker_level,
                                       uint8_t target_level, int level_diff_offset);

    // Hechizo de auto-curación: cura hp_max / 2
    static uint32_t spell_self_heal(uint32_t hp_max);

    // Hard cap de oro: OroMax + OroMax * gold_excess_ratio
    static uint64_t gold_hard_cap(uint64_t max_gold, double gold_excess_ratio);

    // Restauración por poción: stat_max * restore_percent / 100 (HP y maná)
    static uint32_t potion_restore(uint32_t stat_max, uint8_t restore_percent);

    // Rango de daño desarmado: [base_damage, base_damage + damage_variance]
    static std::pair<uint16_t, uint16_t> unarmed_damage_range(const AttackConfig& config);

    // Rango de daño mostrado al jugador: fuerza * [arma_min, arma_max] o desarmado
    static std::pair<uint16_t, uint16_t> display_damage_range(const Player& player,
                                                               const ItemCatalog& catalog,
                                                               const AttackConfig& config);

    // Rango de defensa mostrado al jugador: suma de [min,max] de armadura, casco y escudo
    static std::pair<uint16_t, uint16_t> display_defense_range(const Player& player,
                                                                const ItemCatalog& catalog);

    // Recuperación pasiva por segundo: Vida/Mana = FRazaRecuperacion * segundos
    static double hp_regen_per_second(const BalanceConfig& balance, Race race);
    static double mana_regen_per_second(const BalanceConfig& balance, Race race);

    // Recuperación por meditación por segundo: Mana = FClaseMeditacion * Inteligencia * segundos
    static double meditation_mana_per_second(const BalanceConfig& balance, Race race,
                                             PlayerClass cls);
};

#endif  // GAME_FORMULAS_H
