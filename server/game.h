#ifndef GAME_H
#define GAME_H

#include <cstdint>
#include <map>
#include <vector>

#include "../common/messages.h"

#include "config.h"
#include "map.h"
#include "player.h"

class Game {
private:
    std::map<uint16_t, Player> players;
    Map map;
    int move_step;
    int sprite_width;
    int sprite_height;
    BalanceConfig balance;

    std::vector<ServerEvent> handle_login(uint16_t player_id, const LoginCmd& cmd);
    std::vector<ServerEvent> handle_move(uint16_t player_id, const MoveCmd& cmd);

public:
    explicit Game(const ServerConfig& config);

    std::vector<ServerEvent> process_command(uint16_t player_id, const ClientCommand& cmd);
    std::vector<ServerEvent> tick();
};

#endif  // GAME_H
