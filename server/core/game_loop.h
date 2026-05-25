#ifndef GAME_LOOP_H
#define GAME_LOOP_H

#include "../../common/queue.h"
#include "../../common/thread.h"

#include "../network/client_list_monitor.h"
#include "config.h"
#include "../game/game.h"
#include "../game/player_command.h"
#include "../persistence/player_persistence.h"

class GameLoop: public Thread {
    int tick_rate_hz;
    Game game;
    Queue<PlayerCommand>& input_queue;
    ClientListMonitor& monitor;

public:
    GameLoop(const ServerConfig& config, Queue<PlayerCommand>& input_queue,
             ClientListMonitor& monitor, PlayerPersistence& persistence);

    void run() override;
};

#endif  // GAME_LOOP_H
