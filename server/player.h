#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include <cstdint>
#include "../common/messages.h"

#define MAX_LEVEL 100
#define HP_INCREASE_PER_LEVEL 10
#define MANA_INCREASE_PER_LEVEL 5
#define GOLD_INCREASE_PER_LEVEL 100

#define STARTING_HP 100
#define STARTING_MANA 50
#define STARTING_GOLD 0

class Player {
    friend class Game;

private:
    uint16_t id;
    std::string username;
    Position pos;
    Direction dir;
    Race race;
    Class class_;
    uint8_t level;
    uint32_t experience;
    uint32_t hp_current;
    uint32_t hp_max;
    uint32_t mana_current;
    uint32_t mana_max;
    uint32_t gold;

public:
    Player(uint16_t id, const std::string& username, Position pos, Direction dir, Race race, Class class_);

    void apply_move(Direction new_dir, int dx, int dy);
    void gain_experience(uint32_t exp);
    void level_up();
    void take_damage(uint32_t damage);
    void heal(uint32_t amount);
    void use_mana(uint32_t amount);
    void gain_gold(uint32_t amount);
    void spend_gold(uint32_t amount);

    void increase_max_hp(uint32_t amount);
    void increase_max_mana(uint32_t amount);
};

#endif // PLAYER_H
