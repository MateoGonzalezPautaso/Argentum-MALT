#ifndef CLIENT_GAME_CONTROLLER_H
#define CLIENT_GAME_CONTROLLER_H

#include <string>
#include <unordered_map>

#include <SDL2/SDL.h>
#include <SDL2pp/SDL2pp.hh>

#include "../../common/messages.h"
#include "../../common/queue.h"
#include "../audio/audio_manager.h"
#include "../chat/chat_command_parser.h"
#include "../chat/chat_history.h"
#include "../config/config.h"
#include "../config/player_stats.h"
#include "../input/chat_input.h"
#include "../input/move_controller.h"
#include "../ui/hud/ui_renderer.h"
#include "../render/world_renderer.h"
#include "../ui/controllers/merchant_controller.h"
#include "server_event_handler.h"

class GameController {
public:
    GameController(SDL2pp::Renderer& renderer, const ClientConfig& config,
                   Queue<ClientCommand>& command_queue, AudioManager& audio_manager);
    ~GameController();

    void tick();
    void render();
    void apply_server_event(const ServerEvent& ev);
    bool handle_event(const SDL_Event& event);
    bool is_chat_focused() const { return chat_input.is_focused(); }
    void load_game_assets();

private:
    AudioManager& audio_manager;
    const ClientConfig& config;
    SDL2pp::Renderer& renderer;
    ChatInput chat_input;
    ChatHistory chat_history;
    ChatCommandParser chat_parser_;
    PlayerStats player_stats;
    bool player_is_ghost = false;
    int chat_scroll = 0;
    bool chat_expanded_ = false;
    std::string current_map_name = "city";
    WorldRenderer world_renderer;
    UIRenderer ui_renderer;
    Queue<ClientCommand>& command_queue;
    MoveController move_controller;
    MoveConfig move_config;
    SDL_Cursor* hand_cursor;
    SDL_Cursor* arrow_cursor;
    int mouse_x = 0;
    int mouse_y = 0;
    std::unordered_map<SDL_Keycode, ClientCommand> cheat_commands_;
    std::unique_ptr<MerchantController> merchant_controller;
    ServerEventHandler event_handler_;

    bool handle_mouse_button(const SDL_Event& event);
    bool handle_mouse_motion(const SDL_Event& event);
    bool handle_keydown(const SDL_Event& event);
    void flush_pending_chat();
    void apply_movement_visual(Direction dir, bool advance_frame);
    void interact_with_prop(const std::string& prop_name, int world_x, int world_y);
    bool is_clickable_prop(const std::string& prop_name) const;
    bool is_transition_prop(const std::string& prop_name) const;
};

#endif
