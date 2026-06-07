#include "item_catalog.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

#include <toml++/toml.h>

#include "config.h"
#include "rng.h"

ItemType parse_item_type(const std::string& str) {
    if (str == "SWORD")
        return ItemType::SWORD;
    if (str == "AXE")
        return ItemType::AXE;
    if (str == "HAMMER")
        return ItemType::HAMMER;
    if (str == "ASH_STAFF")
        return ItemType::ASH_STAFF;
    if (str == "ELVEN_FLUTE")
        return ItemType::ELVEN_FLUTE;
    if (str == "KNOTTED_STAFF")
        return ItemType::KNOTTED_STAFF;
    if (str == "STUDDED_STAFF")
        return ItemType::STUDDED_STAFF;
    if (str == "SIMPLE_BOW")
        return ItemType::SIMPLE_BOW;
    if (str == "COMPOSITE_BOW")
        return ItemType::COMPOSITE_BOW;
    if (str == "LEATHER_ARMOR")
        return ItemType::LEATHER_ARMOR;
    if (str == "PLATE_ARMOR")
        return ItemType::PLATE_ARMOR;
    if (str == "BLUE_TUNIC")
        return ItemType::BLUE_TUNIC;
    if (str == "HOOD")
        return ItemType::HOOD;
    if (str == "IRON_HELMET")
        return ItemType::IRON_HELMET;
    if (str == "TURTLE_SHIELD")
        return ItemType::TURTLE_SHIELD;
    if (str == "IRON_SHIELD")
        return ItemType::IRON_SHIELD;
    if (str == "MAGIC_HAT")
        return ItemType::MAGIC_HAT;
    if (str == "HEALTH_POTION")
        return ItemType::HEALTH_POTION;
    if (str == "MANA_POTION")
        return ItemType::MANA_POTION;
    if (str == "GOLD_DROP")
        return ItemType::GOLD_DROP;
    return ItemType::NONE;
}

EquipSlot parse_equip_slot(const std::string& str) {
    if (str == "WEAPON")
        return EquipSlot::WEAPON;
    if (str == "ARMOR")
        return EquipSlot::ARMOR;
    if (str == "HELMET")
        return EquipSlot::HELMET;
    if (str == "SHIELD")
        return EquipSlot::SHIELD;
    if (str == "CONSUMABLE")
        return EquipSlot::CONSUMABLE;
    return EquipSlot::WEAPON;
}

void ItemCatalog::load_from_file(const std::string& path) {
    auto root = toml::parse_file(path);
    items_.clear();
    by_index_.clear();

    auto arr = root["item"].as_array();
    if (!arr) {
        throw std::runtime_error("items.toml: missing [[item]] array");
    }

    for (const auto& node: *arr) {
        const auto* tbl = node.as_table();
        if (!tbl)
            continue;

        Item item;
        item.type = parse_item_type(toml_get_string(*tbl, "item_type", ""));
        if (item.type == ItemType::NONE)
            continue;

        item.name = toml_get_string(*tbl, "name", "");
        item.equip_slot = parse_equip_slot(toml_get_string(*tbl, "equip_slot", "WEAPON"));
        item.min_damage = static_cast<uint8_t>(toml_get_int(*tbl, "min_damage", 0));
        item.max_damage = static_cast<uint8_t>(toml_get_int(*tbl, "max_damage", 0));
        item.mana_consumed = static_cast<uint8_t>(toml_get_int(*tbl, "mana_cost", 0));
        item.min_defense = static_cast<uint8_t>(toml_get_int(*tbl, "min_defense", 0));
        item.max_defense = static_cast<uint8_t>(toml_get_int(*tbl, "max_defense", 0));
        item.spell_effect_id = static_cast<uint8_t>(toml_get_int(*tbl, "spell_effect_id", 0));
        item.price = static_cast<uint32_t>(toml_get_int(*tbl, "price", 0));
        item.attack_range = static_cast<uint16_t>(toml_get_int(*tbl, "attack_range", 0));

        items_.push_back(item);
    }

    for (size_t i = 0; i < items_.size(); ++i) {
        by_index_[items_[i].type] = i;
    }
}

void ItemCatalog::add(const Item& item) {
    by_index_[item.type] = items_.size();
    items_.push_back(item);
}

static std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

const Item* ItemCatalog::find_by_name(const std::string& name) const {
    const std::string lower_name = to_lower(name);
    auto it = std::find_if(items_.begin(), items_.end(),
                           [&](const Item& item) { return to_lower(item.name) == lower_name; });
    if (it != items_.end())
        return &*it;
    return nullptr;
}

const Item* ItemCatalog::find(ItemType type) const {
    auto it = by_index_.find(type);
    if (it == by_index_.end())
        return nullptr;
    return &items_[it->second];
}

const Item& ItemCatalog::get(ItemType type) const {
    const Item* it = find(type);
    if (!it) {
        throw std::runtime_error("Item not found in catalog");
    }
    return *it;
}

const Item& ItemCatalog::random_equipable(Rng& rng) const {
    std::vector<size_t> indices;
    for (size_t i = 0; i < items_.size(); ++i) {
        if (items_[i].equip_slot != EquipSlot::CONSUMABLE && items_[i].type != ItemType::NONE)
            indices.push_back(i);
    }
    if (indices.empty()) {
        throw std::runtime_error("No equipable items in catalog");
    }
    int idx = rng.get_random_int(0, static_cast<int>(indices.size()) - 1);
    return items_[indices[idx]];
}
