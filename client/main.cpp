#include <exception>
#include <iostream>

#include "core/app.h"
#include "core/client.h"

int main() try {
    client_app::init_image();
    client_app::init_ttf();

    Client client(client_app::load_config());
    client.run();

    client_app::shutdown_image();
    client_app::shutdown_ttf();
    return 0;
} catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
}
