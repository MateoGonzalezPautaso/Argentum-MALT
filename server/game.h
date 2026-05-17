#ifndef GAME_H_
#define GAME_H_

#include <cstdint>
#include <string>
#include <vector>

#include "../common/messages.h"

struct Player {
    uint16_t id;
    std::string username;
    Position pos;
    Direction dir;
    Race race;
    Class class_;
    uint8_t level;
    uint32_t experience;
    uint16_t hp_current;
    uint16_t hp_max;
    uint16_t mana_current;
    uint16_t mana_max;
    uint32_t gold;
};

class Game {
private:
    Player player;
    uint16_t world_w;
    uint16_t world_h;

    std::vector<ServerEvent> handle_move(const MoveCmd& cmd);

public:
    explicit Game(uint16_t player_id);

    std::vector<ServerEvent> process_command(const ClientCommand& cmd);
    std::vector<ServerEvent> tick();
    std::vector<ServerEvent> get_initial_events();
};

#endif  // GAME_H_
