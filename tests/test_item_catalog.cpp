#include "gtest/gtest.h"
#include "common/item_catalog.h"
#include "common/rng.h"

TEST(ItemCatalogTest, ParseItemType_KnownValues) {
    EXPECT_EQ(parse_item_type("SWORD"), ItemType::SWORD);
    EXPECT_EQ(parse_item_type("HEALTH_POTION"), ItemType::HEALTH_POTION);
    EXPECT_EQ(parse_item_type("NONE"), ItemType::NONE);
    EXPECT_EQ(parse_item_type("UNKNOWN"), ItemType::NONE);
}

TEST(ItemCatalogTest, ParseEquipSlot_KnownValues) {
    EXPECT_EQ(parse_equip_slot("WEAPON"), EquipSlot::WEAPON);
    EXPECT_EQ(parse_equip_slot("CONSUMABLE"), EquipSlot::CONSUMABLE);
    EXPECT_EQ(parse_equip_slot("UNKNOWN"), EquipSlot::WEAPON);
}

TEST(ItemCatalogTest, AddAndFindItems) {
    ItemCatalog catalog;
    EXPECT_TRUE(catalog.empty());

    Item sword;
    sword.type = ItemType::SWORD;
    sword.name = "Espada";
    sword.equip_slot = EquipSlot::WEAPON;
    catalog.add(sword);

    EXPECT_FALSE(catalog.empty());
    const Item* found = catalog.find(ItemType::SWORD);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, "Espada");

    EXPECT_EQ(catalog.find(ItemType::AXE), nullptr);
}

TEST(ItemCatalogTest, GetThrowsOnNotFound) {
    ItemCatalog catalog;
    EXPECT_THROW({ catalog.get(ItemType::SWORD); }, std::runtime_error);
}

TEST(ItemCatalogTest, RandomEquipableNeverReturnsConsumable) {
    ItemCatalog catalog;
    Item sword;
    sword.type = ItemType::SWORD;
    sword.name = "Espada";
    sword.equip_slot = EquipSlot::WEAPON;
    catalog.add(sword);

    Item potion;
    potion.type = ItemType::HEALTH_POTION;
    potion.name = "Pocion";
    potion.equip_slot = EquipSlot::CONSUMABLE;
    catalog.add(potion);

    Rng rng;
    for (int i = 0; i < 20; ++i) {
        const Item& item = catalog.random_equipable(rng);
        EXPECT_EQ(item.type, ItemType::SWORD);
    }
}

TEST(ItemCatalogTest, All_ReturnsAllItems) {
    ItemCatalog catalog;
    Item sword;
    sword.type = ItemType::SWORD;
    sword.name = "Espada";
    catalog.add(sword);

    Item axe;
    axe.type = ItemType::AXE;
    axe.name = "Hacha";
    catalog.add(axe);

    const auto& items = catalog.all();
    ASSERT_EQ(items.size(), 2);
    EXPECT_EQ(items[0].type, ItemType::SWORD);
    EXPECT_EQ(items[1].type, ItemType::AXE);
}
