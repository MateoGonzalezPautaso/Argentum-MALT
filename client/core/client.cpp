#include "client.h"

#include <iostream>
#include <string>
#include <utility>

#include <SDL2/SDL.h>

Client::Client(const ClientConfig& cfg):
        config(cfg),
        skt(cfg.network.host.c_str(), cfg.network.port.c_str()),
        protocol(std::move(skt)),
        engine(config, command_queue),
        audio_manager(config.sfx),
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

    audio_manager.play_menu_music();

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

    audio_manager.play_game_music();

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

    try {
        protocol.shutdown();
    } catch (...) {}
    receiver.join();
}

std::optional<LoginOkEvent> Client::run_create_character() {
    GameState state = GameState::CreateCharacter;
    bool running = true;
    uint32_t last_tick = SDL_GetTicks();
    bool cmd_sent = false;

    ServerEvent discard;
    while (event_queue.try_pop(discard))
        ;

    while (running && state == GameState::CreateCharacter) {
        running = engine.dispatch_event(state);
        if (!running)
            break;

        if (state == GameState::Login) {
            engine.reset_create_char_state();
            return std::nullopt;
        }

        if (!cmd_sent) {
            std::string username, password;
            Race race;
            PlayerClass player_class;
            if (engine.try_submit_create_character(username, password, race, player_class)) {
                command_queue.push(CreateCharacterCmd{username, password, race, player_class});
                cmd_sent = true;
            }
        }

        if (cmd_sent) {
            ServerEvent ev;
            while (event_queue.try_pop(ev)) {
                if (std::holds_alternative<CharacterCreatedEvent>(ev))
                    return std::get<CharacterCreatedEvent>(ev).data;
                if (std::holds_alternative<CharacterErrorEvent>(ev)) {
                    engine.handle_create_char_error(std::get<CharacterErrorEvent>(ev).message);
                    cmd_sent = false;
                }
            }
        }

        frame_sync(last_tick, [&] { engine.render_create_char_frame(); });
    }
    return std::nullopt;
}

void Client::run() {
    sender.start();
    receiver.start();

    while (true) {
        if (!run_menu()) {
            shutdown();
            return;
        }

        std::optional<LoginOkEvent> auth_result;
        while (!auth_result) {
            auto login = run_login();
            if (login) {
                auth_result = login;
            } else if (engine.wants_create_character()) {
                engine.reset_create_char_state();
                auto created = run_create_character();
                if (created)
                    auth_result = created;
                // if nullopt (back pressed), loop back to run_login
            } else {
                engine.reset_login_state();
                break;  // back to menu
            }
        }

        if (auth_result) {
            std::cout << "Logged in as " << auth_result->username << std::endl;
            engine.apply_server_event(*auth_result);
            break;
        }
    }

    game_loop();
    shutdown();
}
