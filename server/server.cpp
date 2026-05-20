#include "server.h"

#include <iostream>

#include "game.h"

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
        CommandResult result = game.process_command(pcmd.player_id, pcmd.cmd);

        for (const ServerEvent& ev: result.private_events) 
            monitor.push_event(pcmd.player_id, ev);

        for (const ServerEvent& ev: result.broadcast_events) 
            monitor.broadcast(ev);

        for (uint16_t dead_id: monitor.clean_dead()) {
            CommandResult despawn = game.remove_player(dead_id);

            for (const ServerEvent& ev: despawn.broadcast_events) 
                monitor.broadcast(ev);
        }
    }
}

void Server::shutdown() {
    acceptor.stop();
    acceptor.join();
    input_queue.close();
    monitor.stop_all();
}
