#include "inventory.h"

#include <algorithm>
#include <cstring>
#include <utility>

Inventory::Inventory(uint8_t capacity): capacity(capacity) {
    slots.resize(capacity);
    for (uint8_t i = 0; i < capacity; ++i) {
        slots[i].slot_index = i;
        slots[i].item_type = ItemType::NONE;
    }
}

uint8_t Inventory::find_free_slot() const {
    for (uint8_t i = 0; i < capacity; ++i) {
        if (slots[i].item_type == ItemType::NONE) {
            return i;
        }
    }
    return capacity;
}

uint8_t Inventory::slot_count() const { return capacity; }

uint8_t Inventory::occupied_slots() const { return occupied; }

bool Inventory::is_full() const { return occupied == capacity; }

bool Inventory::is_empty_at(uint8_t index) const {
    if (index >= capacity)
        return true;
    return slots[index].item_type == ItemType::NONE;
}

InventorySlot Inventory::at(uint8_t index) const {
    if (index >= capacity)
        return InventorySlot{};
    return slots[index];
}

bool Inventory::place(ItemType type, const std::string& name) {
    uint8_t free_idx = find_free_slot();
    if (free_idx == capacity)
        return false;
    return place_at(free_idx, type, name);
}

bool Inventory::place_at(uint8_t index, ItemType type, const std::string& name) {
    if (index >= capacity)
        return false;
    if (slots[index].item_type == ItemType::NONE)
        ++occupied;
    slots[index].item_type = type;
    slots[index].item_name = name;
    return true;
}

void Inventory::clear(uint8_t index) {
    if (index >= capacity)
        return;
    if (slots[index].item_type != ItemType::NONE)
        --occupied;
    slots[index].item_type = ItemType::NONE;
    slots[index].item_name.clear();
}

void Inventory::swap(uint8_t a, uint8_t b) {
    if (a >= capacity || b >= capacity)
        return;
    std::swap(slots[a], slots[b]);
    slots[a].slot_index = a;
    slots[b].slot_index = b;
}

std::vector<InventorySlot> Inventory::dump_slots() const { return slots; }

void Inventory::load_slots(const std::vector<InventorySlot>& new_slots) {
    slots.clear();
    for (size_t i = 0; i < capacity; ++i) {
        if (i < new_slots.size()) {
            InventorySlot slot = new_slots[i];
            slot.slot_index = static_cast<uint8_t>(i);
            slots.push_back(std::move(slot));
        } else {
            slots.push_back(InventorySlot{static_cast<uint8_t>(i), ItemType::NONE, ""});
        }
    }
    occupied = 0;
    for (const auto& slot: slots) {
        if (slot.item_type != ItemType::NONE)
            ++occupied;
    }
}

std::vector<InventorySlotRecord> Inventory::to_records() const {
    std::vector<InventorySlotRecord> records;
    records.reserve(capacity);
    for (const auto& slot: slots) {
        InventorySlotRecord rec;
        std::strncpy(rec.item_name, slot.item_name.c_str(), InventorySlotRecord::ITEM_NAME_MAX - 1);
        rec.item_name[InventorySlotRecord::ITEM_NAME_MAX - 1] = '\0';
        rec.item_type = static_cast<uint8_t>(slot.item_type);
        records.push_back(rec);
    }
    return records;
}

void Inventory::from_records(const std::vector<InventorySlotRecord>& records) {
    slots.clear();
    for (uint8_t i = 0; i < capacity; ++i) {
        if (i < records.size()) {
            InventorySlot slot;
            slot.slot_index = i;
            slot.item_type = static_cast<ItemType>(records[i].item_type);
            slot.item_name = std::string(records[i].item_name, std::strlen(records[i].item_name));
            slots.push_back(std::move(slot));
        } else {
            slots.push_back(InventorySlot{static_cast<uint8_t>(i), ItemType::NONE, ""});
        }
    }
    occupied = 0;
    for (const auto& slot: slots) {
        if (slot.item_type != ItemType::NONE)
            ++occupied;
    }
}

bool Inventory::has_free_slot() const { return occupied < capacity; }

uint8_t Inventory::first_free_slot() const { return find_free_slot(); }
