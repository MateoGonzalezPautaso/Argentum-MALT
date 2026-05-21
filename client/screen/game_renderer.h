#ifndef CLIENT_GAME_RENDERER_H
#define CLIENT_GAME_RENDERER_H

#include <SDL2/SDL.h>
#include <SDL2pp/SDL2pp.hh>

#include "../../common/messages.h"
#include "../../common/queue.h"

#include "../input/chat_input.h"
#include "../config/config.h"
#include "../input/move_controller.h"
#include "../config/player_stats.h"
#include "../render/ui_renderer.h"
#include "../render/world_renderer.h"

class GameRenderer {
public:
    static constexpr int LOGICAL_W = 1024;
    static constexpr int LOGICAL_H = 768;

    GameRenderer(SDL2pp::Renderer& renderer, const ClientConfig& config,
                 Queue<ClientCommand>& command_queue);

    void tick();
    void render();
    void apply_server_event(const ServerEvent& ev);
    bool handle_event(const SDL_Event& event);
    bool is_chat_focused() const { return chat_input.is_focused(); }

private:
    SDL2pp::Renderer& renderer;
    ChatInput chat_input;
    PlayerStats player_stats;
    WorldRenderer world_renderer;
    UIRenderer ui_renderer;
    MoveController move_controller;
    MoveConfig move_config;

    bool handle_mouse_button(const SDL_Event& event);
    bool handle_keydown(const SDL_Event& event);
};

#endif
