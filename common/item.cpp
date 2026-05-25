#include "item.h"

Item::Item(const std::string& name, ItemType type, uint8_t min_damage, uint8_t max_damage,
           uint8_t mana_consumed, uint8_t min_defense, uint8_t max_defense) {
    this->name = name;
    this->type = type;
    this->min_damage = min_damage;
    this->max_damage = max_damage;
    this->mana_consumed = mana_consumed;
    this->min_defense = min_defense;
    this->max_defense = max_defense;
}

std::string Item::get_name() const { return name; }

ItemType Item::get_type() const { return type; }

uint8_t Item::get_min_damage() const { return min_damage; }

uint8_t Item::get_max_damage() const { return max_damage; }

uint8_t Item::get_mana_consumed() const { return mana_consumed; }

uint8_t Item::get_min_defense() const { return min_defense; }

uint8_t Item::get_max_defense() const { return max_defense; }
