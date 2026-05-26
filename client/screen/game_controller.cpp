#include "game_controller.h"

#include <variant>

#include "../../common/visit.h"

GameController::GameController(SDL2pp::Renderer& renderer, const ClientConfig& config,
                               Queue<ClientCommand>& command_queue):
        renderer(renderer),
        world_renderer(renderer, config.background, config.tilemap, config.sprites, config.viewport,
                       config.font, config.skins),
        ui_renderer(renderer, config.ui, chat_input),
        command_queue(command_queue),
        move_controller(this->command_queue, MoveConfig(config), SDL_GetTicks()),
        move_config(config),
        hand_cursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND)),
        arrow_cursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW)) {}

GameController::~GameController() {
    SDL_FreeCursor(hand_cursor);
    SDL_FreeCursor(arrow_cursor);
}

void GameController::tick() {
    uint32_t now = SDL_GetTicks();
    if (auto dir = move_controller.tick(now)) {
        apply_movement_visual(*dir, true);
    }
}

void GameController::render() {
    renderer.SetDrawColor(0, 0, 0, 255);
    renderer.Clear();
    ui_renderer.render_frame_background();
    world_renderer.render();
    ui_renderer.render_hp_bar(player_stats.hp_current, player_stats.hp_max);
    ui_renderer.render_mp_bar(player_stats.mana_current, player_stats.mana_max);
    ui_renderer.render_exp_bar(player_stats.experience, player_stats.exp_to_next);
    ui_renderer.render_chat_history(chat_history.get_messages());
    ui_renderer.render_chat_input();
    renderer.Present();
}

void GameController::apply_server_event(const ServerEvent& ev) {
    std::visit(overloaded{
                       [this](const EntityMoveEvent& e) { handle_entity_move(e); },
                       [this](const EntitySpawnEvent& e) { handle_entity_spawn(e); },
                       [this](const LoginOkEvent& e) { handle_login_ok(e); },
                       [this](const EntityDespawnEvent& e) { handle_entity_despawn(e); },
                       [this](const DamageReceivedEvent& e) { handle_damage_received(e); },
                       [](const DamageDealtEvent&) {},
                       [this](const AttackDodgedEvent& e) { handle_attack_dodged(e); },
                       [this](const ChatMsgEvent& e) { handle_chat_msg(e); },
                       [this](const EntityDiedEvent& e) { handle_entity_died(e); },
                       [this](const PlayerRespawnedEvent& e) { handle_player_respawned(e); },
                       [this](const ClanNotificationEvent& e) { handle_clan_notification(e); },
                       [this](const ClanUpdateEvent& e) { handle_clan_update(e); },
                       [](const auto&) {},
               },
               ev);
}

void GameController::handle_entity_move(const EntityMoveEvent& e) {
    if (e.entity_id == player_stats.player_id) {
        move_controller.set_position(e.entity_pos.x, e.entity_pos.y);
        world_renderer.set_movable_position(e.entity_pos.x, e.entity_pos.y);
        return;
    }
    world_renderer.move_entity(e.entity_id, e.entity_pos.x, e.entity_pos.y);
    world_renderer.set_entity_src_y(e.entity_id, move_config.body_src_y_for(e.entity_dir),
                                    move_config.head_src_y_for(e.entity_dir));
    world_renderer.step_entity_src_x(e.entity_id, move_config.walk_src_step,
                                     move_config.walk_src_frames_for(e.entity_dir));
}

void GameController::handle_entity_spawn(const EntitySpawnEvent& e) {
    if (e.entity_id == player_stats.player_id) {
        move_controller.set_position(e.entity_pos.x, e.entity_pos.y);
        world_renderer.set_movable_position(e.entity_pos.x, e.entity_pos.y);
        return;
    }
    world_renderer.spawn_entity(e.entity_id, e.entity_pos.x, e.entity_pos.y, e.entity_name,
                                e.entity_race, e.entity_class);
}

void GameController::handle_login_ok(const LoginOkEvent& e) {
    player_stats.player_id = e.player_id;
    player_stats.username = e.username;
    player_stats.race = e.race;
    player_stats.player_class = e.player_class;
    player_stats.level = e.level;
    player_stats.experience = e.experience;
    player_stats.exp_to_next = e.exp_to_next;
    player_stats.hp_current = e.hp_current;
    player_stats.hp_max = e.hp_max;
    player_stats.mana_current = e.mana_current;
    player_stats.mana_max = e.mana_max;
    player_stats.gold = e.gold;
    player_stats.pos = e.pos;
    move_controller.set_position(e.pos.x, e.pos.y);
    world_renderer.set_movable_position(e.pos.x, e.pos.y);
    world_renderer.set_local_player_info(e.race, e.player_class);
}

void GameController::handle_entity_despawn(const EntityDespawnEvent& e) {
    world_renderer.despawn_entity(e.entity_id);
}

void GameController::handle_damage_received(const DamageReceivedEvent& e) {
    if (e.target_id != player_stats.player_id) {
        return;
    }
    player_stats.hp_current = e.hp_current;
    player_stats.hp_max = e.hp_max;
}

void GameController::handle_attack_dodged(const AttackDodgedEvent&) {
    chat_history.add_message(ChatMsgType::SYSTEM, "", "El ataque fue esquivado");
}

void GameController::handle_chat_msg(const ChatMsgEvent& e) {
    if (e.recipient_id != 0 && e.recipient_id != player_stats.player_id &&
        e.sender_id != player_stats.player_id) {
        return;
    }
    chat_history.add_message(e.type, e.sender_name, e.message);
}

void GameController::handle_clan_notification(const ClanNotificationEvent& e) {
    switch (e.type) {
        case ClanNotifType::MEMBER_ONLINE:
            chat_history.add_message(ChatMsgType::SYSTEM, "",
                                     "[Clan] " + e.username + " esta en linea");
            break;
        case ClanNotifType::MEMBER_OFFLINE:
            chat_history.add_message(ChatMsgType::SYSTEM, "",
                                     "[Clan] " + e.username + " se desconecto");
            break;
        case ClanNotifType::MEMBER_ATTACKED:
            chat_history.add_message(ChatMsgType::SYSTEM, "",
                                     "[Clan] " + e.username + " esta siendo atacado!");
            break;
        case ClanNotifType::JOIN_REQUEST:
            chat_history.add_message(ChatMsgType::SYSTEM, "",
                                     "[Clan] " + e.username + " quiere unirse al clan");
            break;
        case ClanNotifType::JOIN_ACCEPTED:
            chat_history.add_message(ChatMsgType::SYSTEM, "",
                                     "[Clan] Has sido aceptado en " + e.clan_name);
            break;
        case ClanNotifType::JOIN_REJECTED:
            chat_history.add_message(ChatMsgType::SYSTEM, "",
                                     "[Clan] Has sido rechazado de " + e.clan_name);
            break;
        case ClanNotifType::KICKED:
            chat_history.add_message(ChatMsgType::SYSTEM, "",
                                     "[Clan] Has sido expulsado de " + e.clan_name);
            break;
    }
}

void GameController::handle_clan_update(const ClanUpdateEvent& e) {
    std::string msg = "--- Clan: " + e.clan_name + " ---";
    for (const auto& m: e.members) {
        msg += "\n  " + m.username;
        if (m.is_founder)
            msg += " (fundador)";
        msg += m.is_online ? " [En linea]" : " [Desconectado]";
    }
    chat_history.add_message(ChatMsgType::SYSTEM, "", msg);
}

void GameController::handle_entity_died(const EntityDiedEvent& e) {
    world_renderer.set_entity_alpha(e.entity_id, 128);
    if (e.entity_id != player_stats.player_id) {
        return;
    }
    player_is_ghost = true;
    world_renderer.set_movable_alpha(128);
}

void GameController::handle_player_respawned(const PlayerRespawnedEvent& e) {
    world_renderer.set_entity_alpha(e.entity_id, 255);
    if (e.entity_id != player_stats.player_id) {
        return;
    }
    player_is_ghost = false;
    player_stats.hp_current = e.hp_current;
    player_stats.hp_max = e.hp_max;
    world_renderer.set_movable_alpha(255);
}

void GameController::apply_movement_visual(Direction dir, bool advance_frame) {
    world_renderer.set_movable_src_y(move_config.body_src_y_for(dir));
    world_renderer.set_anchor_src_y(move_config.head_src_y_for(dir));
    if (advance_frame) {
        world_renderer.step_movable_src_x(move_config.walk_src_step,
                                           move_config.walk_src_frames_for(dir));
    }
}

bool GameController::handle_event(const SDL_Event& event) {
    if (event.type == SDL_QUIT) {
        return false;
    }

    if (chat_input.consume_event(event)) {
        flush_pending_chat();
        return true;
    }

    if (event.type == SDL_MOUSEBUTTONDOWN) {
        return handle_mouse_button(event);
    }

    if (event.type == SDL_MOUSEMOTION) {
        return handle_mouse_motion(event);
    }

    if (event.type == SDL_KEYDOWN) {
        return handle_keydown(event);
    }

    return true;
}

bool GameController::handle_mouse_button(const SDL_Event& event) {
    if (event.button.button != SDL_BUTTON_LEFT) {
        return true;
    }

    chat_input.set_focus(ui_renderer.is_chat_input_hit(event.button.x, event.button.y));

    if (chat_input.is_focused()) {
        return true;
    }

    int world_x = 0;
    int world_y = 0;
    if (!world_renderer.screen_to_world(event.button.x, event.button.y, world_x, world_y)) {
        return true;
    }

    uint16_t entity_id = 0;
    if (player_is_ghost) {
        return true;
    }

    if (world_renderer.hit_test_entity(world_x, world_y, entity_id)) {
        command_queue.push(AttackCmd{entity_id});
        return true;
    }

    move_controller.set_move_target(world_x, world_y);
    return true;
}

bool GameController::handle_mouse_motion(const SDL_Event& event) {
    int world_x = 0;
    int world_y = 0;
    uint16_t entity_id = 0;
    if (world_renderer.screen_to_world(event.motion.x, event.motion.y, world_x, world_y) &&
        world_renderer.hit_test_entity(world_x, world_y, entity_id)) {
        SDL_SetCursor(hand_cursor);
    } else {
        SDL_SetCursor(arrow_cursor);
    }
    return true;
}

bool GameController::handle_keydown(const SDL_Event& event) {
    const uint32_t now = SDL_GetTicks();
    const bool ctrl = event.key.keysym.mod & KMOD_CTRL;

    switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
            return false;
        case SDLK_LEFT:
            apply_movement_visual(Direction::WEST,
                                  move_controller.move_direction(Direction::WEST, now).has_value());
            break;
        case SDLK_RIGHT:
            apply_movement_visual(Direction::EAST,
                                  move_controller.move_direction(Direction::EAST, now).has_value());
            break;
        case SDLK_UP:
            apply_movement_visual(Direction::NORTH,
                                  move_controller.move_direction(Direction::NORTH, now).has_value());
            break;
        case SDLK_DOWN:
            apply_movement_visual(Direction::SOUTH,
                                  move_controller.move_direction(Direction::SOUTH, now).has_value());
            break;
        case SDLK_h:
            if (ctrl)
                command_queue.push(CheatInfiniteHpCmd{});
            else
                world_renderer.set_show_hitboxes(!world_renderer.get_show_hitboxes());
            break;
        case SDLK_m:
            if (ctrl)
                command_queue.push(CheatInfiniteManaCmd{});
            break;
        case SDLK_k:
            if (ctrl)
                command_queue.push(CheatDieCmd{});
            break;
        case SDLK_v:
            if (ctrl)
                command_queue.push(CheatLevelUpCmd{});
            break;
        case SDLK_b:
            if (ctrl)
                command_queue.push(CheatLevelDownCmd{});
            break;
        default:
            break;
    }

    return true;
}

void GameController::flush_pending_chat() {
    if (!chat_input.has_pending_message()) {
        return;
    }
    std::string text = chat_input.pop_pending_message();

    command_queue.push(SendChatMsgCmd{std::move(text)});
}
