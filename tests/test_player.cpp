#include "gtest/gtest.h"
#include "server/game/player.h"
#include "server/core/config.h"

namespace {

BalanceConfig make_balance() {
    BalanceConfig bal;
    bal.starting_gold = 0;
    bal.max_level = 100;
    bal.gold_per_level = 10;
    bal.level_exp_base = 1000;
    bal.level_exp_exponent = 1.8;
    bal.gold_cap_base = 1000;
    bal.gold_cap_exponent = 1.0;
    return bal;
}

Player make_player(uint16_t id = 1) {
    return Player(id, "hero", {100, 100}, Direction::SOUTH, Race::HUMAN, PlayerClass::WARRIOR, make_balance(), 20);
}

Player make_mage_player(uint16_t id = 1) {
    return Player(id, "mage", {100, 100}, Direction::SOUTH, Race::HUMAN, PlayerClass::MAGE, make_balance(), 20);
}
}

}  // namespace

// ─────────────────────────────────────────────────────────────
// take_damage
// ─────────────────────────────────────────────────────────────

TEST(PlayerTest, TakeDamage_ReducesHp) {
    auto p = make_player();
    uint32_t damage = p.get_hp_max() / 2;
    p.take_damage(damage);
    EXPECT_EQ(p.get_hp_current(), p.get_hp_max() - damage);
    EXPECT_FALSE(p.is_ghost());
}

TEST(PlayerTest, TakeDamage_ExactKill) {
    auto p = make_player();
    p.take_damage(p.get_hp_max());
    EXPECT_EQ(p.get_hp_current(), 0u);
    EXPECT_TRUE(p.is_ghost());
}

TEST(PlayerTest, TakeDamage_OverkillClampedToZero) {
    auto p = make_player();
    p.take_damage(999);
    EXPECT_EQ(p.get_hp_current(), 0u);
    EXPECT_TRUE(p.is_ghost());
}

// ─────────────────────────────────────────────────────────────
// heal
// ─────────────────────────────────────────────────────────────

TEST(PlayerTest, Heal_IncreasesHp) {
    auto p = make_player();
    uint32_t max = p.get_hp_max();
    p.take_damage(max / 2);
    p.heal(max / 4);
    EXPECT_EQ(p.get_hp_current(), max - max / 2 + max / 4);
}

TEST(PlayerTest, Heal_CappedAtMax) {
    auto p = make_player();
    p.take_damage(10);
    p.heal(999);
    EXPECT_EQ(p.get_hp_current(), p.get_hp_max());
}

// ─────────────────────────────────────────────────────────────
// resurrect
// ─────────────────────────────────────────────────────────────

TEST(PlayerTest, Resurrect_RestoresHpAndMana) {
    auto p = make_player();
    p.take_damage(999);
    ASSERT_TRUE(p.is_ghost());

    p.resurrect();

    EXPECT_FALSE(p.is_ghost());
    EXPECT_EQ(p.get_hp_current(), p.get_hp_max());
    EXPECT_EQ(p.get_mana_current(), p.get_mana_max());
}

// ─────────────────────────────────────────────────────────────
// gain_experience / level_up
// ─────────────────────────────────────────────────────────────

TEST(PlayerTest, GainExperience_TriggersLevelUp) {
    auto p = make_mage_player();
    EXPECT_EQ(p.get_level(), 1);
    uint32_t hp_before = p.get_hp_max();
    uint32_t mana_before = p.get_mana_max();

    uint32_t threshold = p.exp_to_next_level();
    p.gain_experience(threshold);

    EXPECT_EQ(p.get_level(), 2);
    EXPECT_GT(p.get_hp_max(), hp_before);
    EXPECT_GT(p.get_mana_max(), mana_before);
}

TEST(PlayerTest, GainExperience_ExcessCarriedOver) {
    auto p = make_player();
    uint32_t threshold = p.exp_to_next_level();
    p.gain_experience(threshold + 50);

    EXPECT_EQ(p.get_level(), 2);
    EXPECT_EQ(p.get_experience(), 50u);
}

TEST(PlayerTest, GainExperience_MultipleLevelUps) {
    auto p = make_player();

    // Give enough exp to cross several thresholds
    uint32_t big_exp = 0;
    for (int i = 1; i <= 5; ++i) {
        BalanceConfig bal;
        bal.level_exp_base = 1000;
        bal.level_exp_exponent = 1.8;
        big_exp += static_cast<uint32_t>(bal.level_exp_base * std::pow(i, bal.level_exp_exponent));
    }
    p.gain_experience(big_exp);

    EXPECT_GE(p.get_level(), 5);
}

TEST(PlayerTest, LevelUp_IncreasesMaxHpAndMana) {
    auto p = make_mage_player();
    uint32_t hp_before = p.get_hp_max();
    uint32_t mana_before = p.get_mana_max();

    p.level_up();

    EXPECT_GT(p.get_hp_max(), hp_before);
    EXPECT_GT(p.get_mana_max(), mana_before);
}

TEST(PlayerTest, LevelUp_RestoresCurrentHpAndMana) {
    auto p = make_player();
    p.take_damage(50);
    p.level_up();

    EXPECT_EQ(p.get_hp_current(), p.get_hp_max());
    EXPECT_EQ(p.get_mana_current(), p.get_mana_max());
}

// ─────────────────────────────────────────────────────────────
// gain_gold / spend_gold
// ─────────────────────────────────────────────────────────────

TEST(PlayerTest, GainGold_IncreasesGold) {
    auto p = make_player();
    p.gain_gold(100);
    EXPECT_EQ(p.get_gold(), 100u);
}

TEST(PlayerTest, GainGold_CappedAt150PercentOfOroMax) {
    auto p = make_player();
    // gold_cap_base=1000, gold_cap_exponent=1.0, level=1 → max=1000, hard_cap=1500
    p.gain_gold(9999);
    EXPECT_LE(p.get_gold(), 1500u);
}

TEST(PlayerTest, GainGold_ExactHardCap) {
    auto p = make_player();
    p.gain_gold(1500);
    EXPECT_EQ(p.get_gold(), 1500u);

    p.gain_gold(1);
    EXPECT_EQ(p.get_gold(), 1500u);
}

TEST(PlayerTest, SpendGold_DeductsAmount) {
    auto p = make_player();
    p.gain_gold(200);
    p.spend_gold(50);
    EXPECT_EQ(p.get_gold(), 150u);
}

TEST(PlayerTest, SpendGold_UnderflowClampedToZero) {
    auto p = make_player();
    p.gain_gold(10);
    p.spend_gold(999);
    EXPECT_EQ(p.get_gold(), 0u);
}

// ─────────────────────────────────────────────────────────────
// use_mana
// ─────────────────────────────────────────────────────────────

TEST(PlayerTest, UseMana_DeductsAmount) {
    auto p = make_mage_player();
    uint32_t amount = p.get_mana_max() / 4;
    p.use_mana(amount);
    EXPECT_EQ(p.get_mana_current(), p.get_mana_max() - amount);
}

TEST(PlayerTest, UseMana_UnderflowClampedToZero) {
    auto p = make_mage_player();
    p.use_mana(p.get_mana_max() * 10);
    EXPECT_EQ(p.get_mana_current(), 0u);
}

// ─────────────────────────────────────────────────────────────
// try_attack cooldown
// ─────────────────────────────────────────────────────────────

TEST(PlayerTest, TryAttack_AllowsFirstAttack) {
    auto p = make_player();
    EXPECT_TRUE(p.try_attack(0, 10));
}

TEST(PlayerTest, TryAttack_BlocksOnCooldown) {
    auto p = make_player();
    p.try_attack(0, 10);
    EXPECT_FALSE(p.try_attack(5, 10));
}

TEST(PlayerTest, TryAttack_AllowsAfterCooldown) {
    auto p = make_player();
    p.try_attack(0, 10);
    EXPECT_TRUE(p.try_attack(10, 10));
}
