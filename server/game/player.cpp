#include "player.h"

#include <algorithm>
#include <cmath>

Player::Player(uint16_t id, const std::string& username, Position pos, Direction dir, Race race,
               PlayerClass player_class, const BalanceConfig& balance, uint8_t inv_capacity):
        id(id),
        username(username),
        pos(pos),
        dir(dir),
        race(race),
        player_class(player_class),
        level(1),
        experience(0),
        hp_current(0),
        hp_max(0),
        mana_current(0),
        mana_max(0),
        gold(balance.starting_gold),
        balance(balance),
        inventory(inv_capacity) {
    hp_max = calculate_hp_max();
    hp_current = hp_max;
    mana_max = calculate_mana_max();
    mana_current = mana_max;
    for (int i = 0; i < 4; ++i) {
        equipped[i] = InventorySlot{};
        equipped[i].item_type = ItemType::NONE;
    }
}

Player::Player(uint16_t id, const std::string& username, Position pos, Direction dir, Race race,
               PlayerClass player_class, const BalanceConfig& balance,
               uint8_t level, uint32_t experience,
               uint32_t hp_current, uint32_t hp_max,
               uint32_t mana_current, uint32_t mana_max,
               uint32_t gold, uint8_t inv_capacity):
        id(id),
        username(username),
        pos(pos),
        dir(dir),
        race(race),
        player_class(player_class),
        level(level),
        experience(experience),
        hp_current(hp_current),
        hp_max(hp_max),
        mana_current(mana_current),
        mana_max(mana_max),
        gold(gold),
        balance(balance),
        inventory(inv_capacity) {
    for (int i = 0; i < 4; ++i) {
        equipped[i] = InventorySlot{};
        equipped[i].item_type = ItemType::NONE;
    }
}

bool Player::try_attack(uint32_t current_tick, uint32_t cooldown_ticks) {
    if (current_tick < next_attack_tick)
        return false;
    next_attack_tick = current_tick + cooldown_ticks;
    return true;
}

void Player::apply_move(Direction new_dir, int dx, int dy) {
    dir = new_dir;
    pos.x = static_cast<uint16_t>(static_cast<int>(pos.x) + dx);
    pos.y = static_cast<uint16_t>(static_cast<int>(pos.y) + dy);
}

void Player::gain_experience(uint32_t exp) {
    experience += exp;
    uint32_t threshold = static_cast<uint32_t>(balance.level_exp_base *
                                               std::pow(level, balance.level_exp_exponent));
    while (experience >= threshold && level < balance.max_level) {
        experience -= threshold;
        level_up();
        threshold = static_cast<uint32_t>(balance.level_exp_base *
                                          std::pow(level, balance.level_exp_exponent));
    }
}

void Player::level_up() {
    ++level;
    hp_max = calculate_hp_max();
    mana_max = calculate_mana_max();
    gold += balance.gold_per_level * level;
    hp_current = hp_max;
    mana_current = mana_max;
}

void Player::take_damage(uint32_t damage) {
    if (cheat_infinite_hp)
        return;
    if (damage >= hp_current) {
        hp_current = 0;
        is_dead = true;
    } else {
        hp_current -= damage;
    }
}

bool Player::is_ghost() const { return is_dead; }

void Player::resurrect() {
    is_dead = false;
    hp_current = hp_max;
    mana_current = mana_max;
}

void Player::heal(uint32_t amount) {
    uint64_t total = static_cast<uint64_t>(hp_current) + amount;
    hp_current = static_cast<uint32_t>(std::min<uint64_t>(total, hp_max));
}

void Player::gain_gold(uint32_t amount) {
    uint64_t max_gold = static_cast<uint64_t>(balance.gold_cap_base *
                                              std::pow(level, balance.gold_cap_exponent));
    // Players can hold up to 150% of OroMax; the excess 50% is "at risk" on death
    uint64_t hard_cap = max_gold + max_gold / 2;
    uint64_t total = static_cast<uint64_t>(gold) + amount;
    gold = static_cast<uint32_t>(std::min<uint64_t>(total, hard_cap));
}

void Player::increase_max_hp(uint32_t amount) {
    hp_max += amount;
    hp_current += amount;
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
    if (level <= 1)
        return;
    --level;
    hp_max = calculate_hp_max();
    mana_max = calculate_mana_max();
    hp_current = hp_max;
    mana_current = mana_max;
}

uint32_t Player::calculate_hp_max() const {
    int constitution = balance.hp.constitution_human;
    double f_race = balance.hp.race_hp_factor_human;
    switch (race) {
        case Race::HUMAN: constitution = balance.hp.constitution_human; f_race = balance.hp.race_hp_factor_human; break;
        case Race::ELF:   constitution = balance.hp.constitution_elf;   f_race = balance.hp.race_hp_factor_elf;   break;
        case Race::DWARF: constitution = balance.hp.constitution_dwarf; f_race = balance.hp.race_hp_factor_dwarf; break;
        case Race::GNOME: constitution = balance.hp.constitution_gnome; f_race = balance.hp.race_hp_factor_gnome; break;
    }
    double f_class = balance.hp.class_hp_factor_warrior;
    switch (player_class) {
        case PlayerClass::WARRIOR: f_class = balance.hp.class_hp_factor_warrior; break;
        case PlayerClass::PALADIN: f_class = balance.hp.class_hp_factor_paladin; break;
        case PlayerClass::CLERIC:  f_class = balance.hp.class_hp_factor_cleric;  break;
        case PlayerClass::MAGE:    f_class = balance.hp.class_hp_factor_mage;    break;
    }
    return static_cast<uint32_t>(constitution * f_class * f_race * level);
}

uint32_t Player::calculate_mana_max() const {
    int intelligence = balance.mana.intelligence_human;
    double f_race = balance.mana.race_mana_factor_human;
    switch (race) {
        case Race::HUMAN: intelligence = balance.mana.intelligence_human; f_race = balance.mana.race_mana_factor_human; break;
        case Race::ELF:   intelligence = balance.mana.intelligence_elf;   f_race = balance.mana.race_mana_factor_elf;   break;
        case Race::DWARF: intelligence = balance.mana.intelligence_dwarf; f_race = balance.mana.race_mana_factor_dwarf; break;
        case Race::GNOME: intelligence = balance.mana.intelligence_gnome; f_race = balance.mana.race_mana_factor_gnome; break;
    }
    double f_class = balance.mana.class_mana_factor_warrior;
    switch (player_class) {
        case PlayerClass::WARRIOR: f_class = balance.mana.class_mana_factor_warrior; break;
        case PlayerClass::PALADIN: f_class = balance.mana.class_mana_factor_paladin; break;
        case PlayerClass::CLERIC:  f_class = balance.mana.class_mana_factor_cleric;  break;
        case PlayerClass::MAGE:    f_class = balance.mana.class_mana_factor_mage;    break;
    }
    return static_cast<uint32_t>(intelligence * f_class * f_race * level);
}

uint32_t Player::exp_to_next_level() const {
    return static_cast<uint32_t>(balance.level_exp_base *
                                 std::pow(level, balance.level_exp_exponent));
}

bool Player::equip(uint8_t inv_slot_index) {
    if (inv_slot_index >= inventory.slot_count()) return false;
    if (inventory.is_empty_at(inv_slot_index)) return false;

    InventorySlot item = inventory.at(inv_slot_index);
    ItemType type = item.item_type;

    if (type == ItemType::HEALTH_POTION) {
        heal(hp_max);
        inventory.clear(inv_slot_index);
        return true;
    }
    if (type == ItemType::MANA_POTION) {
        mana_current = mana_max;
        inventory.clear(inv_slot_index);
        return true;
    }

    EquipSlot target = equip_slot_for(type);
    uint8_t eslot = static_cast<uint8_t>(target);

    if (equipped[eslot].item_type == ItemType::NONE) {
        equipped[eslot] = item;
        inventory.clear(inv_slot_index);
        equipped[eslot].slot_index = eslot;
    } else {
        InventorySlot replaced = equipped[eslot];
        equipped[eslot] = item;
        equipped[eslot].slot_index = eslot;
        inventory.place_at(inv_slot_index, replaced.item_type, replaced.item_name,
                           replaced.sprite_id);
    }
    return true;
}

void Player::unequip(EquipSlot eslot) {
    uint8_t index = static_cast<uint8_t>(eslot);
    if (equipped[index].item_type == ItemType::NONE) return;
    if (inventory.is_full()) return;

    uint8_t free_slot = inventory.first_free_slot();
    inventory.place_at(free_slot, equipped[index].item_type, equipped[index].item_name,
                       equipped[index].sprite_id);
    equipped[index] = InventorySlot{};
    equipped[index].item_type = ItemType::NONE;
}

const InventorySlot& Player::get_equipped(EquipSlot eslot) const {
    return equipped[static_cast<uint8_t>(eslot)];
}

void Player::dump_equipped(InventorySlot out[4]) const {
    for (int i = 0; i < 4; ++i) out[i] = equipped[i];
}
