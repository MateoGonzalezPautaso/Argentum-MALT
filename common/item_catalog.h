#ifndef COMMON_ITEM_CATALOG_H
#define COMMON_ITEM_CATALOG_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "item.h"
#include "messages.h"

class Rng;

class ItemCatalog {
public:
    void load_from_file(const std::string& path);
    void add(const Item& item);

    const Item* find(ItemType type) const;
    const Item* find_by_name(const std::string& name) const;
    const Item& get(ItemType type) const;
    const Item& random_equipable(Rng& rng) const;

    const std::vector<Item>& all() const { return items_; }
    bool empty() const { return items_.empty(); }

private:
    std::vector<Item> items_;
    std::unordered_map<ItemType, size_t> by_index_;
    std::unordered_map<std::string, size_t> by_lower_name_;
    std::vector<size_t> equippable_item_indexes_;
};

ItemType parse_item_type(const std::string& str);
EquipSlot parse_equip_slot(const std::string& str);

#endif  // COMMON_ITEM_CATALOG_H
