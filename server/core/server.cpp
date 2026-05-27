#include "server.h"

#include <iostream>
#include <string>

Server::Server(const ServerConfig& cfg):
        config(cfg),
        listener(std::to_string(cfg.port).c_str()),
        input_queue(),
        monitor(),
        persistence("data/players.dat", "data/players.idx"),
        clan_persistence("data/clans.dat", "data/clans.idx"),
        acceptor(listener, input_queue, monitor),
        game_loop(config, input_queue, monitor, persistence, clan_persistence) {}

void Server::run() {
    std::cout << "Server listening on port " << config.port << "...\n";
    std::cout << "Press 'q' + Enter to stop the server.\n";
    acceptor.start();
    game_loop.start();

    std::string line;
    while (std::getline(std::cin, line) && line != "q") {}

    stop();
}

void Server::stop() {
    game_loop.stop();
    game_loop.join();
    acceptor.stop();
    acceptor.join();
    input_queue.close();
    monitor.stop_all();
}
