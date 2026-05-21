#include "menu_controller.h"

MenuController::MenuController(SDL2pp::Renderer& renderer, int logical_w, int logical_h):
        menu_renderer(renderer, logical_w, logical_h) {}

void MenuController::handle_mouse_motion(int x, int y) {
    menu_renderer.set_start_button_hovered(x, y);
    menu_renderer.set_settings_button_hovered(x, y);
}

bool MenuController::is_start_hit(int x, int y) const { return menu_renderer.is_start_hit(x, y); }

bool MenuController::is_settings_hit(int x, int y) const {
    return menu_renderer.is_settings_hit(x, y);
}

void MenuController::render() { menu_renderer.render(); }
