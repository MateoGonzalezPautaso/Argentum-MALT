#ifndef GAME_H_
#define GAME_H_

#include <vector>

#include "../common/messages.h"

#include "player.h"

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
