#ifndef GAME_H_
#define GAME_H_

#include <vector>

#include "../common/messages.h"

#include "config.h"
#include "map.h"
#include "player.h"

class Game {
private:
    Player player;
    Map map;
    int move_step;
    int sprite_width;
    int sprite_height;

    std::vector<ServerEvent> handle_move(const MoveCmd& cmd);

public:
    Game(uint16_t player_id, const ServerConfig& config);

    std::vector<ServerEvent> process_command(const ClientCommand& cmd);
    std::vector<ServerEvent> tick();
    std::vector<ServerEvent> get_initial_events();
};

#endif  // GAME_H_
