#include <iostream>

#include "config.h"
#include "server.h"

int main() try {
    ServerConfig config = load_server_config("config/server.toml");
    Server server(config);
    server.run();
    return 0;
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
}
