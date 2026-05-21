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

void Client::frame_sync(uint32_t& last_tick, std::function<void()> render) const {
    const uint32_t now = SDL_GetTicks();
    const uint32_t elapsed = now - last_tick;
    if (elapsed >= config.tick_ms) {
        last_tick = now;
        render();
    }
    if (elapsed < config.tick_ms) {
        SDL_Delay(config.tick_ms - elapsed);
    }
}

bool Client::run_menu() {
    client_app::GameState state = client_app::GameState::Menu;
    bool running = true;
    uint32_t last_tick = SDL_GetTicks();

    while (running && state == client_app::GameState::Menu) {
        running = client_app::pump_events(engine, state);
        if (!running) {
            break;
        }

        frame_sync(last_tick, [&] { client_app::render_menu(engine); });
    }
    return running;
}

std::optional<LoginOkEvent> Client::run_login() {
    client_app::GameState state = client_app::GameState::Login;
    bool running = true;
    uint32_t last_tick = SDL_GetTicks();

    while (running && state == client_app::GameState::Login) {
        running = client_app::pump_events(engine, state);
        if (!running) {
            break;
        }

        if (state == client_app::GameState::Menu) {
            engine.clear_login_error();
            return std::nullopt;
        }

        if (engine.is_login_submitted()) {
            engine.clear_login_error();
            const std::string& username = engine.login_username_text();
            const std::string& password = engine.login_password_text();
            engine.reset_login_submitted();

            protocol.send_command(LoginCmd{username, password});

            ServerEvent response = protocol.recv_event();

            if (std::holds_alternative<LoginOkEvent>(response)) {
                return std::get<LoginOkEvent>(response);
            }

            if (std::holds_alternative<LoginErrorEvent>(response)) {
                engine.reset_login_fields();
                engine.set_login_error(std::get<LoginErrorEvent>(response).message);
            }
        }

        frame_sync(last_tick, [&] { client_app::render_login(engine); });
    }
    return std::nullopt;
}

void Client::game_loop() {
    client_app::GameState state = client_app::GameState::Playing;
    bool running = true;
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

        frame_sync(last_tick, [&] { client_app::render_playing(engine); });
    }
}

void Client::shutdown() {
    command_queue.close();
    sender.join();

    protocol.shutdown();
    receiver.join();
}

void Client::run() {
    while (true) {
        if (!run_menu()) {
            return;
        }

        auto login_result = run_login();
        if (!login_result) {
            engine.reset_login_submitted();
            continue;
        }

        std::cout << "Logged in as " << login_result->username << std::endl;

        engine.apply_server_event(*login_result);

        break;
    }

    sender.start();
    receiver.start();

    game_loop();

    shutdown();
}
