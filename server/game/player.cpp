#include "player.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#include "game_formulas.h"

Player::Player(uint16_t id, const std::string& username, Position pos, Direction dir, Race race,
               PlayerClass player_class, const BalanceConfig& balance, uint8_t equip_capacity,
               uint8_t hp_potion_capacity, uint8_t mana_potion_capacity, uint8_t bank_capacity):
        Entity(0, username, pos, 1),
        id(id),
        race(race),
        player_class(player_class),
        experience(0),
        mana_current(0),
        mana_max(0),
        gold(balance.starting_gold),
        balance(balance),
        inv(equip_capacity, hp_potion_capacity, mana_potion_capacity),
        bank_inv(bank_capacity),
        bank_gold(0) {
    set_dir(dir);
    UpdateStats stats = update_stats();
    set_hp_max(stats.hp_max);
    set_hp_current(stats.hp_max);
    mana_max = stats.mana_max;
    mana_current = mana_max;
    strength = stats.strength;
    agility = stats.agility;
    for (uint8_t i = 0; i < EQUIP_SLOT_COUNT; ++i) {
        equipped[i] = InventorySlot{};
        equipped[i].item_type = ItemType::NONE;
    }
}

Player::Player(uint16_t id, const std::string& username, Position pos, Direction dir, Race race,
               PlayerClass player_class, const BalanceConfig& balance, uint8_t level,
               uint32_t experience, uint32_t hp_current, uint32_t hp_max, uint32_t mana_current,
               uint32_t mana_max, uint32_t gold, uint8_t equip_capacity,
               uint8_t hp_potion_capacity, uint8_t mana_potion_capacity, uint8_t bank_capacity,
               uint32_t bank_gold):
        Entity(hp_max, username, pos, level),
        id(id),
        race(race),
        player_class(player_class),
        experience(experience),
        mana_current(mana_current),
        mana_max(mana_max),
        gold(gold),
        balance(balance),
        inv(equip_capacity, hp_potion_capacity, mana_potion_capacity),
        bank_inv(bank_capacity),
        bank_gold(bank_gold) {
    set_dir(dir);
    set_hp_current(hp_current);
    UpdateStats stats = update_stats();
    strength = stats.strength;
    agility = stats.agility;
    for (uint8_t i = 0; i < EQUIP_SLOT_COUNT; ++i) {
        equipped[i] = InventorySlot{};
        equipped[i].item_type = ItemType::NONE;
    }
}

void Player::apply_move(Direction new_dir, int dx, int dy) {
    set_dir(new_dir);
    Position pos = get_pos();
    uint16_t pos_x = static_cast<uint16_t>(static_cast<int>(pos.x) + dx);
    uint16_t pos_y = static_cast<uint16_t>(static_cast<int>(pos.y) + dy);
    set_pos(pos_x, pos_y);
}

void Player::lose_experience_on_death() {
    experience = GameFormulas::experience_loss_on_death(balance, experience);
}

void Player::gain_experience(uint32_t exp) {
    experience += exp;
    uint32_t threshold = GameFormulas::exp_threshold(balance, get_level());
    while (experience >= threshold && get_level() < balance.max_level) {
        experience -= threshold;
        level_up();
        threshold = GameFormulas::exp_threshold(balance, get_level());
    }
}

void Player::level_up() {
    set_level(get_level() + 1);
    UpdateStats stats_updated = update_stats();
    set_hp_max(stats_updated.hp_max);
    mana_max = stats_updated.mana_max;
    strength = stats_updated.strength;
    agility = stats_updated.agility;
    gain_gold(GameFormulas::level_up_gold(balance.gold_per_level, get_level()));
    set_hp_current(get_hp_max());
    mana_current = mana_max;
}

void Player::take_damage(uint32_t damage) {
    if (cheat_infinite_hp)
        return;
    Entity::take_damage(damage);
}

void Player::resurrect() {
    set_hp_current(get_hp_max());
    mana_current = mana_max;
}

void Player::heal(uint32_t amount) {
    uint64_t total = static_cast<uint64_t>(get_hp_current()) + amount;
    set_hp_current(static_cast<uint32_t>(std::min<uint64_t>(total, get_hp_max())));
}

void Player::gain_gold(uint32_t amount) {
    uint64_t max_gold = GameFormulas::gold_cap(balance, get_level());
    // Players can hold up to (1 + gold_excess_ratio) * OroMax; the excess is "at risk" on death
    uint64_t hard_cap = max_gold + static_cast<uint64_t>(max_gold * balance.gold_excess_ratio);
    uint64_t total = static_cast<uint64_t>(gold) + amount;
    gold = static_cast<uint32_t>(std::min<uint64_t>(total, hard_cap));
}

void Player::increase_max_hp(uint32_t amount) {
    set_hp_max(get_hp_max() + amount);
    set_hp_current(get_hp_current() + amount);
}

void Player::increase_max_mana(uint32_t amount) {
    mana_max += amount;
    mana_current += amount;
}

// Should check these implementations

void Player::restore_mana(uint32_t amount) {
    uint64_t total = static_cast<uint64_t>(mana_current) + amount;
    mana_current = static_cast<uint32_t>(std::min<uint64_t>(total, mana_max));
}

void Player::use_mana(uint32_t amount) {
    if (cheat_infinite_mana)
        return;
    if (amount >= mana_current) {
        mana_current = 0;
    } else {
        mana_current -= amount;
    }
}

void Player::spend_gold(uint32_t amount) {
    if (amount >= gold) {
        gold = 0;
    } else {
        gold -= amount;
    }
}

uint32_t Player::take_excess_gold() {
    uint32_t oro_max = GameFormulas::gold_cap(balance, get_level());
    if (gold <= oro_max)
        return 0;
    uint32_t excess = gold - oro_max;
    gold = oro_max;
    return excess;
}

void Player::level_down() {
    if (get_level() <= 1)
        return;
    set_level(get_level() - 1);
    UpdateStats stats_updated = update_stats();
    set_hp_max(stats_updated.hp_max);
    mana_max = stats_updated.mana_max;
    strength = stats_updated.strength;
    agility = stats_updated.agility;
    set_hp_current(stats_updated.hp_max);
    mana_current = mana_max;
}

UpdateStats Player::update_stats() const {
    UpdateStats stats_updated;
    stats_updated.hp_max = GameFormulas::max_hp(balance, race, player_class, get_level());
    stats_updated.mana_max = GameFormulas::max_mana(balance, race, player_class, get_level());
    stats_updated.strength = GameFormulas::strength(balance, race, player_class, get_level());
    stats_updated.agility = GameFormulas::agility(balance, race, player_class, get_level());
    return stats_updated;
}

uint32_t Player::exp_to_next_level() const {
    return GameFormulas::exp_threshold(balance, get_level());
}

bool Player::equip(uint8_t inv_slot_index, const ItemCatalog& catalog) {
    if (inv_slot_index >= inv.slot_count())
        return false;
    if (inv.is_empty_at(inv_slot_index))
        return false;

    InventorySlot slot = inv.at(inv_slot_index);
    const Item* def = catalog.find(slot.item_type);
    if (!def)
        return false;

    if (def->equip_slot == EquipSlot::CONSUMABLE) {
        if (def->restore_hp_percent == 0 && def->restore_mana_percent == 0)
            return false;  // consumable with no defined effect
        if (def->restore_hp_percent > 0)
            heal(static_cast<uint32_t>(static_cast<uint64_t>(get_hp_max()) *
                                       def->restore_hp_percent / 100));
        if (def->restore_mana_percent > 0)
            restore_mana(static_cast<uint32_t>(static_cast<uint64_t>(get_mana_max()) *
                                               def->restore_mana_percent / 100));
        inv.clear(inv_slot_index);
        return true;
    }

    uint8_t eslot = static_cast<uint8_t>(def->equip_slot);
    if (eslot >= EQUIP_SLOT_COUNT)
        return false;

    if (equipped[eslot].item_type == ItemType::NONE) {
        equipped[eslot] = slot;
        inv.clear(inv_slot_index);
        equipped[eslot].slot_index = eslot;
    } else {
        InventorySlot replaced = equipped[eslot];
        equipped[eslot] = slot;
        equipped[eslot].slot_index = eslot;
        inv.place_at(inv_slot_index, replaced.item_type, replaced.item_name);
    }
    return true;
}

void Player::unequip(EquipSlot eslot) {
    uint8_t index = static_cast<uint8_t>(eslot);
    if (index >= EQUIP_SLOT_COUNT)
        return;
    if (equipped[index].item_type == ItemType::NONE)
        return;
    if (inv.is_full())
        return;

    uint8_t free_slot = inv.first_free_slot();
    inv.place_at(free_slot, equipped[index].item_type, equipped[index].item_name);
    equipped[index] = InventorySlot{};
    equipped[index].item_type = ItemType::NONE;
}

const InventorySlot& Player::get_equipped(EquipSlot eslot) const {
    return equipped[static_cast<uint8_t>(eslot)];
}

void Player::dump_equipped(InventorySlot out[EQUIP_SLOT_COUNT]) const {
    for (uint8_t i = 0; i < EQUIP_SLOT_COUNT; ++i) out[i] = equipped[i];
}

void Player::restore_equipment(const InventorySlot equipment[EQUIP_SLOT_COUNT]) {
    for (uint8_t i = 0; i < EQUIP_SLOT_COUNT; ++i) {
        equipped[i] = equipment[i];
        equipped[i].slot_index = i;
    }
}

void Player::clear_equipped() {
    for (uint8_t i = 0; i < EQUIP_SLOT_COUNT; ++i) {
        equipped[i] = InventorySlot{};
        equipped[i].item_type = ItemType::NONE;
    }
}

std::vector<InventorySlot> Player::dump_inventory() const { return inv.dump_slots(); }

void Player::clear_inventory() { inv.load_slots({}); }

void Player::load_inventory(const std::vector<InventorySlotRecord>& records) {
    inv.from_records(records);
}

bool Player::add_item(ItemType type, const std::string& name) {
    return inv.place(type, name);
}

void Player::remove_inventory_item(uint8_t slot_index) { inv.clear(slot_index); }

std::vector<InventorySlotRecord> Player::dump_inventory_records() const {
    return inv.to_records();
}

bool Player::take_bank_gold(uint32_t amount) {
    if (amount > bank_gold)
        return false;
    bank_gold -= amount;
    return true;
}

std::vector<InventorySlot> Player::dump_bank() const { return bank_inv.dump_slots(); }

bool Player::add_to_bank(ItemType type, const std::string& name) {
    return bank_inv.place(type, name);
}

void Player::remove_bank_item(uint8_t slot_index) { bank_inv.clear(slot_index); }

std::vector<InventorySlotRecord> Player::dump_bank_records() const { return bank_inv.to_records(); }

void Player::load_bank(const std::vector<InventorySlotRecord>& records) {
    bank_inv.from_records(records);
}
