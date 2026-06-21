#ifndef GAME_FORMULAS_H
#define GAME_FORMULAS_H

#include <cstdint>

#include "../../common/messages.h"
#include "../../common/rng.h"
#include "../core/config.h"

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

    // Experiencia de ataque: Daño * max(NivelDelOtro - Nivel + 10, 0)
    static uint32_t attack_experience(uint32_t damage, uint8_t attacker_level,
                                      uint8_t target_level);

    // Recuperación pasiva por segundo: FRazaRecuperacion * 1s
    static uint32_t hp_regen_per_second(double race_recovery_factor);
    static uint32_t mana_regen_per_second(double race_recovery_factor);

    // Recuperación por meditación por segundo: FClaseMeditacion * Inteligencia * 1s
    static uint32_t meditation_mana_per_second(double meditation_factor, uint32_t intelligence);
};

#endif  // GAME_FORMULAS_H
