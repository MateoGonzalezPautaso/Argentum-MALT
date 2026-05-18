#include <iostream>
#include <string>

#include "../common/socket.h"

#include "config.h"
#include "game.h"
#include "server_protocol.h"

int main() try {
    ServerConfig server_config = load_server_config("config/server.toml");

    std::string port_str = std::to_string(server_config.port);
    Socket listener(port_str.c_str());
    std::cout << "Server listening on port " << server_config.port << "..." << std::endl;

    ServerProtocol srv_prot(listener.accept());
    std::cout << "Client connected!" << std::endl;

    Game game(1, server_config);
    for (const auto& ev: game.get_initial_events()) {
        srv_prot.send_event(ev);
    }

    while (true) {
        ClientCommand cmd = srv_prot.recv_command();
        const auto events = game.process_command(cmd);
        for (const auto& ev: events) {
            srv_prot.send_event(ev);
        }
    }

    return 0;
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
}
