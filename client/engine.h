#ifndef CLIENT_ENGINE_H
#define CLIENT_ENGINE_H

#include <SDL2/SDL.h>
#include <SDL2pp/SDL2pp.hh>

#include "../common/messages.h"

#include "chat_input.h"
#include "config.h"
#include "move_controller.h"
#include "renderer.h"

class ClientProtocol;

class ClientEngine {
private:
    // objetos de SDL que viven durante toda la vida del engine.
    SDL2pp::SDL sdl;
    SDL2pp::Window window;
    ClientRenderer renderer;
    ClientProtocol& protocol;
    MoveController move_controller;
    ChatInput chat_input;

public:
    explicit ClientEngine(const ClientConfig& config, ClientProtocol& protocol);
    ~ClientEngine();

    void tick();
    void show_menu();
    void show_sprite();
    void apply_server_event(const ServerEvent& ev);
    bool handle_event(const SDL_Event& event);
    bool is_menu_click(int x, int y) const;

private:
    bool handle_mouse_button(const SDL_Event& event);
    bool handle_keydown(const SDL_Event& event);
    void sync_chat_to_renderer();
};

#endif
