#ifndef PLAYER_H
#define PLAYER_H

#include <cstdint>
#include <string>

#include "../../common/messages.h"
#include "../core/config.h"

class Player {
    friend class Game;
    friend class CombatController;

private:
    uint16_t id;
    std::string username;
    Position pos;
    Direction dir;
    Race race;
    PlayerClass player_class;
    uint8_t level;
    uint32_t experience;
    uint32_t hp_current;
    uint32_t hp_max;
    uint32_t mana_current;
    uint32_t mana_max;
    uint32_t gold;
    uint32_t next_attack_tick = 0;
    bool is_dead = false;
    BalanceConfig balance;

public:
    Player(uint16_t id, const std::string& username, Position pos, Direction dir, Race race,
           PlayerClass player_class, const BalanceConfig& balance);

    uint16_t pos_x() const { return pos.x; }
    uint16_t pos_y() const { return pos.y; }

    bool try_attack(uint32_t current_tick, uint32_t cooldown_ticks);
    bool is_ghost() const;

    void apply_move(Direction new_dir, int dx, int dy);
    void resurrect();
    void gain_experience(uint32_t exp);
    void level_up();
    void take_damage(uint32_t damage);
    void heal(uint32_t amount);
    void use_mana(uint32_t amount);
    void gain_gold(uint32_t amount);
    void spend_gold(uint32_t amount);

    uint32_t exp_to_next_level() const;

    void increase_max_hp(uint32_t amount);
    void increase_max_mana(uint32_t amount);
};

#endif  // PLAYER_H
