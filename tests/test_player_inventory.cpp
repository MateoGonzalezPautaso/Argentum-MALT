#include "gtest/gtest.h"
#include "server/game/player_inventory.h"

TEST(PlayerInventoryTest, Constructor_SetsCapacities) {
    PlayerInventory inv(20, 5, 3);
    EXPECT_EQ(inv.slot_count(), 28);
    EXPECT_EQ(inv.equip_capacity, 20);
    EXPECT_EQ(inv.hp_capacity, 5);
    EXPECT_EQ(inv.mana_capacity, 3);
}

TEST(PlayerInventoryTest, Place_RoutesEquipItemToEquip) {
    PlayerInventory inv(20, 5, 3);
    EXPECT_TRUE(inv.place(ItemType::SWORD, "Espada"));
    EXPECT_EQ(inv.at(0).item_type, ItemType::SWORD);
    EXPECT_EQ(inv.equip.at(0).item_type, ItemType::SWORD);
    EXPECT_EQ(inv.hp_potions.occupied_slots(), 0);
    EXPECT_EQ(inv.mana_potions.occupied_slots(), 0);
}

TEST(PlayerInventoryTest, Place_RoutesHealthPotionToHpInventory) {
    PlayerInventory inv(20, 5, 3);
    EXPECT_TRUE(inv.place(ItemType::HEALTH_POTION, "Pocion de Vida"));
    EXPECT_EQ(inv.at(20).item_type, ItemType::HEALTH_POTION);
    EXPECT_EQ(inv.hp_potions.at(0).item_type, ItemType::HEALTH_POTION);
    EXPECT_EQ(inv.equip.occupied_slots(), 0);
    EXPECT_EQ(inv.mana_potions.occupied_slots(), 0);
}

TEST(PlayerInventoryTest, Place_RoutesManaPotionToManaInventory) {
    PlayerInventory inv(20, 5, 3);
    EXPECT_TRUE(inv.place(ItemType::MANA_POTION, "Pocion de Mana"));
    EXPECT_EQ(inv.at(25).item_type, ItemType::MANA_POTION);
    EXPECT_EQ(inv.mana_potions.at(0).item_type, ItemType::MANA_POTION);
    EXPECT_EQ(inv.equip.occupied_slots(), 0);
    EXPECT_EQ(inv.hp_potions.occupied_slots(), 0);
}

TEST(PlayerInventoryTest, IsEmptyAt_RoutesCorrectly) {
    PlayerInventory inv(20, 5, 3);
    EXPECT_TRUE(inv.is_empty_at(0));
    EXPECT_TRUE(inv.is_empty_at(20));
    EXPECT_TRUE(inv.is_empty_at(25));

    inv.hp_potions.place(ItemType::HEALTH_POTION, "Pocion");
    EXPECT_FALSE(inv.is_empty_at(20));
    EXPECT_TRUE(inv.is_empty_at(0));
}

TEST(PlayerInventoryTest, Clear_RoutesCorrectly) {
    PlayerInventory inv(20, 5, 3);
    inv.hp_potions.place(ItemType::HEALTH_POTION, "Pocion");
    EXPECT_EQ(inv.hp_potions.occupied_slots(), 1);
    inv.clear(20);
    EXPECT_EQ(inv.hp_potions.occupied_slots(), 0);
}

TEST(PlayerInventoryTest, PlaceAt_RoutesCorrectly) {
    PlayerInventory inv(20, 5, 3);
    EXPECT_TRUE(inv.place_at(20, ItemType::HEALTH_POTION, "Pocion"));
    EXPECT_EQ(inv.hp_potions.at(0).item_type, ItemType::HEALTH_POTION);
    EXPECT_TRUE(inv.place_at(25, ItemType::MANA_POTION, "Mana"));
    EXPECT_EQ(inv.mana_potions.at(0).item_type, ItemType::MANA_POTION);
}

TEST(PlayerInventoryTest, DumpSlots_ConcatenatesAllThree) {
    PlayerInventory inv(20, 5, 3);
    inv.equip.place(ItemType::SWORD, "Espada");
    inv.hp_potions.place(ItemType::HEALTH_POTION, "HP");
    inv.mana_potions.place(ItemType::MANA_POTION, "MP");

    auto all = inv.dump_slots();
    EXPECT_EQ(all.size(), 28);
    EXPECT_EQ(all[0].item_type, ItemType::SWORD);
    EXPECT_EQ(all[20].item_type, ItemType::HEALTH_POTION);
    EXPECT_EQ(all[20].slot_index, 20);
    EXPECT_EQ(all[25].item_type, ItemType::MANA_POTION);
    EXPECT_EQ(all[25].slot_index, 25);
}

TEST(PlayerInventoryTest, SlotCount_ReturnsTotal) {
    PlayerInventory inv(10, 4, 6);
    EXPECT_EQ(inv.slot_count(), 20);
}

TEST(PlayerInventoryTest, IsFull_ChecksEquipOnly) {
    PlayerInventory inv(2, 5, 5);
    EXPECT_FALSE(inv.is_full());
    inv.equip.place(ItemType::SWORD, "A");
    inv.equip.place(ItemType::AXE, "B");
    EXPECT_TRUE(inv.is_full());
    inv.hp_potions.place(ItemType::HEALTH_POTION, "HP");
    EXPECT_TRUE(inv.is_full());  // hp_potions doesn't affect equip fullness
}

TEST(PlayerInventoryTest, ToRecords_RoundTrip) {
    PlayerInventory inv(20, 5, 3);
    inv.equip.place(ItemType::SWORD, "Espada");
    inv.hp_potions.place(ItemType::HEALTH_POTION, "HP");
    inv.mana_potions.place(ItemType::MANA_POTION, "MP");

    auto records = inv.to_records();
    EXPECT_EQ(records.size(), 28);

    PlayerInventory inv2(20, 5, 3);
    inv2.from_records(records);
    EXPECT_EQ(inv2.at(0).item_type, ItemType::SWORD);
    EXPECT_EQ(inv2.at(20).item_type, ItemType::HEALTH_POTION);
    EXPECT_EQ(inv2.at(25).item_type, ItemType::MANA_POTION);
}

TEST(PlayerInventoryTest, LoadSlots_SplitsCorrectly) {
    PlayerInventory inv(20, 5, 3);
    std::vector<InventorySlot> slots;
    for (uint8_t i = 0; i < 28; ++i) {
        slots.push_back({i, ItemType::NONE, ""});
    }
    slots[1].item_type = ItemType::SWORD;
    slots[1].item_name = "B";
    slots[20].item_type = ItemType::HEALTH_POTION;
    slots[20].item_name = "C";
    slots[25].item_type = ItemType::MANA_POTION;
    slots[25].item_name = "D";

    inv.load_slots(slots);
    EXPECT_EQ(inv.at(1).item_type, ItemType::SWORD);
    EXPECT_EQ(inv.at(20).item_type, ItemType::HEALTH_POTION);
    EXPECT_EQ(inv.at(25).item_type, ItemType::MANA_POTION);
}

TEST(PlayerInventoryTest, ClearAll_ClearsAllThree) {
    PlayerInventory inv(20, 5, 3);
    inv.equip.place(ItemType::SWORD, "Espada");
    inv.hp_potions.place(ItemType::HEALTH_POTION, "HP");
    inv.mana_potions.place(ItemType::MANA_POTION, "MP");

    inv.clear_all();
    EXPECT_EQ(inv.equip.occupied_slots(), 0);
    EXPECT_EQ(inv.hp_potions.occupied_slots(), 0);
    EXPECT_EQ(inv.mana_potions.occupied_slots(), 0);
}

TEST(PlayerInventoryTest, Place_FailsWhenSubInventoryFull) {
    PlayerInventory inv(20, 1, 1);
    // Fill potion inventories
    EXPECT_TRUE(inv.place(ItemType::HEALTH_POTION, "HP1"));
    EXPECT_FALSE(inv.place(ItemType::HEALTH_POTION, "HP2"));
    EXPECT_TRUE(inv.place(ItemType::MANA_POTION, "MP1"));
    EXPECT_FALSE(inv.place(ItemType::MANA_POTION, "MP2"));
}
