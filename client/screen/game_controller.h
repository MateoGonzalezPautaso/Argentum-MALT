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
    const ClientConfig& config;
    SDL2pp::Renderer& renderer;
    ChatInput chat_input;
    ChatHistory chat_history;
    PlayerStats player_stats;
    bool player_is_ghost = false;
    std::string current_map_name = "main";
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
    void apply_movement_visual(Direction dir, bool advance_frame);

    void handle_entity_move(const EntityMoveEvent& e);
    void handle_entity_spawn(const EntitySpawnEvent& e);
    void handle_entity_despawn(const EntityDespawnEvent& e);
    void handle_login_ok(const LoginOkEvent& e);
    void handle_damage_received(const DamageReceivedEvent& e);
    void handle_attack_dodged(const AttackDodgedEvent& e);
    void interact_with_prop(const std::string& prop_name);
    bool is_clickable_prop(const std::string& prop_name) const;
    bool is_transition_prop(const std::string& prop_name) const;
    void handle_chat_msg(const ChatMsgEvent& e);
    void handle_entity_died(const EntityDiedEvent& e);
    void handle_player_respawned(const PlayerRespawnedEvent& e);
    void handle_clan_notification(const ClanNotificationEvent& e);
    void handle_clan_update(const ClanUpdateEvent& e);
    void handle_map_transition(const MapTransitionEvent& e);

};

#endif
