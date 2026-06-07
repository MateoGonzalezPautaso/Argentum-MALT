#include "gtest/gtest.h"
#include "server/game/inventory.h"

TEST(InventoryTest, Constructor_InitializesAllSlotsToNone) {
    Inventory inv(5);
    EXPECT_EQ(inv.slot_count(), 5);
    EXPECT_EQ(inv.occupied_slots(), 0);
    for (uint8_t i = 0; i < 5; ++i) {
        EXPECT_TRUE(inv.is_empty_at(i));
    }
}

TEST(InventoryTest, SlotCount_MatchesCapacity) {
    Inventory inv(10);
    EXPECT_EQ(inv.slot_count(), 10);
}

TEST(InventoryTest, OccupiedSlots_InitiallyZero) {
    Inventory inv(3);
    EXPECT_EQ(inv.occupied_slots(), 0);
}

TEST(InventoryTest, IsEmptyAt_OutOfBoundsReturnsTrue) {
    Inventory inv(3);
    EXPECT_TRUE(inv.is_empty_at(99));
}

TEST(InventoryTest, HasFreeSlot_InitiallyTrue) {
    Inventory inv(2);
    EXPECT_TRUE(inv.has_free_slot());
}

TEST(InventoryTest, HasFreeSlot_FalseWhenFull) {
    Inventory inv(1);
    inv.place(ItemType::SWORD, "Espada");
    EXPECT_FALSE(inv.has_free_slot());
}

TEST(InventoryTest, IsFull_FalseInitially) {
    Inventory inv(3);
    EXPECT_FALSE(inv.is_full());
}

TEST(InventoryTest, IsFull_TrueWhenAllOccupied) {
    Inventory inv(2);
    inv.place(ItemType::SWORD, "Espada");
    inv.place(ItemType::AXE, "Hacha");
    EXPECT_TRUE(inv.is_full());
}

TEST(InventoryTest, Place_AutoAssignsSlotAndReturnsTrue) {
    Inventory inv(3);
    EXPECT_TRUE(inv.place(ItemType::SWORD, "Espada"));
    EXPECT_EQ(inv.occupied_slots(), 1);
}

TEST(InventoryTest, Place_ReturnsFalseWhenFull) {
    Inventory inv(1);
    inv.place(ItemType::SWORD, "Espada");
    EXPECT_FALSE(inv.place(ItemType::AXE, "Hacha"));
}

TEST(InventoryTest, Place_FillsFirstFreeSlot) {
    Inventory inv(3);
    inv.place_at(2, ItemType::SWORD, "Espada");
    inv.place(ItemType::AXE, "Hacha");
    // AXE should go to slot 0 (first free)
    InventorySlot slot = inv.at(0);
    EXPECT_EQ(slot.item_type, ItemType::AXE);
    EXPECT_EQ(slot.item_name, "Hacha");
}

TEST(InventoryTest, PlaceAt_SpecificSlot) {
    Inventory inv(4);
    EXPECT_TRUE(inv.place_at(2, ItemType::SWORD, "Espada"));
    InventorySlot slot = inv.at(2);
    EXPECT_EQ(slot.item_type, ItemType::SWORD);
    EXPECT_EQ(slot.item_name, "Espada");
    EXPECT_EQ(slot.slot_index, 2);
    EXPECT_EQ(inv.occupied_slots(), 1);
}

TEST(InventoryTest, PlaceAt_OutOfBoundsReturnsFalse) {
    Inventory inv(3);
    EXPECT_FALSE(inv.place_at(99, ItemType::SWORD, "Espada"));
}

TEST(InventoryTest, PlaceAt_OverwritesOccupiedSlot) {
    Inventory inv(3);
    inv.place_at(1, ItemType::SWORD, "Espada");
    inv.place_at(1, ItemType::AXE, "Hacha");
    InventorySlot slot = inv.at(1);
    EXPECT_EQ(slot.item_type, ItemType::AXE);
    EXPECT_EQ(inv.occupied_slots(), 1);
}

TEST(InventoryTest, Clear_RemovesItem) {
    Inventory inv(3);
    inv.place(ItemType::SWORD, "Espada");
    inv.clear(0);
    EXPECT_TRUE(inv.is_empty_at(0));
    EXPECT_EQ(inv.occupied_slots(), 0);
}

TEST(InventoryTest, Clear_OutOfBoundsDoesNothing) {
    Inventory inv(3);
    inv.place(ItemType::SWORD, "Espada");
    inv.clear(99);
    EXPECT_EQ(inv.occupied_slots(), 1);
}

TEST(InventoryTest, Clear_EmptySlotDoesNothing) {
    Inventory inv(3);
    inv.place_at(1, ItemType::SWORD, "Espada");
    inv.clear(0);
    EXPECT_EQ(inv.occupied_slots(), 1);
}

TEST(InventoryTest, Swap_ExchangesItems) {
    Inventory inv(3);
    inv.place_at(0, ItemType::SWORD, "Espada");
    inv.place_at(2, ItemType::AXE, "Hacha");
    inv.swap(0, 2);

    EXPECT_EQ(inv.at(0).item_type, ItemType::AXE);
    EXPECT_EQ(inv.at(0).item_name, "Hacha");
    EXPECT_EQ(inv.at(2).item_type, ItemType::SWORD);
    EXPECT_EQ(inv.at(2).item_name, "Espada");
}

TEST(InventoryTest, Swap_WithEmptySlot) {
    Inventory inv(3);
    inv.place_at(0, ItemType::SWORD, "Espada");
    inv.swap(0, 1);

    EXPECT_TRUE(inv.is_empty_at(0));
    EXPECT_EQ(inv.at(1).item_type, ItemType::SWORD);
}

TEST(InventoryTest, Swap_OutOfBoundsDoesNothing) {
    Inventory inv(3);
    inv.place_at(0, ItemType::SWORD, "Espada");
    inv.swap(0, 99);
    EXPECT_EQ(inv.at(0).item_type, ItemType::SWORD);
}

TEST(InventoryTest, Swap_UpdatesSlotIndices) {
    Inventory inv(3);
    inv.place_at(1, ItemType::SWORD, "Espada");
    inv.place_at(2, ItemType::AXE, "Hacha");
    inv.swap(1, 2);
    EXPECT_EQ(inv.at(1).slot_index, 1);
    EXPECT_EQ(inv.at(2).slot_index, 2);
}

TEST(InventoryTest, FirstFreeSlot_ReturnsCorrectIndex) {
    Inventory inv(4);
    inv.place_at(1, ItemType::SWORD, "Espada");
    EXPECT_EQ(inv.first_free_slot(), 0);

    inv.place_at(0, ItemType::AXE, "Hacha");
    EXPECT_EQ(inv.first_free_slot(), 2);
}

TEST(InventoryTest, FirstFreeSlot_WhenFullReturnsCapacity) {
    Inventory inv(1);
    inv.place(ItemType::SWORD, "Espada");
    EXPECT_EQ(inv.first_free_slot(), 1);
}

TEST(InventoryTest, At_ReturnsSlotData) {
    Inventory inv(3);
    inv.place_at(1, ItemType::SWORD, "Espada");
    InventorySlot slot = inv.at(1);
    EXPECT_EQ(slot.slot_index, 1);
    EXPECT_EQ(slot.item_type, ItemType::SWORD);
    EXPECT_EQ(slot.item_name, "Espada");
}

TEST(InventoryTest, At_OutOfBoundsReturnsDefault) {
    Inventory inv(3);
    InventorySlot slot = inv.at(99);
    EXPECT_EQ(slot.item_type, ItemType::NONE);
    EXPECT_TRUE(slot.item_name.empty());
}

TEST(InventoryTest, DumpSlots_ReturnsAllSlots) {
    Inventory inv(2);
    inv.place_at(0, ItemType::SWORD, "Espada");
    auto slots = inv.dump_slots();
    ASSERT_EQ(slots.size(), 2);
    EXPECT_EQ(slots[0].item_type, ItemType::SWORD);
    EXPECT_EQ(slots[1].item_type, ItemType::NONE);
}

TEST(InventoryTest, LoadSlots_RestoresState) {
    Inventory inv(3);
    inv.place_at(0, ItemType::SWORD, "Espada");
    inv.place_at(2, ItemType::AXE, "Hacha");

    std::vector<InventorySlot> dumped = inv.dump_slots();

    Inventory inv2(3);
    inv2.load_slots(dumped);

    EXPECT_EQ(inv2.occupied_slots(), 2);
    EXPECT_EQ(inv2.at(0).item_type, ItemType::SWORD);
    EXPECT_EQ(inv2.at(0).item_name, "Espada");
    EXPECT_TRUE(inv2.is_empty_at(1));
    EXPECT_EQ(inv2.at(2).item_type, ItemType::AXE);
}

TEST(InventoryTest, LoadSlots_ShorterInput) {
    Inventory inv(3);
    std::vector<InventorySlot> partial;
    partial.push_back({0, ItemType::SWORD, "Espada"});
    inv.load_slots(partial);
    EXPECT_EQ(inv.occupied_slots(), 1);
    EXPECT_EQ(inv.at(0).item_type, ItemType::SWORD);
    EXPECT_TRUE(inv.is_empty_at(1));
    EXPECT_TRUE(inv.is_empty_at(2));
}

TEST(InventoryTest, ToRecords_SerializesCorrectly) {
    Inventory inv(2);
    inv.place_at(0, ItemType::SWORD, "Espada");
    auto records = inv.to_records();
    ASSERT_EQ(records.size(), 2);
    EXPECT_EQ(records[0].item_type, static_cast<uint8_t>(ItemType::SWORD));
    EXPECT_EQ(std::string(records[0].item_name), "Espada");
    EXPECT_EQ(records[1].item_type, static_cast<uint8_t>(ItemType::NONE));
}

TEST(InventoryTest, FromRecords_DeserializesCorrectly) {
    Inventory inv(3);
    std::vector<InventorySlotRecord> records(3);
    records[0].item_type = static_cast<uint8_t>(ItemType::SWORD);
    std::strncpy(records[0].item_name, "Espada", InventorySlotRecord::ITEM_NAME_MAX - 1);
    records[1].item_type = static_cast<uint8_t>(ItemType::AXE);
    std::strncpy(records[1].item_name, "Hacha", InventorySlotRecord::ITEM_NAME_MAX - 1);

    inv.from_records(records);
    EXPECT_EQ(inv.occupied_slots(), 2);
    EXPECT_EQ(inv.at(0).item_type, ItemType::SWORD);
    EXPECT_EQ(inv.at(0).item_name, "Espada");
    EXPECT_EQ(inv.at(1).item_type, ItemType::AXE);
    EXPECT_TRUE(inv.is_empty_at(2));
}

TEST(InventoryTest, FromRecords_LongNameTruncated) {
    Inventory inv(1);
    std::string long_name(InventorySlotRecord::ITEM_NAME_MAX * 2, 'X');
    std::vector<InventorySlotRecord> records(1);
    std::strncpy(records[0].item_name, long_name.c_str(), InventorySlotRecord::ITEM_NAME_MAX - 1);
    records[0].item_name[InventorySlotRecord::ITEM_NAME_MAX - 1] = '\0';

    inv.from_records(records);
    EXPECT_EQ(inv.at(0).item_name.size(), std::strlen(records[0].item_name));
}

TEST(InventoryTest, RoundTrip_RecordsPreservesData) {
    Inventory inv(2);
    inv.place_at(0, ItemType::SWORD, "Espada");
    inv.place_at(1, ItemType::AXE, "Hacha");

    auto records = inv.to_records();
    Inventory inv2(2);
    inv2.from_records(records);

    EXPECT_EQ(inv2.occupied_slots(), 2);
    EXPECT_EQ(inv2.at(0).item_type, ItemType::SWORD);
    EXPECT_EQ(inv2.at(1).item_type, ItemType::AXE);
}
