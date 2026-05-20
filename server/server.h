#ifndef SERVER_H
#define SERVER_H

#include "../common/queue.h"
#include "../common/socket.h"

#include "acceptor.h"
#include "client_list_monitor.h"
#include "config.h"
#include "game_loop.h"
#include "player_command.h"
#include "player_persistence.h"

class Server {
    ServerConfig config;
    Socket listener;
    Queue<PlayerCommand> input_queue;
    ClientListMonitor monitor;
    PlayerPersistence persistence;
    Acceptor acceptor;
    GameLoop game_loop;

    void stop();

public:
    explicit Server(const ServerConfig& config);

    void run();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
};

#endif  // SERVER_H
