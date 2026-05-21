#include "game_renderer.h"

#include <variant>

#include "../common/visit.h"

GameRenderer::GameRenderer(SDL2pp::Renderer& renderer, const ClientConfig& config,
                           Queue<ClientCommand>& command_queue):
        renderer(renderer),
        world_renderer(renderer, config.background, config.tilemap, config.sprites, LOGICAL_W,
                       LOGICAL_H),
        ui_renderer(renderer, LOGICAL_W, LOGICAL_H, chat_input),
        move_controller(world_renderer, command_queue, MoveConfig(config), SDL_GetTicks()) {}

void GameRenderer::tick() { move_controller.tick(SDL_GetTicks()); }

void GameRenderer::render() {
    renderer.SetDrawColor(0, 0, 0, 255);
    renderer.Clear();
    ui_renderer.render_frame_background();
    world_renderer.render();
    ui_renderer.render_hp_bar(player_stats.hp_current, player_stats.hp_max);
    ui_renderer.render_mp_bar(player_stats.mana_current, player_stats.mana_max);
    ui_renderer.render_exp_bar(player_stats.experience, player_stats.exp_to_next);
    ui_renderer.render_chat_input();
    renderer.Present();
}

void GameRenderer::apply_server_event(const ServerEvent& ev) {
    std::visit(overloaded{
                       [this](const EntityMoveEvent& e) {
                           world_renderer.set_movable_position(e.entity_pos.x, e.entity_pos.y);
                       },
                       [this](const EntitySpawnEvent& e) {
                           world_renderer.set_movable_position(e.entity_pos.x, e.entity_pos.y);
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
                       [](const auto&) {},
               },
               ev);
}

bool GameRenderer::handle_event(const SDL_Event& event) {
    if (event.type == SDL_QUIT) {
        return false;
    }

    if (chat_input.consume_event(event)) {
        return true;
    }

    if (event.type == SDL_MOUSEBUTTONDOWN) {
        return handle_mouse_button(event);
    }

    if (event.type == SDL_KEYDOWN) {
        return handle_keydown(event);
    }

    return true;
}

bool GameRenderer::handle_mouse_button(const SDL_Event& event) {
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
    move_controller.set_move_target(world_x, world_y);
    return true;
}

bool GameRenderer::handle_keydown(const SDL_Event& event) {
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
