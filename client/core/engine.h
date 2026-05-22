#ifndef CLIENT_ENGINE_H
#define CLIENT_ENGINE_H

#include <string>

#include "../../common/messages.h"
#include "../../common/queue.h"

#include "../config/config.h"
#include "../screen/game_controller.h"
#include "../screen/login_controller.h"
#include "../screen/menu_controller.h"
#include "../render/render_context.h"

enum class GameState { Menu, Login, Playing };

class Engine {
public:
    Engine(const ClientConfig& config, Queue<ClientCommand>& command_queue);

    bool dispatch_event(GameState& state);

    void render_menu_frame();
    void render_login_frame();
    void render_game_frame();
    void tick_game();
    void apply_server_event(const ServerEvent& ev);

    bool try_submit_login(std::string& username, std::string& password);
    void handle_login_error(const std::string& msg);
    void reset_login_state();

private:
    RenderContext render_ctx;
    MenuController menu_ctrl;
    LoginController login_ctrl;
    GameController game_controller;

    bool dispatch_menu_event(const SDL_Event& event, GameState& state);
    void dispatch_login_event(const SDL_Event& event, GameState& state);
};

#endif
