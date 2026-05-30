#include "login_renderer.h"

#include <algorithm>

#include "../input/chat_input.h"

#include "geometry.h"
#include "text_renderer.h"
#include "texture_loader.h"

LoginRenderer::LoginRenderer(SDL2pp::Renderer& renderer, const UIConfig& ui_cfg,
                             const ChatInput& username_model, const ChatInput& password_model):
        renderer(renderer),
        username_model(username_model),
        password_model(password_model),
        background_texture(renderer, texture::load_surface(ui_cfg.asset_login_bg)),
        logo_texture(renderer, texture::load_surface(ui_cfg.asset_login_logo)),
        connect_button(
                SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_connect_default)),
                SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_connect_hover))),
        back_button(SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_back_default)),
                    SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_back_hover))),
        new_account_button(
                SDL2pp::Texture(renderer,
                                texture::load_surface(ui_cfg.asset_new_account_default)),
                SDL2pp::Texture(renderer, texture::load_surface(ui_cfg.asset_new_account_hover))),
        background_rect(0, 0, ui_cfg.window_w, ui_cfg.window_h),
        logo_rect(0, 0, 0, 0),
        title_rect(0, 0, 0, 0),
        ui_cfg(ui_cfg) {
    field_font = TTF_OpenFont(ui_cfg.font_path.c_str(), ui_cfg.font_field_size);
    if (!field_font) {
        throw std::runtime_error(std::string("TTF_OpenFont failed: ") + TTF_GetError());
    }
    title_font = TTF_OpenFont(ui_cfg.font_path.c_str(), ui_cfg.font_title_size);
    if (!title_font) {
        throw std::runtime_error(std::string("TTF_OpenFont failed: ") + TTF_GetError());
    }
    init_layout();
}

LoginRenderer::~LoginRenderer() {
    if (field_font) {
        TTF_CloseFont(field_font);
        field_font = nullptr;
    }
    if (title_font) {
        TTF_CloseFont(title_font);
        title_font = nullptr;
    }
}

void LoginRenderer::render() {
    renderer.SetDrawColor(0, 0, 0, 255);
    renderer.Clear();
    renderer.Copy(background_texture, SDL2pp::NullOpt, background_rect);

    renderer.Copy(logo_texture, SDL2pp::NullOpt, logo_rect);

    SDL_Surface* title_surface =
            TTF_RenderUTF8_Blended(title_font, ui_cfg.title_text.c_str(), title_color);
    if (title_surface) {
        SDL2pp::Surface wrapped(title_surface);
        SDL2pp::Texture title_texture(renderer, wrapped);
        renderer.Copy(title_texture, SDL2pp::NullOpt, title_rect);
    }

    render_text_field(username_field_rect, username_model.get_text(), username_model.is_focused(),
                      ui_cfg.placeholder_username);
    render_text_field(password_field_rect, password_model.get_text(), password_model.is_focused(),
                      ui_cfg.placeholder_password);

    connect_button.render(renderer);
    back_button.render(renderer);
    new_account_button.render(renderer);

    render_error();

    renderer.Present();
}

bool LoginRenderer::is_username_hit(int x, int y) const {
    return point_in_rect(x, y, username_field_rect);
}

bool LoginRenderer::is_password_hit(int x, int y) const {
    return point_in_rect(x, y, password_field_rect);
}

bool LoginRenderer::is_connect_button_hit(int x, int y) const {
    return connect_button.is_hit(x, y);
}

bool LoginRenderer::is_back_button_hit(int x, int y) const { return back_button.is_hit(x, y); }

bool LoginRenderer::is_new_account_hit(int x, int y) const {
    return new_account_button.is_hit(x, y);
}

void LoginRenderer::set_new_account_button_hovered(int x, int y) {
    new_account_button.hovered = new_account_button.is_hit(x, y);
}

void LoginRenderer::set_connect_button_hovered(int x, int y) {
    connect_button.hovered = connect_button.is_hit(x, y);
}

void LoginRenderer::set_back_button_hovered(int x, int y) {
    back_button.hovered = back_button.is_hit(x, y);
}

void LoginRenderer::init_layout() {
    const int logo_w = logo_texture.GetWidth();
    const int logo_h = logo_texture.GetHeight();
    const int logo_x = std::max(0, (ui_cfg.window_w - logo_w) / 2);
    const int logo_y = ui_cfg.login_logo_y;
    logo_rect = SDL2pp::Rect(logo_x, logo_y, logo_w, logo_h);

    int title_w = 0;
    int title_h = 0;
    if (title_font) {
        TTF_SizeUTF8(title_font, ui_cfg.title_text.c_str(), &title_w, &title_h);
    }
    const int title_x = std::max(0, (ui_cfg.window_w - title_w) / 2);
    const int title_y = logo_y + logo_h + ui_cfg.login_title_spacing;
    title_rect = SDL2pp::Rect(title_x, title_y, title_w, title_h);

    const int field_x = std::max(0, (ui_cfg.window_w - ui_cfg.login_field_w) / 2);
    const int field_start_y = title_y + title_h + ui_cfg.login_field_spacing;
    const int username_y = field_start_y;
    const int password_y = username_y + ui_cfg.login_field_h + ui_cfg.login_field_gap;

    username_field_rect =
            SDL2pp::Rect(field_x, username_y, ui_cfg.login_field_w, ui_cfg.login_field_h);
    password_field_rect =
            SDL2pp::Rect(field_x, password_y, ui_cfg.login_field_w, ui_cfg.login_field_h);

    const int button_w = connect_button.default_tex.GetWidth();
    const int button_h = connect_button.default_tex.GetHeight();
    const int button_x = std::max(0, (ui_cfg.window_w - button_w) / 2);
    const int button_y = password_y + ui_cfg.login_field_h + ui_cfg.login_button_spacing;
    connect_button.set_position(button_x, button_y, button_w, button_h);

    const int back_w = back_button.default_tex.GetWidth();
    const int back_h = back_button.default_tex.GetHeight();
    back_button.set_position(ui_cfg.back_button_x, ui_cfg.back_button_y, back_w, back_h);

    const int na_w = new_account_button.default_tex.GetWidth();
    const int na_h = new_account_button.default_tex.GetHeight();
    const int na_x = std::max(0, (ui_cfg.window_w - na_w) / 2);
    const int na_y = button_y + button_h + ui_cfg.error_spacing;
    new_account_button.set_position(na_x, na_y, na_w, na_h);
}

void LoginRenderer::render_text_field(const SDL2pp::Rect& rect, const std::string& text,
                                      bool focused, const std::string& placeholder) const {
    if (!field_font) {
        return;
    }

    if (focused) {
        renderer.SetDrawColor(45, 45, 45, 220);
    } else {
        renderer.SetDrawColor(30, 30, 30, 220);
    }
    renderer.FillRect(rect);

    if (focused) {
        renderer.SetDrawColor(200, 180, 80, 255);
    } else {
        renderer.SetDrawColor(100, 100, 100, 255);
    }
    renderer.DrawRect(rect);

    int clipped_w = 0;

    if (!text.empty()) {
        render_text_in_rect(rect, text, text_color, clipped_w);
    } else if (!focused && !placeholder.empty()) {
        render_text_in_rect(rect, placeholder, placeholder_color, clipped_w);
    }

    if (focused && (SDL_GetTicks() / ui_cfg.cursor_blink_ms) % 2U == 0U) {
        const int cursor_x = rect.GetX() + ui_cfg.cursor_padding + clipped_w;
        renderer.SetDrawColor(255, 255, 255, 255);
        SDL2pp::Rect cursor_rect(cursor_x, rect.GetY() + ui_cfg.cursor_padding,
                                 ui_cfg.cursor_width, rect.GetH() - ui_cfg.cursor_height_shrink);
        renderer.FillRect(cursor_rect);
    }
}

void LoginRenderer::render_text_in_rect(const SDL2pp::Rect& rect, const std::string& text,
                                        SDL_Color color, int& clipped_w) const {
    clipped_w = texture::render_text_clipped(renderer, field_font, text, color, rect, 4, 2, true);
}

void LoginRenderer::set_error(const std::string& text) { error_text = text; }

void LoginRenderer::clear_error() { error_text.clear(); }

void LoginRenderer::render_error() const {
    if (error_text.empty() || !field_font) {
        return;
    }

    auto result = texture::render_text(renderer, field_font, error_text, error_color);
    if (result.w == 0) {
        return;
    }

    const int x = std::max(0, (ui_cfg.window_w - result.w) / 2);
    const int y = new_account_button.rect.GetY() + new_account_button.rect.GetH() + ui_cfg.error_spacing;
    SDL2pp::Rect dst(x, y, result.w, result.h);
    renderer.Copy(result.texture, SDL2pp::NullOpt, dst);
}
