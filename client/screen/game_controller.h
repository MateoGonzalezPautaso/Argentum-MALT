#ifndef CLIENT_GAME_CONTROLLER_H
#define CLIENT_GAME_CONTROLLER_H

#include <SDL2/SDL.h>
#include <SDL2pp/SDL2pp.hh>

#include "../../common/messages.h"
#include "../../common/queue.h"

#include "../input/chat_input.h"
#include "../chat/chat_history.h"

#include "../config/config.h"
#include "../config/player_stats.h"
#include "../input/chat_input.h"
#include "../input/move_controller.h"
#include "../render/ui_renderer.h"
#include "../render/world_renderer.h"

class GameController {
public:
    GameController(SDL2pp::Renderer& renderer, const ClientConfig& config,
                   Queue<ClientCommand>& command_queue);
    ~GameController();

    void tick();
    void render();
    void apply_server_event(const ServerEvent& ev);
    bool handle_event(const SDL_Event& event);
    bool is_chat_focused() const { return chat_input.is_focused(); }

private:
    SDL2pp::Renderer& renderer;
    ChatInput chat_input;
    ChatHistory chat_history;
    PlayerStats player_stats;
    bool player_is_ghost = false;
    WorldRenderer world_renderer;
    UIRenderer ui_renderer;
    Queue<ClientCommand>& command_queue;
    MoveController move_controller;
    MoveConfig move_config;
    SDL_Cursor* hand_cursor;
    SDL_Cursor* arrow_cursor;

    bool handle_mouse_button(const SDL_Event& event);
    bool handle_mouse_motion(const SDL_Event& event);
    bool handle_keydown(const SDL_Event& event);
    void flush_pending_chat();

};

#endif
