#ifndef CLIENT_ENGINE_H
#define CLIENT_ENGINE_H

#include <string>

#include "../../common/messages.h"
#include "../../common/queue.h"
#include "../audio/audio_manager.h"
#include "../config/config.h"
#include "../render/gfx/render_context.h"
#include "../ui/controllers/create_character_controller.h"
#include "../ui/controllers/login_controller.h"
#include "../ui/controllers/menu_controller.h"

#include "game_controller.h"

enum class GameState { Menu, Login, CreateCharacter, Playing };

class Engine {
public:
    Engine(const ClientConfig& config, Queue<ClientCommand>& command_queue,
           AudioManager& audio_manager);

    bool dispatch_event(GameState& state);

    void render_menu_frame();
    void render_login_frame();
    void render_create_char_frame();
    void render_game_frame();
    void tick_game();
    void apply_server_event(const ServerEvent& ev);
    void load_game_assets();
    bool is_world_map_loaded() const;

    bool try_submit_login(std::string& username, std::string& password);
    void handle_login_error(const std::string& msg);
    void reset_login_state();

    bool try_submit_create_character(std::string& username, std::string& password, Race& race,
                                     PlayerClass& player_class);
    void handle_create_char_error(const std::string& msg);
    void reset_create_char_state();
    bool wants_create_character() const;

private:
    RenderContext render_ctx;
    MenuController menu_ctrl;
    LoginController login_ctrl;
    CreateCharacterController create_char_ctrl;
    AudioManager& audio_manager;
    GameController game_controller;
    bool create_char_requested = false;

    bool dispatch_menu_event(const SDL_Event& event, GameState& state);
    void dispatch_login_event(const SDL_Event& event, GameState& state);
    void dispatch_create_char_event(const SDL_Event& event, GameState& state);
};

#endif
