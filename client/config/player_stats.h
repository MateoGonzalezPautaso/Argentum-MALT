#ifndef CLIENT_PLAYER_STATS_H
#define CLIENT_PLAYER_STATS_H

#include <cstdint>
#include <string>
#include <vector>

#include "../../common/messages.h"

struct PlayerStats {
    uint16_t player_id = 0;
    std::string username;
    Race race = Race::HUMAN;
    PlayerClass player_class = PlayerClass::WARRIOR;
    uint8_t level = 1;
    uint32_t experience = 0;
    uint32_t exp_to_next = 0;
    uint32_t hp_current = 0;
    uint32_t hp_max = 0;
    uint32_t mana_current = 0;
    uint32_t mana_max = 0;
    uint32_t gold = 0;
    uint8_t crit_chance = 0;
    uint16_t damage_min = 0;
    uint16_t damage_max = 0;
    uint16_t defense_min = 0;
    uint16_t defense_max = 0;
    Position pos;
    std::vector<InventorySlot> inventory;
    InventorySlot equipped[4] = {};
};

#endif  // CLIENT_PLAYER_STATS_H
