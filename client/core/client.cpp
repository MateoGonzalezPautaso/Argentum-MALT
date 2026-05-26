#include "client.h"

#include <iostream>
#include <string>
#include <utility>

#include <SDL2/SDL.h>

Client::Client(const ClientConfig& cfg):
        config(cfg),
        skt("127.0.0.1", "1234"),
        protocol(std::move(skt)),
        engine(config, command_queue),
        sender(protocol, command_queue),
        receiver(protocol, event_queue) {
    SDL_StartTextInput();
}

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
    GameState state = GameState::Menu;
    bool running = true;
    uint32_t last_tick = SDL_GetTicks();

    while (running && state == GameState::Menu) {
        running = engine.dispatch_event(state);
        if (!running) {
            break;
        }

        frame_sync(last_tick, [&] { engine.render_menu_frame(); });
    }
    return running;
}

std::optional<LoginOkEvent> Client::run_login() {
    GameState state = GameState::Login;
    bool running = true;
    uint32_t last_tick = SDL_GetTicks();
    bool login_sent = false;

    ServerEvent discard;
    while (event_queue.try_pop(discard));

    while (running && state == GameState::Login) {
        running = engine.dispatch_event(state);
        if (!running) {
            break;
        }

        if (state == GameState::Menu) {
            engine.reset_login_state();
            return std::nullopt;
        }

        if (!login_sent) {
            std::string username;
            std::string password;
            if (engine.try_submit_login(username, password)) {
                command_queue.push(LoginCmd{username, password});
                login_sent = true;
            }
        }

        if (login_sent) {
            ServerEvent ev;
            while (event_queue.try_pop(ev)) {
                if (std::holds_alternative<LoginOkEvent>(ev)) {
                    return std::get<LoginOkEvent>(ev);
                }
                if (std::holds_alternative<LoginErrorEvent>(ev)) {
                    engine.handle_login_error(std::get<LoginErrorEvent>(ev).message);
                    login_sent = false;
                }
            }
        }

        frame_sync(last_tick, [&] { engine.render_login_frame(); });
    }
    return std::nullopt;
}

void Client::game_loop() {
    GameState state = GameState::Playing;
    bool running = true;
    uint32_t last_tick = SDL_GetTicks();

    while (running) {
        running = engine.dispatch_event(state);
        if (!running) {
            break;
        }

        ServerEvent ev;
        while (event_queue.try_pop(ev)) {
            engine.apply_server_event(ev);
        }

        engine.tick_game();
        frame_sync(last_tick, [&] { engine.render_game_frame(); });
    }
}

void Client::shutdown() {
    command_queue.close();
    sender.join();

    protocol.shutdown();
    receiver.join();
}

void Client::run() {
    sender.start();
    receiver.start();

    while (true) {
        if (!run_menu()) {
            shutdown();
            return;
        }

        auto login_result = run_login();
        if (!login_result) {
            engine.reset_login_state();
            continue;
        }

        std::cout << "Logged in as " << login_result->username << std::endl;

        engine.apply_server_event(*login_result);

        break;
    }

    game_loop();

    shutdown();
}
