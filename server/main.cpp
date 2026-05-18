#include <iostream>

#include "../common/socket.h"

#include "game.h"
#include "server_protocol.h"

int main() try {
    Socket listener("1234");
    std::cout << "Server listening on port 1234..." << std::endl;

    ServerProtocol srv_prot(listener.accept());
    std::cout << "Client connected!" << std::endl;

    Game game(1);
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
