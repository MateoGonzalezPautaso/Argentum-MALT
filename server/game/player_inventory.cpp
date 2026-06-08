#include "player_inventory.h"

PlayerInventory::PlayerInventory(uint8_t eq_cap, uint8_t hp_cap, uint8_t mana_cap):
        equip(eq_cap),
        hp_potions(hp_cap),
        mana_potions(mana_cap),
        equip_capacity(eq_cap),
        hp_capacity(hp_cap),
        mana_capacity(mana_cap) {}

Inventory& PlayerInventory::inv_for(uint8_t& index) {
    return const_cast<Inventory&>(static_cast<const PlayerInventory&>(*this).inv_for(index));
}

const Inventory& PlayerInventory::inv_for(uint8_t& index) const {
    if (index < equip_capacity)
        return equip;
    index -= equip_capacity;
    if (index < hp_capacity)
        return hp_potions;
    index -= hp_capacity;
    return mana_potions;
}

uint8_t PlayerInventory::slot_count() const {
    return equip_capacity + hp_capacity + mana_capacity;
}

bool PlayerInventory::is_empty_at(uint8_t index) const {
    return inv_for(index).is_empty_at(index);
}

const InventorySlot& PlayerInventory::at(uint8_t index) const {
    return inv_for(index).at(index);
}

void PlayerInventory::clear(uint8_t index) {
    inv_for(index).clear(index);
}

bool PlayerInventory::place_at(uint8_t index, ItemType type, const std::string& name) {
    return inv_for(index).place_at(index, type, name);
}

bool PlayerInventory::is_full() const {
    return equip.is_full();
}

uint8_t PlayerInventory::first_free_slot() const {
    return equip.first_free_slot();
}

bool PlayerInventory::place(ItemType type, const std::string& name) {
    if (type == ItemType::HEALTH_POTION)
        return hp_potions.place(type, name);
    if (type == ItemType::MANA_POTION)
        return mana_potions.place(type, name);
    return equip.place(type, name);
}

std::vector<InventorySlot> PlayerInventory::dump_slots() const {
    std::vector<InventorySlot> all = equip.dump_slots();
    std::vector<InventorySlot> hp = hp_potions.dump_slots();
    std::vector<InventorySlot> mana = mana_potions.dump_slots();
    for (auto& s: hp) s.slot_index += equip_capacity;
    for (auto& s: mana) s.slot_index += equip_capacity + hp_capacity;
    all.insert(all.end(), hp.begin(), hp.end());
    all.insert(all.end(), mana.begin(), mana.end());
    return all;
}

void PlayerInventory::load_slots(const std::vector<InventorySlot>& new_slots) {
    std::vector<InventorySlot> eq, hp, mn;
    for (const auto& s: new_slots) {
        if (s.slot_index < equip_capacity)
            eq.push_back(s);
        else if (s.slot_index < equip_capacity + hp_capacity)
            hp.push_back(s);
        else
            mn.push_back(s);
    }
    for (auto& s: hp) s.slot_index -= equip_capacity;
    for (auto& s: mn) s.slot_index -= equip_capacity + hp_capacity;
    equip.load_slots(eq);
    hp_potions.load_slots(hp);
    mana_potions.load_slots(mn);
}

std::vector<InventorySlotRecord> PlayerInventory::to_records() const {
    std::vector<InventorySlotRecord> all = equip.to_records();
    std::vector<InventorySlotRecord> hp = hp_potions.to_records();
    std::vector<InventorySlotRecord> mana = mana_potions.to_records();
    all.insert(all.end(), hp.begin(), hp.end());
    all.insert(all.end(), mana.begin(), mana.end());
    return all;
}

void PlayerInventory::from_records(const std::vector<InventorySlotRecord>& records) {
    const size_t eq_sz = equip_capacity;
    const size_t hp_sz = hp_capacity;
    std::vector<InventorySlotRecord> eq_records, hp_records, mn_records;
    for (size_t i = 0; i < records.size(); ++i) {
        if (i < eq_sz)
            eq_records.push_back(records[i]);
        else if (i < eq_sz + hp_sz)
            hp_records.push_back(records[i]);
        else
            mn_records.push_back(records[i]);
    }
    equip.from_records(eq_records);
    hp_potions.from_records(hp_records);
    mana_potions.from_records(mn_records);
}

void PlayerInventory::clear_all() {
    equip.load_slots({});
    hp_potions.load_slots({});
    mana_potions.load_slots({});
}
