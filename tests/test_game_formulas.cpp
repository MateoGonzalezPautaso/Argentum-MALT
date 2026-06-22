#include "common/rng.h"
#include "gtest/gtest.h"
#include "server/core/config.h"
#include "server/game/game_formulas.h"

namespace {

BalanceConfig make_balance() {
    BalanceConfig b;
    b.hp.constitution_human = 20;
    b.hp.race_hp_factor_human = 1.0;
    b.hp.class_hp_factor_warrior = 2.0;
    b.mana.intelligence_human = 20;
    b.mana.race_mana_factor_human = 1.0;
    b.mana.class_mana_factor_mage = 2.0;
    b.mana.class_mana_factor_warrior = 0.0;
    b.mana.class_meditation_factor_mage = 2.0;
    b.strength.race_strength_factor_human = 1.0;
    b.strength.class_strength_factor_warrior = 1.5;
    b.agility.race_agility_factor_human = 0.9;
    b.agility.class_agility_factor_warrior = 0.7;
    b.level_exp_base = 1000;
    b.level_exp_exponent = 1.8;
    b.gold_cap_base = 100;
    b.gold_cap_exponent = 1.1;
    b.exp_loss_on_death_ratio = 0.9;
    b.npc_gold_reward_min_pct = 0.01;
    b.npc_gold_reward_max_pct = 0.2;
    b.extra_kill_exp_max_pct = 0.1;
    b.race_recovery.human = 1.0;
    return b;
}

AttackConfig make_attack() {
    AttackConfig a;
    a.base_damage = 10;
    a.damage_variance = 0;
    a.critical_chance = 0.0;
    a.dodge_threshold = 0.001;
    a.clan_bonus_per_member = 0.05;
    a.clan_bonus_max = 0.25;
    return a;
}

}  // namespace

// ─────────────────────────────────────────────────────────────
// Stats del personaje: VidaMax, ManaMax, Fuerza, Agilidad
// ─────────────────────────────────────────────────────────────

TEST(GameFormulasTest, MaxHpFormula) {
    BalanceConfig b = make_balance();
    // VidaMax = Constitucion * FClaseVida * FRazaVida * Nivel = 20 * 2.0 * 1.0 * 5
    EXPECT_EQ(GameFormulas::max_hp(b, Race::HUMAN, PlayerClass::WARRIOR, 5), 200u);
}

TEST(GameFormulasTest, MaxManaFormula) {
    BalanceConfig b = make_balance();
    // ManaMax = Inteligencia * FClaseMana * FRazaMana * Nivel = 20 * 2.0 * 1.0 * 3
    EXPECT_EQ(GameFormulas::max_mana(b, Race::HUMAN, PlayerClass::MAGE, 3), 120u);
}

TEST(GameFormulasTest, MaxManaWarriorIsAlwaysZero) {
    BalanceConfig b = make_balance();
    EXPECT_EQ(GameFormulas::max_mana(b, Race::HUMAN, PlayerClass::WARRIOR, 50), 0u);
}

TEST(GameFormulasTest, StrengthFormula) {
    BalanceConfig b = make_balance();
    // Fuerza = FRazaFuerza * FClaseFuerza * Nivel = 1.0 * 1.5 * 4 = 6
    EXPECT_EQ(GameFormulas::strength(b, Race::HUMAN, PlayerClass::WARRIOR, 4), 6u);
}

TEST(GameFormulasTest, AgilityFormula) {
    BalanceConfig b = make_balance();
    // Agilidad = FRazaAgilidad * FClaseAgilidad * Nivel = 0.9 * 0.7 * 10 = 6.3 -> ceil = 7
    EXPECT_EQ(GameFormulas::agility(b, Race::HUMAN, PlayerClass::WARRIOR, 10), 7u);
}

// ─────────────────────────────────────────────────────────────
// Progresion: experiencia y oro
// ─────────────────────────────────────────────────────────────

TEST(GameFormulasTest, ExpThresholdFormula) {
    BalanceConfig b = make_balance();
    // Limite = 1000 * Nivel^1.8
    EXPECT_EQ(GameFormulas::exp_threshold(b, 1), 1000u);
    EXPECT_GT(GameFormulas::exp_threshold(b, 2), GameFormulas::exp_threshold(b, 1));
}

TEST(GameFormulasTest, ExperienceLossOnDeath) {
    BalanceConfig b = make_balance();
    EXPECT_EQ(GameFormulas::experience_loss_on_death(b, 1000), 900u);
}

TEST(GameFormulasTest, ExperienceLossOnDeathZero) {
    BalanceConfig b = make_balance();
    EXPECT_EQ(GameFormulas::experience_loss_on_death(b, 0), 0u);
}

TEST(GameFormulasTest, GoldCapFormula) {
    BalanceConfig b = make_balance();
    b.gold_cap_base = 100;
    b.gold_cap_exponent = 1.0;
    // OroMax = 100 * Nivel^1.0
    EXPECT_EQ(GameFormulas::gold_cap(b, 10), 1000u);
}

TEST(GameFormulasTest, LevelUpGold) {
    EXPECT_EQ(GameFormulas::level_up_gold(100, 5), 500u);
    EXPECT_EQ(GameFormulas::level_up_gold(0, 5), 0u);
}

// ─────────────────────────────────────────────────────────────
// Combate
// ─────────────────────────────────────────────────────────────

TEST(GameFormulasTest, WeaponDamageDegenerateRange) {
    Rng rng;
    // min == max: el roll esta forzado, sin azar.
    EXPECT_EQ(GameFormulas::weapon_damage(10, 3, 3, rng), 30u);
    EXPECT_EQ(GameFormulas::weapon_damage(0, 3, 3, rng), 0u);
}

TEST(GameFormulasTest, WeaponDamageRespectsRange) {
    Rng rng;
    for (int i = 0; i < 50; ++i) {
        uint32_t dmg = GameFormulas::weapon_damage(2, 1, 5, rng);
        EXPECT_GE(dmg, 2u);
        EXPECT_LE(dmg, 10u);
    }
}

TEST(GameFormulasTest, UnarmedDamageNoVariance) {
    Rng rng;
    AttackConfig a = make_attack();
    a.base_damage = 10;
    a.damage_variance = 0;
    EXPECT_EQ(GameFormulas::unarmed_damage(a, rng), 10u);
}

TEST(GameFormulasTest, UnarmedDamageRespectsVariance) {
    Rng rng;
    AttackConfig a = make_attack();
    a.base_damage = 10;
    a.damage_variance = 5;
    for (int i = 0; i < 50; ++i) {
        uint32_t dmg = GameFormulas::unarmed_damage(a, rng);
        EXPECT_GE(dmg, 10u);
        EXPECT_LE(dmg, 15u);
    }
}

TEST(GameFormulasTest, ApplyClanBonusZero) {
    EXPECT_EQ(GameFormulas::apply_clan_bonus(100, 0.0), 100u);
}

TEST(GameFormulasTest, ApplyClanBonusPositive) {
    // base * (1 + bonus) = 100 * 1.25 = 125
    EXPECT_EQ(GameFormulas::apply_clan_bonus(100, 0.25), 125u);
}

TEST(GameFormulasTest, ObjectDefenseDegenerateRange) {
    Rng rng;
    EXPECT_EQ(GameFormulas::object_defense(7, 7, rng), 7u);
}

TEST(GameFormulasTest, ObjectDefenseRespectsRange) {
    Rng rng;
    for (int i = 0; i < 50; ++i) {
        uint32_t def = GameFormulas::object_defense(2, 6, rng);
        EXPECT_GE(def, 2u);
        EXPECT_LE(def, 6u);
    }
}

TEST(GameFormulasTest, IsDodgedZeroAgilityNeverDodges) {
    // pow(x, 0) == 1, nunca menor al umbral: agilidad 0 nunca esquiva.
    Rng rng;
    AttackConfig a = make_attack();
    for (int i = 0; i < 50; ++i) EXPECT_FALSE(GameFormulas::is_dodged(a, 0, rng));
}

TEST(GameFormulasTest, IsDodgedThresholdZeroNeverDodges) {
    Rng rng;
    AttackConfig a = make_attack();
    a.dodge_threshold = 0.0;
    for (int i = 0; i < 50; ++i) EXPECT_FALSE(GameFormulas::is_dodged(a, 100, rng));
}

TEST(GameFormulasTest, IsCriticalZeroChanceNeverCrits) {
    Rng rng;
    AttackConfig a = make_attack();
    a.critical_chance = 0.0;
    for (int i = 0; i < 50; ++i) EXPECT_FALSE(GameFormulas::is_critical(a, 100, rng));
}

TEST(GameFormulasTest, CritChancePercentSaturatesAt100) {
    AttackConfig a = make_attack();
    a.critical_chance = 1.0;  // exagerado a proposito para forzar saturacion
    EXPECT_EQ(GameFormulas::crit_chance_percent(a, 1000), 100);
}

TEST(GameFormulasTest, CritChancePercentZeroStrength) {
    AttackConfig a = make_attack();
    a.critical_chance = 0.003;
    EXPECT_EQ(GameFormulas::crit_chance_percent(a, 0), 0);
}

TEST(GameFormulasTest, DodgeChancePercentZeroAgility) {
    AttackConfig a = make_attack();
    EXPECT_EQ(GameFormulas::dodge_chance_percent(a, 0), 0);
}

TEST(GameFormulasTest, DodgeChancePercentIncreasesWithAgility) {
    AttackConfig a = make_attack();
    uint8_t low = GameFormulas::dodge_chance_percent(a, 10);
    uint8_t high = GameFormulas::dodge_chance_percent(a, 100);
    EXPECT_GT(high, low);
}

TEST(GameFormulasTest, ClanBonusCapsAtMax) {
    AttackConfig a = make_attack();
    a.clan_bonus_per_member = 0.05;
    a.clan_bonus_max = 0.25;
    EXPECT_DOUBLE_EQ(GameFormulas::clan_bonus(a, 100), 0.25);
}

TEST(GameFormulasTest, ClanBonusZeroAllies) {
    AttackConfig a = make_attack();
    EXPECT_DOUBLE_EQ(GameFormulas::clan_bonus(a, 0), 0.0);
}

TEST(GameFormulasTest, ClanBonusScalesPerMember) {
    AttackConfig a = make_attack();
    a.clan_bonus_per_member = 0.05;
    a.clan_bonus_max = 0.25;
    EXPECT_DOUBLE_EQ(GameFormulas::clan_bonus(a, 3), 0.15);
}

// ─────────────────────────────────────────────────────────────
// Recompensas
// ─────────────────────────────────────────────────────────────

TEST(GameFormulasTest, NpcGoldRewardRespectsRange) {
    Rng rng;
    BalanceConfig b = make_balance();
    for (int i = 0; i < 50; ++i) {
        uint32_t gold = GameFormulas::npc_gold_reward(b, 1000, rng);
        EXPECT_GE(gold, 10u);   // 0.01 * 1000
        EXPECT_LE(gold, 200u);  // 0.2 * 1000
    }
}

TEST(GameFormulasTest, BonusKillExperienceZeroWhenUnderleveled) {
    // level_factor = target_level - attacker_level + offset; si da negativo, se clampea a 0.
    Rng rng;
    BalanceConfig b = make_balance();
    EXPECT_EQ(GameFormulas::bonus_kill_experience(b, 1000, /*attacker=*/50, /*target=*/1, rng), 0u);
}

TEST(GameFormulasTest, BonusKillExperienceZeroOffsetSameLevel) {
    Rng rng;
    BalanceConfig b = make_balance();
    b.extra_kill_exp_max_pct = 0.1;
    b.experience_level_offset = 0;
    for (int i = 0; i < 50; ++i) {
        // level_factor = target(1) - attacker(1) + offset(0) = 0 -> siempre 0.
        uint32_t exp = GameFormulas::bonus_kill_experience(b, 1000, /*attacker=*/1, /*target=*/1,
                                                           rng);
        EXPECT_EQ(exp, 0u);
    }
}

TEST(GameFormulasTest, BonusKillExperienceRespectsRange) {
    Rng rng;
    BalanceConfig b = make_balance();
    b.extra_kill_exp_max_pct = 0.1;
    b.experience_level_offset = 10;
    for (int i = 0; i < 50; ++i) {
        // level_factor = target(1) - attacker(1) + offset(10) = 10
        uint32_t exp = GameFormulas::bonus_kill_experience(b, 1000, /*attacker=*/1, /*target=*/1,
                                                           rng);
        EXPECT_GE(exp, 0u);
        EXPECT_LE(exp, 1000u);  // 0.1 * 1000 * 10
    }
}

// ─────────────────────────────────────────────────────────────
// Escalado de criaturas: HP = HP_base * Nivel, Daño = Daño_base * Nivel
// ─────────────────────────────────────────────────────────────

TEST(GameFormulasTest, NpcHpScalesWithLevel) {
    EXPECT_EQ(GameFormulas::npc_hp(100, 5), 500u);
    EXPECT_EQ(GameFormulas::npc_hp(100, 1), 100u);
}

TEST(GameFormulasTest, NpcDamageScalesWithLevel) {
    EXPECT_EQ(GameFormulas::npc_damage(5, 15), 75u);
}

// ─────────────────────────────────────────────────────────────
// Experiencia de ataque: Daño * max(NivelDelOtro - Nivel + offset, 0)
// ─────────────────────────────────────────────────────────────

TEST(GameFormulasTest, AttackExperienceBasic) {
    // damage=10, target=5, attacker=5, offset=10 -> factor=10 -> 100
    EXPECT_EQ(GameFormulas::attack_experience(10, 5, 5, 10), 100u);
}

TEST(GameFormulasTest, AttackExperienceClampedAtZero) {
    // Atacar a alguien de nivel mucho menor da factor negativo, se clampea a 0.
    EXPECT_EQ(GameFormulas::attack_experience(10, 50, 1, 10), 0u);
}

TEST(GameFormulasTest, AttackExperienceZeroDamage) {
    EXPECT_EQ(GameFormulas::attack_experience(0, 5, 5, 10), 0u);
}

// ─────────────────────────────────────────────────────────────
// Regeneracion pasiva y por meditacion
// ─────────────────────────────────────────────────────────────

TEST(GameFormulasTest, HpRegenPerSecondMatchesRaceFactor) {
    BalanceConfig b = make_balance();
    b.race_recovery.human = 1.5;
    EXPECT_DOUBLE_EQ(GameFormulas::hp_regen_per_second(b, Race::HUMAN), 1.5);
}

TEST(GameFormulasTest, ManaRegenPerSecondMatchesRaceFactor) {
    BalanceConfig b = make_balance();
    b.race_recovery.elf = 2.0;
    EXPECT_DOUBLE_EQ(GameFormulas::mana_regen_per_second(b, Race::ELF), 2.0);
}

TEST(GameFormulasTest, MeditationManaPerSecondFormula) {
    BalanceConfig b = make_balance();
    b.mana.intelligence_human = 20;
    b.mana.class_meditation_factor_mage = 2.0;
    // Mana = FClaseMeditacion * Inteligencia = 2.0 * 20 = 40
    EXPECT_DOUBLE_EQ(GameFormulas::meditation_mana_per_second(b, Race::HUMAN, PlayerClass::MAGE),
                     40.0);
}

TEST(GameFormulasTest, MeditationManaPerSecondWarriorIsZero) {
    BalanceConfig b = make_balance();
    EXPECT_DOUBLE_EQ(
            GameFormulas::meditation_mana_per_second(b, Race::HUMAN, PlayerClass::WARRIOR), 0.0);
}
