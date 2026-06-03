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

    const Item* find(ItemType type) const;
    const Item& get(ItemType type) const;
    const Item& random_equipable(Rng& rng) const;

    const std::vector<Item>& all() const { return items_; }
    bool empty() const { return items_.empty(); }

private:
    std::vector<Item> items_;
    std::unordered_map<ItemType, size_t> by_index_;
};

ItemType parse_item_type(const std::string& str);
EquipSlot parse_equip_slot(const std::string& str);

#endif  // COMMON_ITEM_CATALOG_H
