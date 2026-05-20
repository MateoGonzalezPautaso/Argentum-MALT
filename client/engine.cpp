#include "engine.h"

#include <variant>

#include <SDL2/SDL.h>

#include "../common/visit.h"

ClientEngine::ClientEngine(const ClientConfig& config, Queue<ClientCommand>& command_queue):
        sdl(SDL_INIT_VIDEO | SDL_INIT_AUDIO),
        window(config.window.title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
               config.window.width, config.window.height, 0),
        sdl_renderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
        menu_renderer(sdl_renderer, LOGICAL_W, LOGICAL_H),
        login_renderer(sdl_renderer, LOGICAL_W, LOGICAL_H, login_username, login_password),
        world_renderer(sdl_renderer, config.background, config.tilemap, config.sprites, LOGICAL_W,
                       LOGICAL_H),
        ui_renderer(sdl_renderer, LOGICAL_W, LOGICAL_H, chat_input),
        move_controller(world_renderer, command_queue, MoveConfig(config), SDL_GetTicks()) {
    SDL_RenderSetLogicalSize(sdl_renderer.Get(), LOGICAL_W, LOGICAL_H);
    SDL_StartTextInput();
    login_username.set_focus(true);
}

ClientEngine::~ClientEngine() { SDL_StopTextInput(); }

void ClientEngine::show_sprite() { render_game_frame(); }

void ClientEngine::show_menu() { render_menu_frame(); }

void ClientEngine::show_login() { render_login_frame(); }

void ClientEngine::tick() { move_controller.tick(SDL_GetTicks()); }

const std::string& ClientEngine::login_username_text() const {
    return login_username.get_text();
}

const std::string& ClientEngine::login_password_text() const {
    return login_password.get_text();
}

void ClientEngine::apply_server_event(const ServerEvent& ev) {
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

bool ClientEngine::handle_event(const SDL_Event& event) {
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

void ClientEngine::handle_login_event(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
            login_submitted = true;
            return;
        }
        if (event.key.keysym.sym == SDLK_TAB) {
            login_active_field = (login_active_field + 1) % 2;
            login_username.set_focus(login_active_field == 0);
            login_password.set_focus(login_active_field == 1);
            return;
        }
    }

    if (login_active_field == 0) {
        login_username.consume_event(event);
    } else {
        login_password.consume_event(event);
    }

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        const int x = event.button.x;
        const int y = event.button.y;

        if (login_renderer.is_username_hit(x, y)) {
            login_active_field = 0;
            login_username.set_focus(true);
            login_password.set_focus(false);
        } else if (login_renderer.is_password_hit(x, y)) {
            login_active_field = 1;
            login_username.set_focus(false);
            login_password.set_focus(true);
        } else if (login_renderer.is_connect_button_hit(x, y)) {
            login_submitted = true;
        } else {
            login_username.set_focus(false);
            login_password.set_focus(false);
        }
    }
}

bool ClientEngine::is_menu_click(int x, int y) const { return menu_renderer.is_start_hit(x, y); }

bool ClientEngine::is_menu_settings_click(int x, int y) const {
    return menu_renderer.is_settings_hit(x, y);
}

bool ClientEngine::handle_mouse_button(const SDL_Event& event) {
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

bool ClientEngine::handle_keydown(const SDL_Event& event) {
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
        default:
            break;
    }

    return true;
}

void ClientEngine::render_game_frame() {
    sdl_renderer.SetDrawColor(0, 0, 0, 255);
    sdl_renderer.Clear();
    ui_renderer.render_frame_background();
    world_renderer.render();
    ui_renderer.render_hp_bar(player_stats.hp_current, player_stats.hp_max);
    ui_renderer.render_mp_bar(player_stats.mana_current, player_stats.mana_max);
    ui_renderer.render_chat_input();
    sdl_renderer.Present();
}

void ClientEngine::render_menu_frame() {
    sdl_renderer.SetDrawColor(0, 0, 0, 255);
    sdl_renderer.Clear();
    menu_renderer.render();
    sdl_renderer.Present();
}

void ClientEngine::render_login_frame() {
    sdl_renderer.SetDrawColor(0, 0, 0, 255);
    sdl_renderer.Clear();
    login_renderer.render();
    sdl_renderer.Present();
}

void ClientEngine::handle_menu_mouse_motion(int x, int y) {
    menu_renderer.set_start_button_hovered(x, y);
    menu_renderer.set_settings_button_hovered(x, y);
}

void ClientEngine::handle_login_mouse_motion(int x, int y) {
    login_renderer.set_connect_button_hovered(x, y);
    login_renderer.set_back_button_hovered(x, y);
}
