#include "client.h"

#include <iostream>
#include <string>

#include <SDL2/SDL.h>

#include "app.h"

Client::Client(const ClientConfig& cfg):
        config(cfg),
        skt("127.0.0.1", "1234"),
        protocol(std::move(skt)),
        engine(config, command_queue),
        sender(protocol, command_queue),
        receiver(protocol, event_queue) {}

LoginOkEvent Client::do_login() {
    std::string username;
    std::string password;

    std::cout << "Username: ";
    std::getline(std::cin, username);
    std::cout << "Password: ";
    std::getline(std::cin, password);

    protocol.send_command(LoginCmd{username, password});

    ServerEvent response = protocol.recv_event();

    if (std::holds_alternative<LoginOkEvent>(response)) {
        return std::get<LoginOkEvent>(response);
    }

    if (std::holds_alternative<LoginErrorEvent>(response)) {
        throw std::runtime_error("Login failed: " + std::get<LoginErrorEvent>(response).message);
    }

    throw std::runtime_error("Unexpected response to login");
}

void Client::game_loop() {
    client_app::GameState state = client_app::GameState::Playing;
    bool running = true;
    const uint32_t tick_ms = config.tick_ms;
    uint32_t last_tick = SDL_GetTicks();

    while (running) {
        running = client_app::pump_events(engine, state);
        if (!running) {
            break;
        }

        ServerEvent ev;
        while (event_queue.try_pop(ev)) {
            engine.apply_server_event(ev);
        }

        const uint32_t now = SDL_GetTicks();
        const uint32_t elapsed = now - last_tick;
        if (elapsed >= tick_ms) {
            last_tick = now;
            client_app::render_playing(engine);
        }

        const uint32_t sleep_ms = (elapsed < tick_ms) ? (tick_ms - elapsed) : 0;
        if (sleep_ms > 0) {
            SDL_Delay(sleep_ms);
        }
    }
}

void Client::shutdown() {
    command_queue.close();
    sender.join();

    protocol.shutdown();
    receiver.join();
}

void Client::run() {
    LoginOkEvent login_ev = do_login();
    std::cout << "Logged in as " << login_ev.username << std::endl;

    engine.apply_server_event(login_ev);

    sender.start();
    receiver.start();

    game_loop();

    shutdown();
}
