#include "server.h"

#include <iostream>

Server::Server(const ServerConfig& cfg):
        config(cfg),
        listener(std::to_string(cfg.port).c_str()),
        input_queue(),
        monitor(),
        acceptor(listener, input_queue, monitor),
        game_loop(config, input_queue, monitor) {}

void Server::run() {
    std::cout << "Server listening on port " << config.port << "...\n";
    acceptor.start();
    game_loop.start();
    game_loop.join();
    shutdown();
}

void Server::shutdown() {
    acceptor.stop();
    acceptor.join();
    input_queue.close();
    monitor.stop_all();
}
