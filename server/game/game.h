#ifndef GAME_H
#define GAME_H

#include <cstdint>
#include <map>
#include <string>

#include "../../common/messages.h"
#include "../core/config.h"
#include "../persistence/player_persistence.h"

#include "combat_controller.h"
#include "command_result.h"
#include "map.h"
#include "player.h"

class Game {
private:
    std::map<uint16_t, Player> players;
    PlayerPersistence& persistence;
    Map map;
    int move_step;
    int sprite_width;
    int sprite_height;
    BalanceConfig balance;
    CombatController combat_controller;
    uint32_t tick_count = 0;

    CommandResult handle_login(uint16_t player_id, const LoginCmd& cmd);
    CommandResult handle_move(uint16_t player_id, const MoveCmd& cmd);
    CommandResult handle_attack(uint16_t player_id, const AttackCmd& cmd);
    CommandResult handle_send_chat_msg(uint16_t player_id, const SendChatMsgCmd& cmd);
    CommandResult handle_resurrect(uint16_t player_id);
    bool is_username_logged_in(const std::string& username) const;

public:
    explicit Game(const ServerConfig& config, PlayerPersistence& persistence);

    CommandResult process_command(uint16_t player_id, const ClientCommand& cmd);
    CommandResult remove_player(uint16_t player_id);
    CommandResult tick();
};

#endif  // GAME_H
