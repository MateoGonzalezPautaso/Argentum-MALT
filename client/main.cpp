#include <exception>
#include <iostream>

#include "config/config.h"
#include "core/client.h"

int main() try {
    Client client(load_client_config("config/client.toml"));
    client.run();
    return 0;
} catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
}
