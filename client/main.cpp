#include <exception>
#include <iostream>

#include <SDL2/SDL.h>

#include "../common/messages.h"
#include "../common/queue.h"
#include "../common/socket.h"

#include "app.h"
#include "client_protocol.h"
#include "engine.h"
#include "receiver.h"

int main() try {
    client_app::init_image();
    client_app::init_ttf();
    ClientConfig config = client_app::load_config();

    Socket skt("127.0.0.1", "1234");
    ClientProtocol proto(std::move(skt));
    ClientEngine client(config, proto);

    Queue<ServerEvent> event_queue;
    Receiver receiver(proto, event_queue);
    receiver.start();

    client_app::GameState state = client_app::GameState::Menu;
    bool running = true;
    const uint32_t tick_ms = config.tick_ms;
    uint32_t last_tick = SDL_GetTicks();

    while (running) {
        running = client_app::pump_events(client, state);
        if (!running) {
            break;
        }

        ServerEvent ev;
        while (event_queue.try_pop(ev)) {
            client.apply_server_event(ev);
        }

        const uint32_t now = SDL_GetTicks();
        const uint32_t elapsed = now - last_tick;
        if (elapsed >= tick_ms) {
            last_tick = now;
            if (state == client_app::GameState::Menu) {
                client_app::render_menu(client);
            } else {
                client_app::render_playing(client);
            }
        }

        const uint32_t sleep_ms = (elapsed < tick_ms) ? (tick_ms - elapsed) : 0;
        if (sleep_ms > 0) {
            SDL_Delay(sleep_ms);
        }
    }

    proto.shutdown();
    receiver.join();

    client_app::shutdown_image();
    client_app::shutdown_ttf();
    return 0;
} catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
}
