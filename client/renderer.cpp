#include "renderer.h"

#include <memory>
#include <vector>

#include <SDL2/SDL.h>

#include "menu_renderer.h"
#include "ui_renderer.h"
#include "world_renderer.h"


// - MenuRenderer: menu principal
// - WorldRenderer: tilemap, sprites, camara, animaciones
// - UIRenderer: overlays/UI del juego
ClientRenderer::ClientRenderer(SDL2pp::Window& window, const BackgroundConfig& background,
                               const TilemapConfig& tilemap,
                               const std::vector<SpriteConfig>& sprites_config, int window_w,
                               int window_h):
        renderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
        menu_renderer(std::make_unique<MenuRenderer>(renderer, window_w, window_h)),
        world_renderer(std::make_unique<WorldRenderer>(renderer, background, tilemap,
                                                       sprites_config, window_w, window_h)),
        ui_renderer(std::make_unique<UIRenderer>(renderer, window_w, window_h)) {}

ClientRenderer::~ClientRenderer() = default;

void ClientRenderer::render_frame() {
    renderer.SetDrawColor(0, 0, 0, 255);
    renderer.Clear();
    ui_renderer->render_frame_background();
    world_renderer->render();
    ui_renderer->render_chat_input();
    renderer.Present();
}

void ClientRenderer::render_menu() {
    // Render de pantalla de menu principal.
    menu_renderer->render();
}

bool ClientRenderer::is_menu_button_hit(int x, int y) const {
    return menu_renderer->is_button_hit(x, y);
}

bool ClientRenderer::move_sprite(int dx, int dy) { return world_renderer->move_sprite(dx, dy); }

void ClientRenderer::set_movable_position(int x, int y) {
    world_renderer->set_movable_position(x, y);
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

void ClientRenderer::set_chat_input_text(const std::string& text) {
    ui_renderer->set_chat_input_text(text);
}

void ClientRenderer::set_chat_input_focus(bool focused) {
    ui_renderer->set_chat_input_focus(focused);
}

bool ClientRenderer::is_chat_input_hit(int x, int y) const {
    return ui_renderer->is_chat_input_hit(x, y);
}

void ClientRenderer::set_movable_src_y(int y) { world_renderer->set_movable_src_y(y); }

void ClientRenderer::step_movable_src_x(int step, int frame_count) {
    world_renderer->step_movable_src_x(step, frame_count);
}

void ClientRenderer::set_anchor_src_y(int y) { world_renderer->set_anchor_src_y(y); }
