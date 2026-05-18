#include "engine.h"

#include <algorithm>
#include <cmath>

#include <SDL2/SDL.h>

ClientEngine::ClientEngine(const ClientConfig& config, ClientProtocol& protocol):
        sdl(SDL_INIT_VIDEO | SDL_INIT_AUDIO),
        window(config.window.title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, 0),
        renderer(window, config.background, config.tilemap, config.sprites, 1024, 768),
        protocol(protocol),
        last_walk_tick(SDL_GetTicks()),
        walk_frame_ms(config.walk_frame_ms),
        move_step(config.move_step),
        walk_src_step(config.walk_src_step),
        walk_src_frames(config.walk_src_frames),
        walk_src_frames_down(config.walk_src_frames_down),
        walk_src_frames_up(config.walk_src_frames_up),
        walk_src_frames_left(config.walk_src_frames_left),
        walk_src_frames_right(config.walk_src_frames_right),
        dir_src_y_down(config.dir_src_y_down),
        dir_src_y_up(config.dir_src_y_up),
        dir_src_y_left(config.dir_src_y_left),
        dir_src_y_right(config.dir_src_y_right),
        head_dir_src_y_down(config.head_dir_src_y_down),
        head_dir_src_y_up(config.head_dir_src_y_up),
        head_dir_src_y_left(config.head_dir_src_y_left),
        head_dir_src_y_right(config.head_dir_src_y_right) {
    SDL_StartTextInput();
    sync_chat_to_renderer();
}

ClientEngine::~ClientEngine() { SDL_StopTextInput(); }

void ClientEngine::show_sprite() { renderer.render_frame(); }

void ClientEngine::show_menu() { renderer.render_menu(); }

void ClientEngine::tick() {
    if (!has_target) {
        return;
    }
    move_toward_target(SDL_GetTicks());
}

void ClientEngine::move_sprite(int dx, int dy) { renderer.move_sprite(dx, dy); }

void ClientEngine::set_movable_src_y(int y) { renderer.set_movable_src_y(y); }

void ClientEngine::step_movable_src_x(int step, int frame_count) {
    renderer.step_movable_src_x(step, frame_count);
}

void ClientEngine::set_anchor_src_y(int y) { renderer.set_anchor_src_y(y); }

int ClientEngine::walk_src_frames_for(Direction dir) const {
    switch (dir) {
        case Direction::NORTH:
            return walk_src_frames_up;
        case Direction::SOUTH:
            return walk_src_frames_down;
        case Direction::WEST:
            return walk_src_frames_left;
        case Direction::EAST:
            return walk_src_frames_right;
        default:
            return walk_src_frames;
    }
}

void ClientEngine::set_move_target(int x, int y) {
    target_x = x;
    target_y = y;
    has_target = true;
}

void ClientEngine::apply_movement(Direction dir, int dx, int dy, uint32_t now, bool cancel_target) {
    // mapea direccion a filas del sprite de jugador y avanza animacion de caminata.
    if (cancel_target) {
        has_target = false;
    }

    set_direction_rows(dir);
    advance_walk_frame(dir, now);

    move_sprite(dx, dy);
    protocol.send_command(MoveCmd{dir});
}

void ClientEngine::set_direction_rows(Direction dir) {
    if (dir == Direction::NORTH) {
        set_movable_src_y(dir_src_y_up);
        set_anchor_src_y(head_dir_src_y_up);
    } else if (dir == Direction::SOUTH) {
        set_movable_src_y(dir_src_y_down);
        set_anchor_src_y(head_dir_src_y_down);
    } else if (dir == Direction::WEST) {
        set_movable_src_y(dir_src_y_left);
        set_anchor_src_y(head_dir_src_y_left);
    } else if (dir == Direction::EAST) {
        set_movable_src_y(dir_src_y_right);
        set_anchor_src_y(head_dir_src_y_right);
    }
}

void ClientEngine::advance_walk_frame(Direction dir, uint32_t now) {
    if (now - last_walk_tick < walk_frame_ms) {
        return;
    }
    step_movable_src_x(walk_src_step, walk_src_frames_for(dir));
    last_walk_tick = now;
}

bool ClientEngine::get_movable_position(int& x, int& y) {
    if (!renderer.get_movable_position(x, y)) {
        has_target = false;
        return false;
    }
    return true;
}

bool ClientEngine::should_stop_at_target(int current_x, int current_y, int new_x, int new_y) {
    if (new_x == target_x && new_y == target_y) {
        return true;
    }
    if (new_x == current_x && new_y == current_y) {
        return true;
    }
    return false;
}

void ClientEngine::compute_step_to_target(int current_x, int current_y, int& move_dx, int& move_dy,
                                          Direction& dir) const {
    const int dx = target_x - current_x;
    const int dy = target_y - current_y;
    move_dx = 0;
    move_dy = 0;
    dir = Direction::SOUTH;

    if (std::abs(dx) >= std::abs(dy)) {
        if (dx > 0) {
            move_dx = std::min(move_step, dx);
            dir = Direction::EAST;
        } else {
            move_dx = std::max(-move_step, dx);
            dir = Direction::WEST;
        }
    } else {
        if (dy > 0) {
            move_dy = std::min(move_step, dy);
            dir = Direction::SOUTH;
        } else {
            move_dy = std::max(-move_step, dy);
            dir = Direction::NORTH;
        }
    }
}

void ClientEngine::move_toward_target(uint32_t now) {
    // avanza al objetivo por el eje en cada tick
    int current_x = 0;
    int current_y = 0;
    if (!get_movable_position(current_x, current_y)) {
        return;
    }

    if (target_x == current_x && target_y == current_y) {
        has_target = false;
        return;
    }

    int move_dx = 0;
    int move_dy = 0;
    Direction dir = Direction::SOUTH;
    compute_step_to_target(current_x, current_y, move_dx, move_dy, dir);
    apply_movement(dir, move_dx, move_dy, now, false);

    int new_x = 0;
    int new_y = 0;
    if (!get_movable_position(new_x, new_y)) {
        return;
    }

    if (should_stop_at_target(current_x, current_y, new_x, new_y)) {
        has_target = false;
    }
}

bool ClientEngine::handle_event(const SDL_Event& event) {
    if (event.type == SDL_QUIT) {
        return false;
    }

    if (chat_input.consume_event(event)) {
        sync_chat_to_renderer();
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

bool ClientEngine::is_menu_click(int x, int y) const { return renderer.is_menu_button_hit(x, y); }

bool ClientEngine::handle_mouse_button(const SDL_Event& event) {
    if (event.button.button != SDL_BUTTON_LEFT) {
        return true;
    }

    if (chat_input.set_focus(renderer.is_chat_input_hit(event.button.x, event.button.y))) {
        sync_chat_to_renderer();
    }

    if (chat_input.is_focused()) {
        return true;
    }

    int world_x = 0;
    int world_y = 0;
    if (!renderer.screen_to_world(event.button.x, event.button.y, world_x, world_y)) {
        return true;
    }
    set_move_target(world_x, world_y);
    return true;
}

bool ClientEngine::handle_keydown(const SDL_Event& event) {
    const uint32_t now = SDL_GetTicks();

    switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
            return false;
        case SDLK_LEFT:
            apply_movement(Direction::WEST, -move_step, 0, now, true);
            break;
        case SDLK_RIGHT:
            apply_movement(Direction::EAST, move_step, 0, now, true);
            break;
        case SDLK_UP:
            apply_movement(Direction::NORTH, 0, -move_step, now, true);
            break;
        case SDLK_DOWN:
            apply_movement(Direction::SOUTH, 0, move_step, now, true);
            break;
        default:
            break;
    }

    return true;
}

void ClientEngine::sync_chat_to_renderer() {
    renderer.set_chat_input_text(chat_input.get_text());
    renderer.set_chat_input_focus(chat_input.is_focused());
}
