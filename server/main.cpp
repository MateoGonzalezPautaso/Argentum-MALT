#include <iostream>

#include "../common/socket.h"

#include "server_protocol.h"

template <class... Ts>
struct overloaded: Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

int main() try {
    Socket listener("1234");
    std::cout << "Server listening on port 1234..." << std::endl;

    ServerProtocol srv_prot(listener.accept());
    std::cout << "Client connected!" << std::endl;

    while (true) {
        ClientCommand cmd = srv_prot.recv_command();

        std::visit(
                overloaded{
                        [](const MoveCmd& msg) {
                            std::cout << "MOVE received! Direction: "
                                      << static_cast<int>(msg.direction) << std::endl;
                        },
                        [](const auto&) { std::cout << "Unknown command received" << std::endl; },
                },
                cmd);
    }

    return 0;
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
}
