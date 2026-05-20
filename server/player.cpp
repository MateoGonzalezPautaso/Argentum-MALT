#include "player.h"

#include <algorithm>
#include <cmath>

Player::Player(uint16_t id, const std::string& username, Position pos, Direction dir, Race race,
               PlayerClass player_class, const BalanceConfig& balance):
        id(id),
        username(username),
        pos(pos),
        dir(dir),
        race(race),
        player_class(player_class),
        level(1),
        experience(0),
        hp_current(balance.starting_hp),
        hp_max(balance.starting_hp),
        mana_current(balance.starting_mana),
        mana_max(balance.starting_mana),
        gold(balance.starting_gold),
        balance(balance) {}

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
    hp_max += balance.hp_per_level;
    mana_max += balance.mana_per_level;
    gold += balance.gold_per_level * level;
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
    uint64_t max_gold = static_cast<uint64_t>(balance.gold_cap_base *
                                              std::pow(level, balance.gold_cap_exponent));
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

uint32_t Player::exp_to_next_level() const {
    return static_cast<uint32_t>(
            balance.level_exp_base * std::pow(level, balance.level_exp_exponent));
}
