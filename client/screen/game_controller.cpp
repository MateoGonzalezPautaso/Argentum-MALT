#include "game_controller.h"

#include <variant>

#include "../../common/visit.h"

GameController::GameController(SDL2pp::Renderer& renderer, const ClientConfig& config,
                               Queue<ClientCommand>& command_queue):
        renderer(renderer),
        world_renderer(renderer, config.background, config.tilemap, config.sprites, config.viewport,
                       config.font),
        ui_renderer(renderer, config.ui, chat_input),
        command_queue(command_queue),
        move_controller(world_renderer, this->command_queue, MoveConfig(config), SDL_GetTicks()),
        move_config(config),
        hand_cursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND)),
        arrow_cursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW)) {}

GameController::~GameController() {
    SDL_FreeCursor(hand_cursor);
    SDL_FreeCursor(arrow_cursor);
}

void GameController::tick() { move_controller.tick(SDL_GetTicks()); }

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
                       [this](const EntityMoveEvent& e) {
                           if (e.entity_id == player_stats.player_id) {
                               world_renderer.set_movable_position(e.entity_pos.x, e.entity_pos.y);
                           } else {
                               world_renderer.move_entity(e.entity_id, e.entity_pos.x,
                                                          e.entity_pos.y);
                               world_renderer.set_entity_src_y(
                                       e.entity_id, move_config.body_src_y_for(e.entity_dir),
                                       move_config.head_src_y_for(e.entity_dir));
                               world_renderer.step_entity_src_x(
                                       e.entity_id, move_config.walk_src_step,
                                       move_config.walk_src_frames_for(e.entity_dir));
                           }
                       },
                       [this](const EntitySpawnEvent& e) {
                           if (e.entity_id == player_stats.player_id) {
                               world_renderer.set_movable_position(e.entity_pos.x, e.entity_pos.y);
                           } else {
                               world_renderer.spawn_entity(e.entity_id, e.entity_pos.x,
                                                           e.entity_pos.y, e.entity_name);
                           }
                       },
                       [this](const EntityDespawnEvent& e) {
                           world_renderer.despawn_entity(e.entity_id);
                       },
                       [this](const LoginOkEvent& e) {
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
                           world_renderer.set_movable_position(e.pos.x, e.pos.y);
                       },

                         [this](const DamageReceivedEvent& e) {
                             if (e.damage >= player_stats.hp_current) {
                                 player_stats.hp_current = 0;
                             } else {
                                 player_stats.hp_current -= e.damage;
                             }
                             chat_history.add_message(
                                     ChatMsgType::SYSTEM, "",
                                     "Recibiste " + std::to_string(e.damage) + " de daño");
                         },
                         [this](const DamageDealtEvent& e) {
                             chat_history.add_message(
                                     ChatMsgType::SYSTEM, "",
                                     "Hiciste " + std::to_string(e.damage) + " de daño");
                         },
                         [this](const AttackDodgedEvent&) {
                             chat_history.add_message(ChatMsgType::SYSTEM, "",
                                                      "El ataque fue esquivado");
                         },
                         [this](const ChatMsgEvent& e) {
                             chat_history.add_message(e.type, e.sender_name, e.message);
                         },
                         [this](const EntityDiedEvent& e) {
                             world_renderer.set_entity_alpha(e.entity_id, 128);
                             if (e.entity_id == player_stats.player_id) {
                                 player_is_ghost = true;
                             }
                         },
                         [this](const PlayerRespawnedEvent& e) {
                             world_renderer.set_entity_alpha(e.entity_id, 255);
                             if (e.entity_id == player_stats.player_id) {
                                 player_is_ghost = false;
                             }
                         },
                         [](const auto&) {},
                },
                ev);

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

    switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
            return false;
        case SDLK_LEFT:
            move_controller.move_direction(Direction::WEST, now);
            break;
        case SDLK_RIGHT:
            move_controller.move_direction(Direction::EAST, now);
            break;
        case SDLK_UP:
            move_controller.move_direction(Direction::NORTH, now);
            break;
        case SDLK_DOWN:
            move_controller.move_direction(Direction::SOUTH, now);
            break;
        case SDLK_h:
            world_renderer.set_show_hitboxes(!world_renderer.get_show_hitboxes());
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
