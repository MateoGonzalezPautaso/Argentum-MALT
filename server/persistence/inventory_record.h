#ifndef SERVER_INVENTORY_RECORD_H
#define SERVER_INVENTORY_RECORD_H

#include <cstdint>
#include <cstring>

struct InventorySlotRecord {
    static constexpr std::size_t ITEM_NAME_MAX = 32;

    char item_name[ITEM_NAME_MAX] = {};
    uint8_t item_type = 0;
    uint8_t sprite_id = 0;
};

#endif  // SERVER_INVENTORY_RECORD_H
