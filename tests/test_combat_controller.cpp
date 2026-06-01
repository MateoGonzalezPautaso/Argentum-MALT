#include <map>

#include "gtest/gtest.h"
#include "server/core/config.h"
#include "server/game/combat_controller.h"
#include "server/game/player.h"

class CombatControllerTest: public ::testing::Test {
protected:
    std::map<uint16_t, Player> players;
    AttackConfig config;
    CombatController* controller = nullptr;

    void SetUp() override {
        config.base_damage = 10;
        config.damage_variance = 0;
        config.attack_range_px = 200;
        config.cooldown_ticks = 10;
        controller = new CombatController(config, players);
    }

    void TearDown() override { delete controller; }

    Player& add_player(uint16_t id, const std::string& username, Position pos = {100, 100}) {
        BalanceConfig bal;
        bal.starting_mana = 50;
        bal.max_level = 100;
        bal.mana_per_level = 5;
        bal.gold_per_level = 10;
        bal.level_exp_base = 1000;
        bal.level_exp_exponent = 1.8;
        bal.gold_cap_base = 1000;
        bal.gold_cap_exponent = 1.0;
        players.emplace(id, Player(id, username, pos, Direction::SOUTH, Race::HUMAN,
                                   PlayerClass::WARRIOR, bal));
        return players.at(id);
    }

    // Levels a player up to the given level (from level 1)
    void set_level(Player& p, int target_level) {
        for (int i = 1; i < target_level; ++i)
            p.level_up();
    }
};

// ─────────────────────────────────────────────────────────────
// Self-attack
// ─────────────────────────────────────────────────────────────

TEST_F(CombatControllerTest, SelfAttack_Blocked) {
    auto& p = add_player(1, "alice");
    set_level(p, 15);

    auto result = controller->melee_attack(1, 1, 0);
    EXPECT_TRUE(result.broadcast_events.empty());
    EXPECT_TRUE(result.private_events.empty());
}

// ─────────────────────────────────────────────────────────────
// Fair play: newbie protection
// ─────────────────────────────────────────────────────────────

TEST_F(CombatControllerTest, AttackerIsNewbie_Blocked) {
    auto& attacker = add_player(1, "alice");
    auto& target = add_player(2, "bob");
    set_level(attacker, 5);   // newbie
    set_level(target, 15);

    auto result = controller->melee_attack(1, 2, 0);
    EXPECT_TRUE(result.broadcast_events.empty());
    ASSERT_FALSE(result.private_events.empty());
    ASSERT_TRUE(std::holds_alternative<ChatMsgEvent>(result.private_events[0]));
}

TEST_F(CombatControllerTest, TargetIsNewbie_Blocked) {
    auto& attacker = add_player(1, "alice");
    auto& target = add_player(2, "bob");
    set_level(attacker, 15);
    set_level(target, 5);   // newbie

    auto result = controller->melee_attack(1, 2, 0);
    EXPECT_TRUE(result.broadcast_events.empty());
    ASSERT_FALSE(result.private_events.empty());
    ASSERT_TRUE(std::holds_alternative<ChatMsgEvent>(result.private_events[0]));
}

// ─────────────────────────────────────────────────────────────
// Fair play: level difference cap
// ─────────────────────────────────────────────────────────────

TEST_F(CombatControllerTest, LevelDiffExceedsLimit_Blocked) {
    auto& attacker = add_player(1, "alice");
    auto& target = add_player(2, "bob");
    set_level(attacker, 30);
    set_level(target, 13);  // diff = 17 > 10

    auto result = controller->melee_attack(1, 2, 0);
    EXPECT_TRUE(result.broadcast_events.empty());
    ASSERT_TRUE(std::holds_alternative<ChatMsgEvent>(result.private_events[0]));
}

TEST_F(CombatControllerTest, LevelDiffAtLimit_Allowed) {
    auto& attacker = add_player(1, "alice");
    auto& target = add_player(2, "bob");
    set_level(attacker, 23);
    set_level(target, 13);  // diff = 10, exactly at limit

    auto result = controller->melee_attack(1, 2, 0);
    ASSERT_FALSE(result.broadcast_events.empty());
    EXPECT_TRUE(std::holds_alternative<DamageReceivedEvent>(result.broadcast_events[0]));
}

// ─────────────────────────────────────────────────────────────
// Attack cooldown
// ─────────────────────────────────────────────────────────────

TEST_F(CombatControllerTest, AttackOnCooldown_Blocked) {
    auto& attacker = add_player(1, "alice");
    auto& target = add_player(2, "bob");
    set_level(attacker, 15);
    set_level(target, 15);

    controller->melee_attack(1, 2, 0);
    auto result = controller->melee_attack(1, 2, 5);  // cooldown = 10

    EXPECT_TRUE(result.broadcast_events.empty());
    EXPECT_TRUE(result.private_events.empty());
}

TEST_F(CombatControllerTest, AttackAfterCooldown_Allowed) {
    auto& attacker = add_player(1, "alice");
    auto& target = add_player(2, "bob");
    set_level(attacker, 15);
    set_level(target, 15);

    controller->melee_attack(1, 2, 0);
    auto result = controller->melee_attack(1, 2, 10);  // cooldown = 10

    ASSERT_FALSE(result.broadcast_events.empty());
    EXPECT_TRUE(std::holds_alternative<DamageReceivedEvent>(result.broadcast_events[0]));
}

// ─────────────────────────────────────────────────────────────
// Range check
// ─────────────────────────────────────────────────────────────

TEST_F(CombatControllerTest, OutOfRange_Blocked) {
    auto& attacker = add_player(1, "alice", {0, 0});
    auto& target = add_player(2, "bob", {500, 500});  // far away
    set_level(attacker, 15);
    set_level(target, 15);

    auto result = controller->melee_attack(1, 2, 0);
    EXPECT_TRUE(result.broadcast_events.empty());
    EXPECT_TRUE(result.private_events.empty());
}

// ─────────────────────────────────────────────────────────────
// Successful attack
// ─────────────────────────────────────────────────────────────

TEST_F(CombatControllerTest, SuccessfulAttack_DealsDamage) {
    auto& attacker = add_player(1, "alice");
    auto& target = add_player(2, "bob");
    set_level(attacker, 15);
    set_level(target, 15);

    auto result = controller->melee_attack(1, 2, 0);

    ASSERT_FALSE(result.broadcast_events.empty());
    ASSERT_TRUE(std::holds_alternative<DamageReceivedEvent>(result.broadcast_events[0]));
    const auto& dmg = std::get<DamageReceivedEvent>(result.broadcast_events[0]);
    EXPECT_EQ(dmg.damage, 10u);  // base_damage with variance=0
    EXPECT_EQ(dmg.attacker_id, 1);
    EXPECT_EQ(dmg.target_id, 2);
    EXPECT_LT(target.get_hp_current(), target.get_hp_max());
}

TEST_F(CombatControllerTest, SuccessfulAttack_EmitsDamageDealtToAttacker) {
    auto& attacker = add_player(1, "alice");
    auto& target = add_player(2, "bob");
    set_level(attacker, 15);
    set_level(target, 15);

    auto result = controller->melee_attack(1, 2, 0);

    ASSERT_FALSE(result.private_events.empty());
    ASSERT_TRUE(std::holds_alternative<DamageDealtEvent>(result.private_events[0]));
    EXPECT_EQ(std::get<DamageDealtEvent>(result.private_events[0]).damage, 10u);
}

TEST_F(CombatControllerTest, LethalAttack_EmitsEntityDied) {
    auto& attacker = add_player(1, "alice");
    auto& target = add_player(2, "bob");
    set_level(attacker, 15);
    set_level(target, 15);

    // Deal enough attacks to kill the target
    // target has 100 + 14*10 = 240 hp after 14 level_ups; damage=10 per tick
    // Force kill by reducing hp first
    target.take_damage(target.get_hp_max() - 10);  // leave 10 hp

    auto result = controller->melee_attack(1, 2, 0);

    bool found_died = false;
    for (const auto& ev: result.broadcast_events)
        if (std::holds_alternative<EntityDiedEvent>(ev))
            found_died = true;
    EXPECT_TRUE(found_died);
    EXPECT_TRUE(target.is_ghost());
}

// ─────────────────────────────────────────────────────────────
// Attacker/target not found
// ─────────────────────────────────────────────────────────────

TEST_F(CombatControllerTest, AttackerNotFound_ReturnsEmpty) {
    auto& target = add_player(2, "bob");
    set_level(target, 15);

    auto result = controller->melee_attack(99, 2, 0);
    EXPECT_TRUE(result.broadcast_events.empty());
    EXPECT_TRUE(result.private_events.empty());
}

TEST_F(CombatControllerTest, TargetNotFound_ReturnsEmpty) {
    auto& attacker = add_player(1, "alice");
    set_level(attacker, 15);

    auto result = controller->melee_attack(1, 99, 0);
    EXPECT_TRUE(result.broadcast_events.empty());
    EXPECT_TRUE(result.private_events.empty());
}

// ─────────────────────────────────────────────────────────────
// Dead players cannot fight
// ─────────────────────────────────────────────────────────────

TEST_F(CombatControllerTest, DeadAttacker_Blocked) {
    auto& attacker = add_player(1, "alice");
    auto& target = add_player(2, "bob");
    set_level(attacker, 15);
    set_level(target, 15);

    attacker.take_damage(attacker.get_hp_max());
    ASSERT_TRUE(attacker.is_ghost());

    auto result = controller->melee_attack(1, 2, 0);
    EXPECT_TRUE(result.broadcast_events.empty());
    EXPECT_TRUE(result.private_events.empty());
}

TEST_F(CombatControllerTest, DeadTarget_Blocked) {
    auto& attacker = add_player(1, "alice");
    auto& target = add_player(2, "bob");
    set_level(attacker, 15);
    set_level(target, 15);

    target.take_damage(target.get_hp_max());
    ASSERT_TRUE(target.is_ghost());

    auto result = controller->melee_attack(1, 2, 0);
    EXPECT_TRUE(result.broadcast_events.empty());
    EXPECT_TRUE(result.private_events.empty());
}
