#include "engine.h"

#include <variant>

#include <SDL2/SDL.h>

template <class... Ts>
struct overloaded: Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

ClientEngine::ClientEngine(const ClientConfig& config, ClientProtocol& protocol):
        sdl(SDL_INIT_VIDEO | SDL_INIT_AUDIO),
        window(config.window.title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, 0),
        sdl_renderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
        menu_renderer(sdl_renderer, 1024, 768),
        world_renderer(sdl_renderer, config.background, config.tilemap, config.sprites, 1024, 768),
        ui_renderer(sdl_renderer, 1024, 768, chat_input),
        protocol(protocol),
        move_controller(world_renderer, protocol,
                        MoveConfig{
                                .move_step = config.move_step,
                                .walk_src_step = config.walk_src_step,
                                .walk_src_frames = config.walk_src_frames,
                                .walk_src_frames_down = config.walk_src_frames_down,
                                .walk_src_frames_up = config.walk_src_frames_up,
                                .walk_src_frames_left = config.walk_src_frames_left,
                                .walk_src_frames_right = config.walk_src_frames_right,
                                .walk_frame_ms = config.walk_frame_ms,
                                .dir_src_y_down = config.dir_src_y_down,
                                .dir_src_y_up = config.dir_src_y_up,
                                .dir_src_y_left = config.dir_src_y_left,
                                .dir_src_y_right = config.dir_src_y_right,
                                .head_dir_src_y_down = config.head_dir_src_y_down,
                                .head_dir_src_y_up = config.head_dir_src_y_up,
                                .head_dir_src_y_left = config.head_dir_src_y_left,
                                .head_dir_src_y_right = config.head_dir_src_y_right,
                        },
                        SDL_GetTicks()) {
    SDL_StartTextInput();
}

ClientEngine::~ClientEngine() { SDL_StopTextInput(); }

void ClientEngine::show_sprite() { render_game_frame(); }

void ClientEngine::show_menu() { render_menu_frame(); }

void ClientEngine::tick() { move_controller.tick(SDL_GetTicks()); }

void ClientEngine::apply_server_event(const ServerEvent& ev) {
    std::visit(overloaded{
                       [this](const EntityMoveEvent& e) {
                           world_renderer.set_movable_position(e.entity_pos.x, e.entity_pos.y);
                       },
                       [this](const EntitySpawnEvent& e) {
                           world_renderer.set_movable_position(e.entity_pos.x, e.entity_pos.y);
                       },
                       [this](const LoginOkEvent& e) {
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

bool ClientEngine::is_menu_click(int x, int y) const {
    return menu_renderer.is_button_hit(x, y);
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
    ui_renderer.render_chat_input();
    sdl_renderer.Present();
}

void ClientEngine::render_menu_frame() {
    sdl_renderer.SetDrawColor(0, 0, 0, 255);
    sdl_renderer.Clear();
    menu_renderer.render();
    sdl_renderer.Present();
}
