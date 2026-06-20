#include "menu_controller.h"

MenuController::MenuController(SDL2pp::Renderer& renderer, const UIConfig& ui_cfg):
        menu_renderer(renderer, ui_cfg) {}

void MenuController::handle_mouse_motion(int x, int y) {
    menu_renderer.set_start_button_hovered(x, y);
    menu_renderer.set_audio_button_hovered(x, y);
}

bool MenuController::is_start_hit(int x, int y) const { return menu_renderer.is_start_hit(x, y); }

bool MenuController::is_audio_hit(int x, int y) const {
    return menu_renderer.is_audio_hit(x, y);
}

void MenuController::set_audio_muted(bool muted) { menu_renderer.set_audio_muted(muted); }

void MenuController::render() { menu_renderer.render(); }
