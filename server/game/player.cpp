#include "player.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

Player::Player(uint16_t id, const std::string& username, Position pos, Direction dir, Race race,
               PlayerClass player_class, const BalanceConfig& balance, uint8_t inv_capacity):
        id(id),
        dir(dir),
        race(race),
        player_class(player_class),
        experience(0),
        mana_current(0),
        mana_max(0),
        gold(balance.starting_gold),
        balance(balance),
        inventory(inv_capacity),
        Entity(0, username, pos, 1) {
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
               uint32_t mana_max, uint32_t gold, uint8_t inv_capacity, uint32_t strength,
               uint32_t agility):
        id(id),
        dir(dir),
        race(race),
        player_class(player_class),
        experience(experience),
        mana_current(mana_current),
        mana_max(mana_max),
        gold(gold),
        balance(balance),
        inventory(inv_capacity),
        strength(strength),
        agility(agility),
        Entity(hp_max, username, pos, level) {
    for (uint8_t i = 0; i < EQUIP_SLOT_COUNT; ++i) {
        equipped[i] = InventorySlot{};
        equipped[i].item_type = ItemType::NONE;
    }
}

void Player::apply_move(Direction new_dir, int dx, int dy) {
    dir = new_dir;
    Position pos = get_pos();
    uint16_t pos_x = static_cast<uint16_t>(static_cast<int>(pos.x) + dx);
    uint16_t pos_y = static_cast<uint16_t>(static_cast<int>(pos.y) + dy);
    set_pos(pos_x, pos_y);
}

void Player::gain_experience(uint32_t exp) {
    experience += exp;
    uint32_t threshold = static_cast<uint32_t>(balance.level_exp_base *
                                               std::pow(get_level(), balance.level_exp_exponent));
    while (experience >= threshold && get_level() < balance.max_level) {
        experience -= threshold;
        level_up();
        threshold = static_cast<uint32_t>(balance.level_exp_base *
                                          std::pow(get_level(), balance.level_exp_exponent));
    }
}

void Player::level_up() {
    set_level(get_level() + 1);
    UpdateStats stats_updated = update_stats();
    set_hp_max(stats_updated.hp_max);
    mana_max = stats_updated.mana_max;
    strength = stats_updated.strength;
    agility = stats_updated.agility;
    gold += balance.gold_per_level * get_level();
    set_hp_current(stats_updated.hp_max);
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
    uint64_t max_gold = static_cast<uint64_t>(balance.gold_cap_base *
                                              std::pow(get_level(), balance.gold_cap_exponent));
    // Players can hold up to 150% of OroMax; the excess 50% is "at risk" on death
    uint64_t hard_cap = max_gold + max_gold / 2;
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
    int constitution = 0;
    double f_hp_race = 0.0;
    int intelligence = 0;
    double f_mana_race = 0.0;
    double f_strength_race = 0.0;
    double f_agility_race = 0.0;
    switch (race) {
        case Race::HUMAN:
            constitution = balance.hp.constitution_human;
            f_hp_race = balance.hp.race_hp_factor_human;
            intelligence = balance.mana.intelligence_human;
            f_mana_race = balance.mana.race_mana_factor_human;
            f_strength_race = balance.strength.race_strength_factor_human;
            f_agility_race = balance.agility.race_agility_factor_human;
            break;
        case Race::ELF:
            constitution = balance.hp.constitution_elf;
            f_hp_race = balance.hp.race_hp_factor_elf;
            intelligence = balance.mana.intelligence_elf;
            f_mana_race = balance.mana.race_mana_factor_elf;
            f_strength_race = balance.strength.race_strength_factor_elf;
            f_agility_race = balance.agility.race_agility_factor_elf;
            break;
        case Race::DWARF:
            constitution = balance.hp.constitution_dwarf;
            f_hp_race = balance.hp.race_hp_factor_dwarf;
            intelligence = balance.mana.intelligence_dwarf;
            f_mana_race = balance.mana.race_mana_factor_dwarf;
            f_strength_race = balance.strength.race_strength_factor_dwarf;
            f_agility_race = balance.agility.race_agility_factor_dwarf;
            break;
        case Race::GNOME:
            constitution = balance.hp.constitution_gnome;
            f_hp_race = balance.hp.race_hp_factor_gnome;
            intelligence = balance.mana.intelligence_gnome;
            f_mana_race = balance.mana.race_mana_factor_gnome;
            f_strength_race = balance.strength.race_strength_factor_gnome;
            f_agility_race = balance.agility.race_agility_factor_gnome;
            break;
    }

    double f_hp_class = 0.0;
    double f_mana_class = 0.0;
    double f_strength_class = 0.0;
    double f_agility_class = 0.0;
    switch (player_class) {
        case PlayerClass::WARRIOR:
            f_hp_class = balance.hp.class_hp_factor_warrior;
            f_mana_class = balance.mana.class_mana_factor_warrior;
            f_strength_class = balance.strength.class_strength_factor_warrior;
            f_agility_class = balance.agility.class_agility_factor_warrior;
            break;
        case PlayerClass::PALADIN:
            f_hp_class = balance.hp.class_hp_factor_paladin;
            f_mana_class = balance.mana.class_mana_factor_paladin;
            f_strength_class = balance.strength.class_strength_factor_paladin;
            f_agility_class = balance.agility.class_agility_factor_paladin;
            break;
        case PlayerClass::CLERIC:
            f_hp_class = balance.hp.class_hp_factor_cleric;
            f_mana_class = balance.mana.class_mana_factor_cleric;
            f_strength_class = balance.strength.class_strength_factor_cleric;
            f_agility_class = balance.agility.class_agility_factor_cleric;
            break;
        case PlayerClass::MAGE:
            f_hp_class = balance.hp.class_hp_factor_mage;
            f_mana_class = balance.mana.class_mana_factor_mage;
            f_strength_class = balance.strength.class_strength_factor_mage;
            f_agility_class = balance.agility.class_agility_factor_mage;
            break;
    }

    UpdateStats stats_updated;
    stats_updated.hp_max =
            static_cast<uint32_t>(constitution * f_hp_race * f_hp_class * get_level());
    stats_updated.mana_max =
            static_cast<uint32_t>(intelligence * f_mana_race * f_mana_class * get_level());
    stats_updated.strength =
            static_cast<uint32_t>(f_strength_race * f_strength_class * get_level());
    stats_updated.agility = static_cast<uint32_t>(f_agility_race * f_agility_class * get_level());
    return stats_updated;
}

uint32_t Player::exp_to_next_level() const {
    return static_cast<uint32_t>(balance.level_exp_base *
                                 std::pow(get_level(), balance.level_exp_exponent));
}

bool Player::equip(uint8_t inv_slot_index, const ItemCatalog& catalog) {
    if (inv_slot_index >= inventory.slot_count())
        return false;
    if (inventory.is_empty_at(inv_slot_index))
        return false;

    InventorySlot slot = inventory.at(inv_slot_index);
    const Item* def = catalog.find(slot.item_type);
    if (!def)
        return false;

    if (def->equip_slot == EquipSlot::CONSUMABLE) {
        switch (def->type) {
            case ItemType::HEALTH_POTION:
                heal(get_hp_max());
                break;
            case ItemType::MANA_POTION:
                mana_current = mana_max;
                break;
            default:
                return false;
        }
        inventory.clear(inv_slot_index);
        return true;
    }

    uint8_t eslot = static_cast<uint8_t>(def->equip_slot);

    if (equipped[eslot].item_type == ItemType::NONE) {
        equipped[eslot] = slot;
        inventory.clear(inv_slot_index);
        equipped[eslot].slot_index = eslot;
    } else {
        InventorySlot replaced = equipped[eslot];
        equipped[eslot] = slot;
        equipped[eslot].slot_index = eslot;
        inventory.place_at(inv_slot_index, replaced.item_type, replaced.item_name);
    }
    return true;
}

void Player::unequip(EquipSlot eslot) {
    uint8_t index = static_cast<uint8_t>(eslot);
    if (index >= EQUIP_SLOT_COUNT)
        return;
    if (equipped[index].item_type == ItemType::NONE)
        return;
    if (inventory.is_full())
        return;

    uint8_t free_slot = inventory.first_free_slot();
    inventory.place_at(free_slot, equipped[index].item_type, equipped[index].item_name);
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

std::vector<InventorySlot> Player::dump_inventory() const { return inventory.dump_slots(); }

void Player::load_inventory(const std::vector<InventorySlotRecord>& records) {
    inventory.from_records(records);
}

bool Player::add_item(ItemType type, const std::string& name) {
    return inventory.place(type, name);
}

std::vector<InventorySlotRecord> Player::dump_inventory_records() const {
    return inventory.to_records();
}
