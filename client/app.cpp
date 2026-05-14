#include "app.h"

#include <stdexcept>
#include <string>

#include <SDL2/SDL_image.h>

namespace client_app {

void init_image() {
    const int img_flags = IMG_INIT_PNG;
    if ((IMG_Init(img_flags) & img_flags) != img_flags) {
        throw std::runtime_error(std::string("IMG_Init failed: ") + IMG_GetError());
    }
}

void shutdown_image() {
    IMG_Quit();
}

ClientConfig load_config() {
    const std::string config_path = "client_config.toml";
    return load_client_config(config_path);
}

bool handle_menu_event(const SDL_Event& event, ClientEngine& client, GameState& state) {
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        return false;
    }
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        if (client.is_menu_click(event.button.x, event.button.y)) {
            state = GameState::Playing;
        }
    }
    return true;
}

bool handle_playing_event(const SDL_Event& event, ClientEngine& client) {
    return client.handle_event(event);
}

void render_menu(ClientEngine& client) {
    client.show_menu();
}

void render_playing(ClientEngine& client) {
    client.tick();
    client.show_sprite();
}

bool pump_events(ClientEngine& client, GameState& state) {
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return false;
        }
        if (state == GameState::Menu) {
            if (!handle_menu_event(event, client, state)) {
                return false;
            }
        } else {
            if (!handle_playing_event(event, client)) {
                return false;
            }
        }
    }
    return true;
}

}
