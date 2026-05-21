#ifndef CLIENT_ENGINE_H
#define CLIENT_ENGINE_H

#include <SDL2/SDL.h>
#include <SDL2pp/SDL2pp.hh>

#include "../common/messages.h"
#include "../common/queue.h"

#include "chat_input.h"
#include "config.h"
#include "move_controller.h"
#include "player_stats.h"
#include "render/login_renderer.h"
#include "render/menu_renderer.h"
#include "render/ui_renderer.h"
#include "render/world_renderer.h"

class ClientEngine {
private:
    static constexpr int LOGICAL_W = 1024;
    static constexpr int LOGICAL_H = 768;

    SDL2pp::SDL sdl;
    SDL2pp::Window window;
    SDL2pp::Renderer sdl_renderer;
    MenuRenderer menu_renderer;
    LoginRenderer login_renderer;
    WorldRenderer world_renderer;
    UIRenderer ui_renderer;
    ChatInput chat_input;
    ChatInput login_username;
    ChatInput login_password;
    int login_active_field = 0;
    bool login_submitted = false;
    PlayerStats player_stats;
    MoveController move_controller;

public:
    explicit ClientEngine(const ClientConfig& config, Queue<ClientCommand>& command_queue);
    ~ClientEngine();

    void tick();
    void show_menu();
    void show_login();
    void show_sprite();
    void apply_server_event(const ServerEvent& ev);
    bool handle_event(const SDL_Event& event);
    void handle_login_event(const SDL_Event& event);
    bool is_menu_click(int x, int y) const;
    bool is_menu_settings_click(int x, int y) const;

    bool is_login_submitted() const { return login_submitted; }
    void reset_login_submitted();
    const std::string& login_username_text() const;
    const std::string& login_password_text() const;

    void handle_menu_mouse_motion(int x, int y);
    void handle_login_mouse_motion(int x, int y);

private:
    void render_game_frame();
    void render_menu_frame();
    void render_login_frame();
    bool handle_mouse_button(const SDL_Event& event);
    bool handle_keydown(const SDL_Event& event);
};

#endif
