#ifndef CLIENT_ENGINE_H
#define CLIENT_ENGINE_H

#include <string>

#include <SDL2/SDL.h>
#include <SDL2pp/SDL2pp.hh>

#include "../common/messages.h"

#include "chat_input.h"
#include "config.h"
#include "menu_renderer.h"
#include "move_controller.h"
#include "ui_renderer.h"
#include "world_renderer.h"

class ClientProtocol;

class ClientEngine {
private:
    // Logical dimensions of the game world. The actual window size may differ, but the game will be rendered as if it were this size.
    static constexpr int LOGICAL_W = 1024;
    static constexpr int LOGICAL_H = 768;

    SDL2pp::SDL sdl;
    SDL2pp::Window window;
    SDL2pp::Renderer sdl_renderer;
    MenuRenderer menu_renderer;
    WorldRenderer world_renderer;
    UIRenderer ui_renderer;
    ChatInput chat_input;
    ClientProtocol& protocol;
    MoveController move_controller;

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
    void render_game_frame();
    void render_menu_frame();
    bool handle_mouse_button(const SDL_Event& event);
    bool handle_keydown(const SDL_Event& event);
};

#endif
