#include "menu_renderer.h"

#include "../../render/gfx/texture_loader.h"

MenuRenderer::MenuRenderer(SDL2pp::Renderer& renderer, const UIConfig& ui_cfg):
        renderer(renderer),
        menu_background_texture(renderer, texture::load_surface(ui_cfg.asset_menu_bg)),
        start_button(SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_start_default)),
                     SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_start_hover))),
        audio_button(SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_audio_default)),
                     SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_audio_hover))),
        audio_off_texture(renderer, texture::load_surface(ui_cfg.asset_audio_off)),
        menu_background_rect(0, 0, ui_cfg.window_w, ui_cfg.window_h),
        ui_cfg(ui_cfg) {
    init_layout();
}

void MenuRenderer::render() {
    renderer.SetDrawColor(0, 0, 0, 255);
    renderer.Clear();
    renderer.Copy(menu_background_texture, SDL2pp::NullOpt, menu_background_rect);
    audio_button.render_togglable(renderer, audio_off_texture, audio_muted_);
    start_button.render(renderer);
    renderer.Present();
}

bool MenuRenderer::is_start_hit(int x, int y) const { return start_button.is_hit(x, y); }

bool MenuRenderer::is_audio_hit(int x, int y) const { return audio_button.is_hit(x, y); }

void MenuRenderer::set_start_button_hovered(int x, int y) {
    start_button.hovered = start_button.is_hit(x, y);
}

void MenuRenderer::set_audio_button_hovered(int x, int y) {
    audio_button.hovered = audio_button.is_hit(x, y);
}

void MenuRenderer::init_layout() {
    start_button.set_position(ui_cfg.start_x, ui_cfg.start_y, ui_cfg.start_w, ui_cfg.start_h);
    audio_button.set_position(ui_cfg.audio_x, ui_cfg.audio_y, ui_cfg.audio_w, ui_cfg.audio_h);
}
