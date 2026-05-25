#ifndef ITEM_H
#define ITEM_H

#include <cstdint>
#include <string>

#include "messages.h"

class Item {
private:
    std::string name;
    ItemType type;
    uint8_t min_damage;
    uint8_t max_damage;
    uint8_t mana_consumed;
    uint8_t min_defense;
    uint8_t max_defense;

public:
    Item(const std::string& name, ItemType item, uint8_t min_damage, uint8_t max_damage,
         uint8_t mana_consumed, uint8_t min_defense, uint8_t max_defense);
    std::string get_name() const;
    ItemType get_type() const;
    uint8_t get_min_damage() const;
    uint8_t get_max_damage() const;
    uint8_t get_mana_consumed() const;
    uint8_t get_min_defense() const;
    uint8_t get_max_defense() const;
};

#endif
