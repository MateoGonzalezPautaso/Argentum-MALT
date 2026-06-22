#include <map>
#include <optional>
#include <unordered_map>

#include "common/config.h"
#include "common/item.h"
#include "gtest/gtest.h"
#include "server/core/config.h"
#include "server/game/clan_manager.h"
#include "server/game/combat_controller.h"
#include "server/game/game_formulas.h"
#include "server/game/map.h"
#include "server/game/player.h"
#include "server/persistence/clan_persistence.h"

class CombatControllerTest: public ::testing::Test {
protected:
    std::map<uint16_t, Player> players;
    std::map<uint16_t, EnemyNpc> enemy_npcs;
    AttackConfig config;
    NpcConfig npc_config;
    ItemCatalog item_catalog;
    BalanceConfig balance_config;
    std::unordered_map<std::string, Map> maps;
    ClanConfig clan_config;
    ClanPersistence clan_persistence{"", ""};
    ClanManager clan_manager{clan_persistence, clan_config};
    MessagesConfig msgs_config;
    std::optional<CombatController> controller;
    std::unordered_map<std::string, Map> test_maps;
    std::unordered_map<std::string, TilemapConfig> stored_configs;
    uint32_t next_npc_id = 5000;

    void SetUp() override {
        config.base_damage = 10;
        config.damage_variance = 0;
        config.attack_range_px = 200;
        config.cooldown_ticks = 10;
        config.critical_chance = 0;
        npc_config.vision_range_px = 200;
        npc_config.idle_move_min_ticks = 10;
        npc_config.idle_move_max_ticks = 20;

        controller.emplace(config, players, item_catalog, enemy_npcs, NpcDropConfig{},
                           NpcDropConfig{}, balance_config, npc_config, clan_manager, maps,
                           msgs_config);
    }

    Player& add_player(uint16_t id, const std::string& username, Position pos = {100, 100}) {
        BalanceConfig bal;
        bal.max_level = 100;
        bal.gold_per_level = 10;
        bal.level_exp_base = 1000;
        bal.level_exp_exponent = 1.8;
        bal.gold_cap_base = 1000;
        bal.gold_cap_exponent = 1.0;
        bal.race_stat(Race::HUMAN).agility_factor = 0.0;
        bal.class_stat(PlayerClass::WARRIOR).agility_factor = 0.0;
        bal.race_stat(Race::HUMAN).strength_factor = 0.0;
        bal.class_stat(PlayerClass::WARRIOR).strength_factor = 0.0;
        players.emplace(id, Player(id, username, pos, Direction::SOUTH, Race::HUMAN,
                                   PlayerClass::WARRIOR, bal, 20, 10, 10, 40));
        return players.at(id);
    }

    void set_level(Player& p, int target_level) {
        for (int i = 1; i < target_level; ++i) p.level_up();
    }

    EnemyNpc& add_npc(uint16_t id, const std::string& name, Position pos = {200, 200},
                      uint32_t hp = 100, uint32_t damage = 5, uint8_t level = 5,
                      uint32_t speed = 2) {
        auto [it, _] = enemy_npcs.emplace(
                id, EnemyNpc(pos, hp, damage, entity_rng, item_catalog, level, name, 0, speed));
        return it->second;
    }

    void set_map(const std::string& map_name, int tile_size, int cols, int rows,
                 bool all_walkable = true) {
        TilemapConfig tcfg;
        tcfg.tile_size = tile_size;
        tcfg.map_type = MapType::DUNGEON;
        TileDef td;
        td.walkable = all_walkable;
        tcfg.tiles["ground"] = td;
        tcfg.mapa = std::vector<std::vector<std::string>>(rows,
                                                          std::vector<std::string>(cols, "ground"));
        tcfg.mob_spawn_zones = std::vector<std::vector<bool>>(rows, std::vector<bool>(cols, true));
        stored_configs[map_name] = std::move(tcfg);
        test_maps.try_emplace(map_name, stored_configs[map_name]);
        controller->set_maps(test_maps);
    }

    Rng entity_rng;
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
    set_level(attacker, 5);  // newbie
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
    set_level(target, 5);  // newbie

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
    ASSERT_FALSE(result.targeted_events.at(2).empty());
    EXPECT_TRUE(std::holds_alternative<DamageReceivedEvent>(result.targeted_events.at(2)[0]));
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

    ASSERT_FALSE(result.targeted_events.at(2).empty());
    EXPECT_TRUE(std::holds_alternative<DamageReceivedEvent>(result.targeted_events.at(2)[0]));
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

    ASSERT_FALSE(result.targeted_events.at(2).empty());
    ASSERT_TRUE(std::holds_alternative<DamageReceivedEvent>(result.targeted_events.at(2)[0]));
    const auto& dmg = std::get<DamageReceivedEvent>(result.targeted_events.at(2)[0]);
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

    target.take_damage(target.get_hp_max() - 10);  // leave 10 hp

    auto result = controller->melee_attack(1, 2, 0);

    bool found_died = false;
    for (const auto& ev: result.broadcast_events)
        if (std::holds_alternative<EntityDiedEvent>(ev))
            found_died = true;
    EXPECT_TRUE(found_died);
    EXPECT_TRUE(target.is_dead());
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
    ASSERT_TRUE(attacker.is_dead());

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
    ASSERT_TRUE(target.is_dead());

    auto result = controller->melee_attack(1, 2, 0);
    EXPECT_TRUE(result.broadcast_events.empty());
    EXPECT_TRUE(result.private_events.empty());
}

// ─────────────────────────────────────────────────────────────
// Weapon-based damage formula
// ─────────────────────────────────────────────────────────────

TEST_F(CombatControllerTest, WeaponEquipped_DamageUsesStrength) {
    Item sword;
    sword.type = ItemType::SWORD;
    sword.name = "Espada";
    sword.equip_slot = EquipSlot::WEAPON;
    sword.min_damage = 5;
    sword.max_damage = 5;
    item_catalog.add(sword);
    controller.emplace(config, players, item_catalog, enemy_npcs, NpcDropConfig{}, NpcDropConfig{},
                       balance_config, npc_config, clan_manager, maps, msgs_config);

    auto& attacker = add_player(1, "alice");
    auto& target = add_player(2, "bob");
    set_level(attacker, 15);
    set_level(target, 15);

    attacker.add_item(ItemType::SWORD, "Espada");
    attacker.equip(0, item_catalog);

    auto result = controller->melee_attack(1, 2, 0);

    ASSERT_FALSE(result.targeted_events.at(2).empty());
    ASSERT_TRUE(std::holds_alternative<DamageReceivedEvent>(result.targeted_events.at(2)[0]));
    const auto& dmg = std::get<DamageReceivedEvent>(result.targeted_events.at(2)[0]);
    EXPECT_EQ(dmg.damage, attacker.get_strength() * 5u);
}

// ─────────────────────────────────────────────────────────────
// NPC idle movement
// ─────────────────────────────────────────────────────────────

TEST_F(CombatControllerTest, NpcIdleMove_MovesWhenNoPlayer) {
    set_map("dungeon", 32, 30, 30, true);

    auto& npc = add_npc(5001, "Orc", {320, 320}, 100, 10, 5, 2);
    npc.set_current_map("dungeon");
    npc.set_idle_move_timer(1);

    Position initial = npc.get_pos();
    controller->update_npc_ai(0);

    bool moved_x = npc.get_pos().x != initial.x;
    bool moved_y = npc.get_pos().y != initial.y;
    EXPECT_TRUE(moved_x || moved_y)
            << "NPC should move on idle from position {" << initial.x << "," << initial.y << "}";
}

TEST_F(CombatControllerTest, NpcIdleMove_DeadNpcDoesNotMove) {
    set_map("dungeon", 32, 30, 30, true);

    auto& npc = add_npc(5001, "Orc", {320, 320}, 100, 10, 5, 2);
    npc.set_current_map("dungeon");
    npc.take_damage(npc.get_hp_max());
    ASSERT_TRUE(npc.is_dead());

    Position initial = npc.get_pos();
    controller->update_npc_ai(0);

    EXPECT_EQ(npc.get_pos().x, initial.x) << "Dead NPC should not move";
    EXPECT_EQ(npc.get_pos().y, initial.y) << "Dead NPC should not move";
}

TEST_F(CombatControllerTest, NpcIdleMove_EmitsEntityMoveEvent) {
    set_map("dungeon", 32, 30, 30, true);

    auto& npc = add_npc(5001, "Orc", {320, 320}, 100, 10, 5, 2);
    npc.set_current_map("dungeon");
    npc.set_idle_move_timer(1);

    auto& player = add_player(1, "alice", {100, 100});
    player.set_current_map("dungeon");

    auto result = controller->update_npc_ai(0);

    ASSERT_FALSE(result.targeted_events.empty())
            << "Should emit EntityMoveEvent for players on NPC map";
}

TEST_F(CombatControllerTest, NpcIdleMove_TimerResetsOnBlockedMovement) {
    set_map("dungeon", 32, 30, 30, false);

    auto& npc = add_npc(5001, "Orc", {320, 320}, 100, 10, 5, 2);
    npc.set_current_map("dungeon");
    npc.set_idle_move_timer(1);

    Position initial = npc.get_pos();
    controller->update_npc_ai(0);

    EXPECT_EQ(npc.get_idle_move_timer(), 0u)
            << "Move timer should reset to 0 when movement is blocked";
    EXPECT_EQ(npc.get_pos().x, initial.x) << "NPC should not move on non-walkable tiles";
    EXPECT_EQ(npc.get_pos().y, initial.y) << "NPC should not move on non-walkable tiles";
}
