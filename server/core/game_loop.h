#ifndef GAME_LOOP_H
#define GAME_LOOP_H

#include "../../common/queue.h"
#include "../../common/thread.h"
#include "../game/game.h"
#include "../game/player_command.h"
#include "../game/player_data_service.h"
#include "../network/client_list_monitor.h"
#include "../persistence/clan_persistence.h"

#include "config.h"

class GameLoop: public Thread {
    int tick_rate_hz;
    int save_interval_ticks;
    Game game;
    Queue<PlayerCommand>& input_queue;
    ClientListMonitor& monitor;

public:
    GameLoop(const ServerConfig& config, Queue<PlayerCommand>& input_queue,
             ClientListMonitor& monitor, PlayerDataService& player_data_service,
             ClanPersistence& clan_persistence);

    void run() override;
};

#endif  // GAME_LOOP_H
