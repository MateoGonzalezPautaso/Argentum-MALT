#include <exception>
#include <iostream>
#include <string>

#include "config/config.h"
#include "core/client.h"

int main(int argc, char* argv[]) try {
    ClientConfig config = load_client_config("config/client.toml");

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--fullscreen" || arg == "--full-screen") {
            config.window.fullscreen = true;
        } else if (arg == "--width" && i + 1 < argc) {
            config.window.width = std::stoi(argv[++i]);
        } else if (arg == "--height" && i + 1 < argc) {
            config.window.height = std::stoi(argv[++i]);
        }
    }

    Client client(config);
    client.run();
    return 0;
} catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
}
