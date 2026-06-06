#ifndef ITEM_H
#define ITEM_H

#include <cstdint>
#include <string>

#include "messages.h"

struct Item {
    ItemType type = ItemType::NONE;
    std::string name;
    EquipSlot equip_slot = EquipSlot::WEAPON;
    uint8_t min_damage = 0;
    uint8_t max_damage = 0;
    uint8_t mana_consumed = 0;
    uint8_t min_defense = 0;
    uint8_t max_defense = 0;
    uint16_t attack_range = 0;
};

#endif
