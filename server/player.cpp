#include "player.h"

#include <algorithm>
#include <cmath>

Player::Player(uint16_t id, const std::string& username, Position pos, Direction dir, Race race, Class class_):
        id(id),
        username(username),
        pos(pos),
        dir(dir),
        race(race),
        class_(class_),
        level(1),
        experience(0),
        hp_current(STARTING_HP),
        hp_max(STARTING_HP),
        mana_current(STARTING_MANA),
        mana_max(STARTING_MANA),
        gold(STARTING_GOLD) {}

void Player::move(Direction new_dir) { dir = new_dir; }

void Player::gain_experience(uint32_t exp) {
    experience += exp;
    uint32_t threshold = static_cast<uint32_t>(1000 * std::pow(level, 1.8));
    while (experience >= threshold && level < UINT8_MAX) {
        experience -= threshold;
        level_up();
        threshold = static_cast<uint32_t>(1000 * std::pow(level, 1.8));
    }
}

void Player::level_up() {
    ++level;
    hp_max += HP_INCREASE_PER_LEVEL;
    mana_max += MANA_INCREASE_PER_LEVEL;
    gold += GOLD_INCREASE_PER_LEVEL * level;
    hp_current = hp_max;
    mana_current = mana_max;
}

void Player::take_damage(uint32_t damage) {
    if (damage >= hp_current) {
        hp_current = 0;
    } else {
        hp_current -= damage;
    }
}

void Player::heal(uint32_t amount) {
    uint32_t total = static_cast<uint64_t>(hp_current) + amount;
    hp_current = static_cast<uint32_t>(std::min<uint64_t>(total, hp_max));
}

void Player::gain_gold(uint32_t amount) {
    uint64_t max_gold = static_cast<uint64_t>(100 * std::pow(level, 1.1));
    uint64_t total = static_cast<uint64_t>(gold) + amount;
    gold = static_cast<uint32_t>(std::min<uint64_t>(total, max_gold));
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

void Player::use_mana(uint32_t amount) {
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
