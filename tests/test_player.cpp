#include "gtest/gtest.h"
#include "server/core/config.h"
#include "server/game/player.h"

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
    return Player(id, "hero", {100, 100}, Direction::SOUTH, Race::HUMAN, PlayerClass::WARRIOR,
                  make_balance(), 20, 10, 10, 40);
}

Player make_mage_player(uint16_t id = 1) {
    return Player(id, "mage", {100, 100}, Direction::SOUTH, Race::HUMAN, PlayerClass::MAGE,
                  make_balance(), 20, 10, 10, 40);
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
    EXPECT_FALSE(p.is_dead());
}

TEST(PlayerTest, TakeDamage_ExactKill) {
    auto p = make_player();
    p.take_damage(p.get_hp_max());
    EXPECT_EQ(p.get_hp_current(), 0u);
    EXPECT_TRUE(p.is_dead());
}

TEST(PlayerTest, TakeDamage_OverkillClampedToZero) {
    auto p = make_player();
    p.take_damage(999);
    EXPECT_EQ(p.get_hp_current(), 0u);
    EXPECT_TRUE(p.is_dead());
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
    ASSERT_TRUE(p.is_dead());

    p.resurrect();

    EXPECT_FALSE(p.is_dead());
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

// ─────────────────────────────────────────────────────────────
// equip / unequip
// ─────────────────────────────────────────────────────────────

namespace {

ItemCatalog make_test_catalog() {
    ItemCatalog catalog;

    Item sword;
    sword.type = ItemType::SWORD;
    sword.name = "Espada";
    sword.equip_slot = EquipSlot::WEAPON;
    catalog.add(sword);

    Item armor;
    armor.type = ItemType::LEATHER_ARMOR;
    armor.name = "Armadura";
    armor.equip_slot = EquipSlot::ARMOR;
    catalog.add(armor);

    Item health_potion;
    health_potion.type = ItemType::HEALTH_POTION;
    health_potion.name = "Pocion";
    health_potion.equip_slot = EquipSlot::CONSUMABLE;
    health_potion.restore_hp_percent = 100;
    catalog.add(health_potion);

    Item mana_potion;
    mana_potion.type = ItemType::MANA_POTION;
    mana_potion.name = "Mana";
    mana_potion.equip_slot = EquipSlot::CONSUMABLE;
    mana_potion.restore_mana_percent = 100;
    catalog.add(mana_potion);

    Item helmet;
    helmet.type = ItemType::IRON_HELMET;
    helmet.name = "Casco";
    helmet.equip_slot = EquipSlot::HELMET;
    catalog.add(helmet);

    return catalog;
}

}  // namespace

TEST(PlayerTest, Equip_InvalidSlotIndexReturnsFalse) {
    auto p = make_player();
    ItemCatalog catalog = make_test_catalog();
    EXPECT_FALSE(p.equip(99, catalog));
}

TEST(PlayerTest, Equip_EmptySlotReturnsFalse) {
    auto p = make_player();
    ItemCatalog catalog = make_test_catalog();
    EXPECT_FALSE(p.equip(0, catalog));
}

TEST(PlayerTest, Equip_ItemNotInCatalogReturnsFalse) {
    auto p = make_player();
    p.add_item(ItemType::AXE, "Hacha");
    ItemCatalog catalog;
    EXPECT_FALSE(p.equip(0, catalog));
}

TEST(PlayerTest, Equip_SuccessMovesItemToEquippedSlot) {
    auto p = make_player();
    ItemCatalog catalog = make_test_catalog();
    p.add_item(ItemType::SWORD, "Espada");
    EXPECT_TRUE(p.equip(0, catalog));

    const InventorySlot& slot = p.get_equipped(EquipSlot::WEAPON);
    EXPECT_EQ(slot.item_type, ItemType::SWORD);
    EXPECT_EQ(slot.item_name, "Espada");

    EXPECT_TRUE(p.dump_inventory()[0].item_type == ItemType::NONE);
}

TEST(PlayerTest, Equip_ReplacesExistingEquippedItem) {
    auto p = make_player();
    ItemCatalog catalog = make_test_catalog();

    p.add_item(ItemType::SWORD, "Espada");
    p.equip(0, catalog);  // WEAPON = Espada, inventory slot 0 now empty

    p.add_item(ItemType::SWORD, "SegundaEspada");  // goes to slot 0
    p.equip(0, catalog);                           // WEAPON = SegundaEspada, old goes to slot 0

    const InventorySlot& weapon_slot = p.get_equipped(EquipSlot::WEAPON);
    EXPECT_EQ(weapon_slot.item_type, ItemType::SWORD);
    EXPECT_EQ(weapon_slot.item_name, "SegundaEspada");

    InventorySlot inv_slot = p.dump_inventory()[0];
    EXPECT_EQ(inv_slot.item_type, ItemType::SWORD);
    EXPECT_EQ(inv_slot.item_name, "Espada");
}

TEST(PlayerTest, Equip_ConsumableHealthPotionHealsToFull) {
    auto p = make_player();
    ItemCatalog catalog = make_test_catalog();

    p.take_damage(50);
    ASSERT_LT(p.get_hp_current(), p.get_hp_max());

    p.add_item(ItemType::HEALTH_POTION, "Pocion");
    EXPECT_TRUE(p.equip(20, catalog));

    EXPECT_EQ(p.get_hp_current(), p.get_hp_max());
    EXPECT_TRUE(p.dump_inventory()[20].item_type == ItemType::NONE);
}

TEST(PlayerTest, Equip_ConsumableManaPotionRestoresMana) {
    auto p = make_mage_player();
    ItemCatalog catalog = make_test_catalog();

    p.use_mana(p.get_mana_max() / 2);
    ASSERT_LT(p.get_mana_current(), p.get_mana_max());

    p.add_item(ItemType::MANA_POTION, "Mana");
    EXPECT_TRUE(p.equip(30, catalog));

    EXPECT_EQ(p.get_mana_current(), p.get_mana_max());
    EXPECT_TRUE(p.dump_inventory()[30].item_type == ItemType::NONE);
}

TEST(PlayerTest, Unequip_MovesItemToInventory) {
    auto p = make_player();
    ItemCatalog catalog = make_test_catalog();

    p.add_item(ItemType::SWORD, "Espada");
    p.equip(0, catalog);

    p.unequip(EquipSlot::WEAPON);

    const InventorySlot& weapon_slot = p.get_equipped(EquipSlot::WEAPON);
    EXPECT_EQ(weapon_slot.item_type, ItemType::NONE);

    InventorySlot inv_slot = p.dump_inventory()[0];
    EXPECT_EQ(inv_slot.item_type, ItemType::SWORD);
    EXPECT_EQ(inv_slot.item_name, "Espada");
}

TEST(PlayerTest, Unequip_EmptySlotDoesNothing) {
    auto p = make_player();
    p.unequip(EquipSlot::WEAPON);
    // No crash, state unchanged
    EXPECT_EQ(p.get_equipped(EquipSlot::WEAPON).item_type, ItemType::NONE);
}

TEST(PlayerTest, Unequip_FullInventoryDoesNothing) {
    auto p = make_player();
    ItemCatalog catalog = make_test_catalog();

    p.add_item(ItemType::SWORD, "Espada");
    p.equip(0, catalog);

    // Fill the inventory (capacity=20)
    for (int i = 0; i < 20; ++i) {
        p.add_item(ItemType::GOLD_DROP, "Oro");
    }

    p.unequip(EquipSlot::WEAPON);

    // Weapon should still be equipped
    const InventorySlot& weapon_slot = p.get_equipped(EquipSlot::WEAPON);
    EXPECT_EQ(weapon_slot.item_type, ItemType::SWORD);
}

TEST(PlayerTest, GetEquipped_ReturnsDefaultForEmptySlot) {
    auto p = make_player();
    const InventorySlot& slot = p.get_equipped(EquipSlot::SHIELD);
    EXPECT_EQ(slot.item_type, ItemType::NONE);
}

TEST(PlayerTest, DumpEquipped_RestoreEquipment) {
    auto p = make_player();
    ItemCatalog catalog = make_test_catalog();

    p.add_item(ItemType::SWORD, "Espada");
    p.add_item(ItemType::LEATHER_ARMOR, "Armadura");
    p.equip(0, catalog);
    p.equip(1, catalog);

    InventorySlot dumped[EQUIP_SLOT_COUNT];
    p.dump_equipped(dumped);
    EXPECT_EQ(dumped[0].item_type, ItemType::SWORD);
    EXPECT_EQ(dumped[1].item_type, ItemType::LEATHER_ARMOR);
    EXPECT_EQ(dumped[2].item_type, ItemType::NONE);

    // Clear equipped and restore
    p.unequip(EquipSlot::WEAPON);
    p.unequip(EquipSlot::ARMOR);
    p.restore_equipment(dumped);

    EXPECT_EQ(p.get_equipped(EquipSlot::WEAPON).item_type, ItemType::SWORD);
    EXPECT_EQ(p.get_equipped(EquipSlot::ARMOR).item_type, ItemType::LEATHER_ARMOR);
}

TEST(PlayerTest, Equip_DifferentSlots) {
    auto p = make_player();
    ItemCatalog catalog = make_test_catalog();

    p.add_item(ItemType::SWORD, "Espada");
    p.add_item(ItemType::LEATHER_ARMOR, "Armadura");
    p.add_item(ItemType::IRON_HELMET, "Casco");

    p.equip(0, catalog);
    p.equip(1, catalog);
    p.equip(2, catalog);

    EXPECT_EQ(p.get_equipped(EquipSlot::WEAPON).item_type, ItemType::SWORD);
    EXPECT_EQ(p.get_equipped(EquipSlot::ARMOR).item_type, ItemType::LEATHER_ARMOR);
    EXPECT_EQ(p.get_equipped(EquipSlot::HELMET).item_type, ItemType::IRON_HELMET);
}

TEST(PlayerTest, AddItem_PlacesInInventory) {
    auto p = make_player();
    EXPECT_TRUE(p.add_item(ItemType::SWORD, "Espada"));
    EXPECT_EQ(p.dump_inventory()[0].item_type, ItemType::SWORD);
}

TEST(PlayerTest, AddItem_FailsWhenInventoryFull) {
    auto p = make_player();
    for (int i = 0; i < 20; ++i) {
        p.add_item(ItemType::GOLD_DROP, "Oro");
    }
    EXPECT_FALSE(p.add_item(ItemType::SWORD, "Espada"));
}
