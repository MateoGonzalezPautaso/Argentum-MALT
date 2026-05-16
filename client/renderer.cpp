#include "renderer.h"

#include <memory>

#include <SDL2/SDL.h>

#include "menu_renderer.h"
#include "world_renderer.h"

ClientRenderer::ClientRenderer(SDL2pp::Window& window,
                               const BackgroundConfig& background,
                               const TilemapConfig& tilemap,
                               const std::vector<SpriteConfig>& sprites_config,
                               int window_w,
                               int window_h)
        : renderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
          menu_renderer(std::make_unique<MenuRenderer>(renderer, window_w, window_h)),
          world_renderer(std::make_unique<WorldRenderer>(
                  renderer, background, tilemap, sprites_config, window_w, window_h)) {}

ClientRenderer::~ClientRenderer() = default;

void ClientRenderer::render_frame() {
    world_renderer->render();
}

void ClientRenderer::render_menu() {
    menu_renderer->render();
}

bool ClientRenderer::is_menu_button_hit(int x, int y) const {
    return menu_renderer->is_button_hit(x, y);
}

void ClientRenderer::move_sprite(int dx, int dy) {
    world_renderer->move_sprite(dx, dy);
}

bool ClientRenderer::get_movable_position(int& x, int& y) const {
    return world_renderer->get_movable_position(x, y);
}

void ClientRenderer::get_camera_offset(int& x, int& y) const {
    world_renderer->get_camera_offset(x, y);
}

bool ClientRenderer::screen_to_world(int screen_x, int screen_y, int& world_x, int& world_y) const {
    return world_renderer->screen_to_world(screen_x, screen_y, world_x, world_y);
}

void ClientRenderer::set_movable_src_y(int y) {
    world_renderer->set_movable_src_y(y);
}

void ClientRenderer::step_movable_src_x(int step, int frame_count) {
    world_renderer->step_movable_src_x(step, frame_count);
}

void ClientRenderer::set_anchor_src_y(int y) {
    world_renderer->set_anchor_src_y(y);
}
