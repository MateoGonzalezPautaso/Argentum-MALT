#include "menu_renderer.h"

#include <algorithm>

#include "texture_loader.h"

MenuRenderer::MenuRenderer(SDL2pp::Renderer& renderer, const UIConfig& ui_cfg):
        renderer(renderer),
        menu_background_texture(renderer, texture::load_surface(ui_cfg.asset_menu_bg)),
        start_button(SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_start_default)),
                     SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_start_hover))),
        settings_button(
                SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_settings_default)),
                SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_settings_hover))),
        menu_background_rect(0, 0, ui_cfg.window_w, ui_cfg.window_h),
        ui_cfg(ui_cfg) {
    init_layout();
}

void MenuRenderer::render() {
    renderer.SetDrawColor(0, 0, 0, 255);
    renderer.Clear();
    renderer.Copy(menu_background_texture, SDL2pp::NullOpt, menu_background_rect);
    settings_button.render(renderer);
    start_button.render(renderer);
    renderer.Present();
}

bool MenuRenderer::is_start_hit(int x, int y) const { return start_button.is_hit(x, y); }

bool MenuRenderer::is_settings_hit(int x, int y) const { return settings_button.is_hit(x, y); }

void MenuRenderer::set_start_button_hovered(int x, int y) {
    start_button.hovered = start_button.is_hit(x, y);
}

void MenuRenderer::set_settings_button_hovered(int x, int y) {
    settings_button.hovered = settings_button.is_hit(x, y);
}

void MenuRenderer::init_layout() {
    start_button.set_position(ui_cfg.start_x, ui_cfg.start_y, ui_cfg.start_w, ui_cfg.start_h);
    settings_button.set_position(ui_cfg.settings_x, ui_cfg.settings_y, ui_cfg.settings_w,
                                 ui_cfg.settings_h);
}
