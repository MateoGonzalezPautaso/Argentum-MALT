#include "engine.h"

#include <SDL2/SDL.h>

ClientEngine::ClientEngine(const ClientConfig& config, ClientProtocol& protocol)
        : sdl(SDL_INIT_VIDEO | SDL_INIT_AUDIO),
            window(config.window.title,
                    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                    config.window.width, config.window.height,
                    SDL_WINDOW_RESIZABLE),
        renderer(window, config.background, config.sprites, config.window.width, config.window.height),
        protocol(protocol),
        last_walk_tick(SDL_GetTicks()),
        walk_frame_ms(config.walk_frame_ms),
        move_step(config.move_step),
        walk_src_step(config.walk_src_step),
    walk_src_max(config.walk_src_max),
    dir_src_y_down(config.dir_src_y_down),
    dir_src_y_up(config.dir_src_y_up),
    dir_src_y_left(config.dir_src_y_left),
    dir_src_y_right(config.dir_src_y_right) {}

void ClientEngine::show_sprite() {
    renderer.render_frame();
}

void ClientEngine::move_sprite(int dx, int dy) {
    renderer.move_sprite(dx, dy);
}

void ClientEngine::toggle_extra_sprite() {
    renderer.toggle_extra_sprite();
}

void ClientEngine::set_movable_src_y(int y) {
    renderer.set_movable_src_y(y);
}

void ClientEngine::step_movable_src_x(int step, int max_x) {
    renderer.step_movable_src_x(step, max_x);
}

bool ClientEngine::handle_event(const SDL_Event& event) {
    if (event.type == SDL_QUIT) {
        return false;
    }

    if (event.type != SDL_KEYDOWN) {
        return true;
    }

    const uint32_t now = SDL_GetTicks();

    switch (event.key.keysym.sym) {
    case SDLK_ESCAPE:
        return false;
    case SDLK_LEFT:
        set_movable_src_y(dir_src_y_left);
        if (now - last_walk_tick >= walk_frame_ms) {
            step_movable_src_x(walk_src_step, walk_src_max);
            last_walk_tick = now;
        }
        move_sprite(-move_step, 0);
        protocol.send_command(MoveCmd{Direction::WEST});
        break;
    case SDLK_RIGHT:
        set_movable_src_y(dir_src_y_right);
        if (now - last_walk_tick >= walk_frame_ms) {
            step_movable_src_x(walk_src_step, walk_src_max);
            last_walk_tick = now;
        }
        move_sprite(move_step, 0);
        protocol.send_command(MoveCmd{Direction::EAST});
        break;
    case SDLK_UP:
        set_movable_src_y(dir_src_y_up);
        if (now - last_walk_tick >= walk_frame_ms) {
            step_movable_src_x(walk_src_step, walk_src_max);
            last_walk_tick = now;
        }
        move_sprite(0, -move_step);
        protocol.send_command(MoveCmd{Direction::NORTH});
        break;
    case SDLK_DOWN:
        set_movable_src_y(dir_src_y_down);
        if (now - last_walk_tick >= walk_frame_ms) {
            step_movable_src_x(walk_src_step, walk_src_max);
            last_walk_tick = now;
        }
        move_sprite(0, move_step);
        protocol.send_command(MoveCmd{Direction::SOUTH});
        break;
    case SDLK_p:
        toggle_extra_sprite();
        break;
    default:
        break;
    }

    return true;
}
