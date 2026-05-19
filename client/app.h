#ifndef CLIENT_APP_H
#define CLIENT_APP_H

#include <cstdint>

#include <SDL2/SDL.h>

#include "config.h"
#include "engine.h"

namespace client_app {

enum class GameState { Menu, Login, Playing };

void init_image();
void shutdown_image();
void init_ttf();
void shutdown_ttf();
ClientConfig load_config();
bool handle_menu_event(const SDL_Event& event, const ClientEngine& client, GameState& state);
bool handle_login_event(const SDL_Event& event, ClientEngine& client, GameState& state);
bool handle_playing_event(const SDL_Event& event, ClientEngine& client);
void render_menu(ClientEngine& client);
void render_login(ClientEngine& client);
void render_playing(ClientEngine& client);
bool pump_events(ClientEngine& client, GameState& state);

}  // namespace client_app

#endif
