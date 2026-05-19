#ifndef SERVER_H
#define SERVER_H

#include "../common/queue.h"
#include "../common/socket.h"

#include "acceptor.h"
#include "client_list_monitor.h"
#include "config.h"
#include "game.h"
#include "player_command.h"

class Server {
    ServerConfig config;
    Socket listener;
    Queue<PlayerCommand> input_queue;
    ClientListMonitor monitor;
    Acceptor acceptor;
    Game game;

    void game_loop();
    void shutdown();

public:
    explicit Server(const ServerConfig& config);

    void run();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
};

#endif  // SERVER_H
