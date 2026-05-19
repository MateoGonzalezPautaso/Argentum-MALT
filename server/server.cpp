#include "server.h"

#include <iostream>

Server::Server(const ServerConfig& cfg):
        config(cfg),
        listener(std::to_string(cfg.port).c_str()),
        input_queue(),
        monitor(),
        acceptor(listener, input_queue, monitor),
        game(config) {}

void Server::run() {
    std::cout << "Server listening on port " << config.port << "...\n";
    acceptor.start();
    game_loop();
    shutdown();
}

void Server::game_loop() {
    while (true) {
        PlayerCommand pcmd = input_queue.pop();
        const auto events = game.process_command(pcmd.player_id, pcmd.cmd);
        for (const auto& ev : events)
            monitor.broadcast(ev);
        monitor.clean_dead();
    }
}

void Server::shutdown() {
    acceptor.stop();
    acceptor.join();
    input_queue.close();
    monitor.stop_all();
}
