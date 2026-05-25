#include "equipable_items.h"

EquipableItems::EquipableItems(Rng& rng): rng(rng) {
    items.push_back(Item("Sword", ItemType::SWORD, 2, 5, 0, 0, 0));
    items.push_back(Item("Axe", ItemType::AXE, 4, 5, 0, 0, 0));
    items.push_back(Item("Hammer", ItemType::HAMMER, 1, 9, 0, 0, 0));
    items.push_back(Item("Ash staff", ItemType::ASH_STAFF, 2, 4, 5, 0, 0));
    items.push_back(Item("Elven flute", ItemType::ELVEN_FLUTE, 0, 0, 100, 0, 0));
    items.push_back(Item("Knotted staff", ItemType::KNOTTED_STAFF, 4, 8, 15, 0, 0));
    items.push_back(Item("Studded staff", ItemType::STUDDED_STAFF, 8, 20, 30, 0, 0));
    items.push_back(Item("Simple bow", ItemType::SIMPLE_BOW, 1, 4, 0, 0, 0));
    items.push_back(Item("Composite bow", ItemType::COMPOSITE_BOW, 4, 16, 0, 0, 0));
    items.push_back(Item("Leather armor", ItemType::LEATHER_ARMOR, 0, 0, 0, 2, 6));
    items.push_back(Item("Plate armor", ItemType::PLATE_ARMOR, 0, 0, 0, 15, 30));
    items.push_back(Item("Blue tunic", ItemType::BLUE_TUNIC, 0, 0, 0, 6, 10));
    items.push_back(Item("Hood", ItemType::HOOD, 0, 0, 0, 1, 4));
    items.push_back(Item("Iron helmet", ItemType::IRON_HELMET, 0, 0, 0, 4, 8));
    items.push_back(Item("Turtle shield", ItemType::TURTLE_SHIELD, 0, 0, 0, 1, 2));
    items.push_back(Item("Iron shield", ItemType::IRON_SHIELD, 0, 0, 0, 1, 4));
    items.push_back(Item("Magic hat", ItemType::MAGIC_HAT, 0, 0, 0, 4, 12));
}

const Item& EquipableItems::get_random_item() {
    int random_number = rng.get_random_int(0, items.size() - 1);
    return items[random_number];
}
