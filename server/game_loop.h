#ifndef GAME_LOOP_H
#define GAME_LOOP_H

#include "../common/queue.h"
#include "../common/thread.h"

#include "client_list_monitor.h"
#include "config.h"
#include "game.h"
#include "player_command.h"

class GameLoop: public Thread {
    int tick_rate_hz;
    Game game;
    Queue<PlayerCommand>& input_queue;
    ClientListMonitor& monitor;

public:
    GameLoop(const ServerConfig& config, Queue<PlayerCommand>& input_queue,
             ClientListMonitor& monitor);

    void run() override;
};

#endif  // GAME_LOOP_H
