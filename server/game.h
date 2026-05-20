#ifndef GAME_H
#define GAME_H

#include <cstdint>
#include <map>
#include <vector>

#include "../common/messages.h"

#include "config.h"
#include "map.h"
#include "player.h"

struct CommandResult {
    std::vector<ServerEvent> private_events;    // only to the player who sent the command
    std::vector<ServerEvent> broadcast_events;  // to all connected players
};

class Game {
private:
    std::map<uint16_t, Player> players;
    Map map;
    int move_step;
    int sprite_width;
    int sprite_height;
    BalanceConfig balance;

    CommandResult handle_login(uint16_t player_id, const LoginCmd& cmd);
    CommandResult handle_move(uint16_t player_id, const MoveCmd& cmd);

public:
    explicit Game(const ServerConfig& config);

    CommandResult process_command(uint16_t player_id, const ClientCommand& cmd);
    CommandResult tick();
};

#endif  // GAME_H
