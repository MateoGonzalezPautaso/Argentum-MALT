#include "player_inventory.h"

PlayerInventory::PlayerInventory(uint8_t eq_cap, uint8_t hp_cap, uint8_t mana_cap):
        equip(eq_cap), hp_potions(hp_cap), mana_potions(mana_cap) {}

uint8_t PlayerInventory::slot_count() const { return equip.slot_count(); }

bool PlayerInventory::is_empty_at(uint8_t index) const { return equip.is_empty_at(index); }

const InventorySlot& PlayerInventory::at(uint8_t index) const { return equip.at(index); }

void PlayerInventory::clear(uint8_t index) { equip.clear(index); }

bool PlayerInventory::place_at(uint8_t index, ItemType type, const std::string& name) {
    return equip.place_at(index, type, name);
}

bool PlayerInventory::is_full() const { return equip.is_full(); }

uint8_t PlayerInventory::first_free_slot() const { return equip.first_free_slot(); }

bool PlayerInventory::place(ItemType type, const std::string& name) {
    return equip.place(type, name);
}

std::vector<InventorySlot> PlayerInventory::dump_slots() const { return equip.dump_slots(); }

void PlayerInventory::load_slots(const std::vector<InventorySlot>& new_slots) {
    equip.load_slots(new_slots);
}

std::vector<InventorySlotRecord> PlayerInventory::to_records() const { return equip.to_records(); }

void PlayerInventory::from_records(const std::vector<InventorySlotRecord>& records) {
    equip.from_records(records);
}
