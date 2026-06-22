#include "login_renderer.h"

#include <algorithm>

#include "../../input/chat_input.h"
#include "../../render/gfx/texture_loader.h"

LoginRenderer::LoginRenderer(SDL2pp::Renderer& renderer, const UIConfig& ui_cfg,
                             const ChatInput& username_model, const ChatInput& password_model):
        CredentialScreenRenderer(renderer, ui_cfg, username_model, password_model,
                                 ui_cfg.title_text),
        connect_button(
                SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_connect_default)),
                SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_connect_hover))),
        audio_button(
                SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_audio_default)),
                SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_audio_hover))),
        audio_off_texture(renderer, texture::load_surface(ui_cfg.asset_audio_off)),
        new_account_button(
                SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_new_account_default)),
                SDL2pp::Texture(renderer,
                                texture::load_surface(ui_cfg.asset_new_account_hover))) {
    init_layout();
}

void LoginRenderer::render() {
    render_chrome();
    connect_button.render(renderer);
    audio_button.render_togglable(renderer, audio_off_texture, audio_muted_);
    new_account_button.render(renderer);
    render_error(new_account_button.rect.GetY() + new_account_button.rect.GetH());
    renderer.Present();
}

bool LoginRenderer::is_connect_button_hit(int x, int y) const {
    return connect_button.is_hit(x, y);
}
bool LoginRenderer::is_audio_hit(int x, int y) const { return audio_button.is_hit(x, y); }
bool LoginRenderer::is_new_account_hit(int x, int y) const {
    return new_account_button.is_hit(x, y);
}

void LoginRenderer::set_connect_button_hovered(int x, int y) {
    connect_button.hovered = connect_button.is_hit(x, y);
}
void LoginRenderer::set_audio_button_hovered(int x, int y) {
    audio_button.hovered = audio_button.is_hit(x, y);
}
void LoginRenderer::set_new_account_button_hovered(int x, int y) {
    new_account_button.hovered = new_account_button.is_hit(x, y);
}

void LoginRenderer::init_layout() {
    const int password_y = password_field_rect.GetY();
    const int button_w = connect_button.default_tex.GetWidth();
    const int button_h = connect_button.default_tex.GetHeight();
    const int button_x = std::max(0, (ui_cfg.window_w - button_w) / 2);
    const int button_y = password_y + ui_cfg.login_field_h + ui_cfg.login_button_spacing;
    connect_button.set_position(button_x, button_y, button_w, button_h);

    const int audio_w = audio_button.default_tex.GetWidth();
    const int audio_h = audio_button.default_tex.GetHeight();
    audio_button.set_position(ui_cfg.back_button_x, ui_cfg.back_button_y, audio_w, audio_h);

    const int na_w = new_account_button.default_tex.GetWidth();
    const int na_h = new_account_button.default_tex.GetHeight();
    const int na_x = std::max(0, (ui_cfg.window_w - na_w) / 2);
    const int na_y = button_y + button_h + ui_cfg.error_spacing;
    new_account_button.set_position(na_x, na_y, na_w, na_h);
}
