#ifndef SERVER_INVENTORY_H
#define SERVER_INVENTORY_H

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "../../common/messages.h"
#include "../persistence/inventory_record.h"

class Inventory {
private:
    std::vector<InventorySlot> slots;
    uint8_t capacity;
    uint8_t occupied = 0;

    std::optional<uint8_t> find_free_slot() const;
    void recount_occupied();

public:
    explicit Inventory(uint8_t capacity);

    uint8_t slot_count() const;
    uint8_t occupied_slots() const;
    bool is_full() const;
    bool is_empty_at(uint8_t index) const;
    bool has_free_slot() const;
    uint8_t first_free_slot() const;
    const InventorySlot& at(uint8_t index) const;

    bool place(ItemType type, const std::string& name);
    bool place_at(uint8_t index, ItemType type, const std::string& name);
    void clear(uint8_t index);
    void swap(uint8_t a, uint8_t b);

    std::vector<InventorySlot> dump_slots() const;
    void load_slots(const std::vector<InventorySlot>& new_slots);

    std::vector<InventorySlotRecord> to_records() const;
    void from_records(const std::vector<InventorySlotRecord>& records);
};

#endif  // SERVER_INVENTORY_H
