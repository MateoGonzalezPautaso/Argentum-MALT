#include "game_formulas.h"

#include <algorithm>
#include <cmath>

namespace {

struct RaceStatInputs {
    int constitution;
    double hp_factor;
    int intelligence;
    double mana_factor;
    double strength_factor;
    double agility_factor;
};

RaceStatInputs race_inputs(const BalanceConfig& balance, Race race) {
    switch (race) {
        case Race::HUMAN:
            return {balance.hp.constitution_human, balance.hp.race_hp_factor_human,
                    balance.mana.intelligence_human, balance.mana.race_mana_factor_human,
                    balance.strength.race_strength_factor_human,
                    balance.agility.race_agility_factor_human};
        case Race::ELF:
            return {balance.hp.constitution_elf, balance.hp.race_hp_factor_elf,
                    balance.mana.intelligence_elf, balance.mana.race_mana_factor_elf,
                    balance.strength.race_strength_factor_elf,
                    balance.agility.race_agility_factor_elf};
        case Race::DWARF:
            return {balance.hp.constitution_dwarf, balance.hp.race_hp_factor_dwarf,
                    balance.mana.intelligence_dwarf, balance.mana.race_mana_factor_dwarf,
                    balance.strength.race_strength_factor_dwarf,
                    balance.agility.race_agility_factor_dwarf};
        case Race::GNOME:
            return {balance.hp.constitution_gnome, balance.hp.race_hp_factor_gnome,
                    balance.mana.intelligence_gnome, balance.mana.race_mana_factor_gnome,
                    balance.strength.race_strength_factor_gnome,
                    balance.agility.race_agility_factor_gnome};
    }
    return {};
}

struct ClassStatInputs {
    double hp_factor;
    double mana_factor;
    double strength_factor;
    double agility_factor;
};

ClassStatInputs class_inputs(const BalanceConfig& balance, PlayerClass cls) {
    switch (cls) {
        case PlayerClass::WARRIOR:
            return {balance.hp.class_hp_factor_warrior, balance.mana.class_mana_factor_warrior,
                    balance.strength.class_strength_factor_warrior,
                    balance.agility.class_agility_factor_warrior};
        case PlayerClass::PALADIN:
            return {balance.hp.class_hp_factor_paladin, balance.mana.class_mana_factor_paladin,
                    balance.strength.class_strength_factor_paladin,
                    balance.agility.class_agility_factor_paladin};
        case PlayerClass::CLERIC:
            return {balance.hp.class_hp_factor_cleric, balance.mana.class_mana_factor_cleric,
                    balance.strength.class_strength_factor_cleric,
                    balance.agility.class_agility_factor_cleric};
        case PlayerClass::MAGE:
            return {balance.hp.class_hp_factor_mage, balance.mana.class_mana_factor_mage,
                    balance.strength.class_strength_factor_mage,
                    balance.agility.class_agility_factor_mage};
    }
    return {};
}

}  // namespace

uint32_t GameFormulas::max_hp(const BalanceConfig& balance, Race race, PlayerClass cls,
                              uint8_t level) {
    auto r = race_inputs(balance, race);
    auto c = class_inputs(balance, cls);
    return static_cast<uint32_t>(r.constitution * r.hp_factor * c.hp_factor * level);
}

uint32_t GameFormulas::max_mana(const BalanceConfig& balance, Race race, PlayerClass cls,
                                uint8_t level) {
    auto r = race_inputs(balance, race);
    auto c = class_inputs(balance, cls);
    return static_cast<uint32_t>(r.intelligence * r.mana_factor * c.mana_factor * level);
}

uint32_t GameFormulas::strength(const BalanceConfig& balance, Race race, PlayerClass cls,
                                uint8_t level) {
    auto r = race_inputs(balance, race);
    auto c = class_inputs(balance, cls);
    return static_cast<uint32_t>(std::ceil(r.strength_factor * c.strength_factor * level));
}

uint32_t GameFormulas::agility(const BalanceConfig& balance, Race race, PlayerClass cls,
                               uint8_t level) {
    auto r = race_inputs(balance, race);
    auto c = class_inputs(balance, cls);
    return static_cast<uint32_t>(std::ceil(r.agility_factor * c.agility_factor * level));
}

uint32_t GameFormulas::exp_threshold(const BalanceConfig& balance, uint8_t level) {
    return static_cast<uint32_t>(balance.level_exp_base *
                                 std::pow(level, balance.level_exp_exponent));
}

uint32_t GameFormulas::experience_loss_on_death(const BalanceConfig& balance,
                                                uint32_t experience) {
    return static_cast<uint32_t>(experience * balance.exp_loss_on_death_ratio);
}

uint32_t GameFormulas::gold_cap(const BalanceConfig& balance, uint8_t level) {
    return static_cast<uint32_t>(balance.gold_cap_base *
                                 std::pow(level, balance.gold_cap_exponent));
}

uint32_t GameFormulas::weapon_damage(uint32_t strength, uint16_t weapon_min, uint16_t weapon_max,
                                     Rng& rng) {
    int roll = rng.get_random_int(weapon_min, weapon_max);
    return strength * static_cast<uint32_t>(roll);
}

uint32_t GameFormulas::unarmed_damage(const AttackConfig& config, Rng& rng) {
    int variance = config.damage_variance > 0 ? rng.get_random_int(0, config.damage_variance) : 0;
    return static_cast<uint32_t>(config.base_damage + variance);
}

uint32_t GameFormulas::apply_clan_bonus(uint32_t base, double bonus_pct) {
    return static_cast<uint32_t>(std::round(base * (1.0 + bonus_pct)));
}

uint32_t GameFormulas::object_defense(uint16_t min_defense, uint16_t max_defense, Rng& rng) {
    return static_cast<uint32_t>(rng.get_random_int(min_defense, max_defense));
}

bool GameFormulas::is_dodged(const AttackConfig& config, uint32_t agility, Rng& rng) {
    return std::pow(rng.get_random_double(0, 1), agility) < config.dodge_threshold;
}

bool GameFormulas::is_critical(const AttackConfig& config, uint32_t strength, Rng& rng) {
    double critic_probability = strength * config.critical_chance * 100;
    return rng.get_random_double(0, 99) < critic_probability;
}

uint8_t GameFormulas::crit_chance_percent(const AttackConfig& config, uint32_t strength) {
    double threshold = strength * config.critical_chance * 100.0;
    double actual_pct = (threshold / 99.0) * 100.0;
    if (actual_pct > 100.0)
        return 100;
    return static_cast<uint8_t>(std::round(actual_pct));
}

uint8_t GameFormulas::dodge_chance_percent(const AttackConfig& config, uint32_t agility) {
    if (agility == 0)
        return 0;
    double pct = std::pow(config.dodge_threshold, 1.0 / static_cast<double>(agility)) * 100.0;
    return static_cast<uint8_t>(std::round(pct));
}

double GameFormulas::clan_bonus(const AttackConfig& config, int nearby_allies) {
    return std::min(config.clan_bonus_per_member * nearby_allies, config.clan_bonus_max);
}

uint32_t GameFormulas::npc_gold_reward(const BalanceConfig& balance, uint32_t hp_max, Rng& rng) {
    double pct = rng.get_random_double(balance.npc_gold_reward_min_pct,
                                       balance.npc_gold_reward_max_pct);
    return static_cast<uint32_t>(pct * hp_max);
}

uint32_t GameFormulas::bonus_kill_experience(const BalanceConfig& balance,
                                             uint32_t target_hp_max, int level_factor, Rng& rng) {
    double pct = rng.get_random_double(0, balance.extra_kill_exp_max_pct);
    return static_cast<uint32_t>(pct * target_hp_max * level_factor);
}

uint32_t GameFormulas::attack_experience(uint32_t damage, uint8_t attacker_level,
                                         uint8_t target_level) {
    int level_factor = static_cast<int>(target_level) - static_cast<int>(attacker_level) + 10;
    return damage * static_cast<uint32_t>(std::max(level_factor, 0));
}

uint32_t GameFormulas::hp_regen_per_second(double race_recovery_factor) {
    return static_cast<uint32_t>(race_recovery_factor);
}

uint32_t GameFormulas::mana_regen_per_second(double race_recovery_factor) {
    return static_cast<uint32_t>(race_recovery_factor);
}

uint32_t GameFormulas::meditation_mana_per_second(double meditation_factor, uint32_t intelligence) {
    return static_cast<uint32_t>(meditation_factor * static_cast<double>(intelligence));
}
