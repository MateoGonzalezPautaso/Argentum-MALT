#ifndef EQUIPABLE_ITEMS_H
#define EQUIPABLE_ITEMS_H

#include <vector>

#include "item.h"
#include "rng.h"

class EquipableItems {
private:
    std::vector<Item> items;
    Rng& rng;

public:
    explicit EquipableItems(Rng& rng);
    const Item& get_random_item();
};

#endif
