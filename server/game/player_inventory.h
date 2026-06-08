#ifndef SERVER_PLAYER_INVENTORY_H
#define SERVER_PLAYER_INVENTORY_H

#include <cstdint>
#include <string>
#include <vector>

#include "../../common/messages.h"
#include "../persistence/inventory_record.h"

#include "inventory.h"

struct PlayerInventory {
    Inventory equip;
    Inventory hp_potions;
    Inventory mana_potions;

    PlayerInventory(uint8_t eq_cap, uint8_t hp_cap, uint8_t mana_cap);

    uint8_t slot_count() const;
    bool is_empty_at(uint8_t index) const;
    const InventorySlot& at(uint8_t index) const;
    void clear(uint8_t index);
    bool place_at(uint8_t index, ItemType type, const std::string& name);
    bool is_full() const;
    uint8_t first_free_slot() const;
    bool place(ItemType type, const std::string& name);

    std::vector<InventorySlot> dump_slots() const;
    void load_slots(const std::vector<InventorySlot>& new_slots);
    std::vector<InventorySlotRecord> to_records() const;
    void from_records(const std::vector<InventorySlotRecord>& records);
};

#endif  // SERVER_PLAYER_INVENTORY_H
