#include "credential_screen_renderer.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "../../input/chat_input.h"
#include "../../render/gfx/geometry.h"
#include "../../render/gfx/texture_loader.h"

CredentialScreenRenderer::CredentialScreenRenderer(SDL2pp::Renderer& renderer,
                                                   const UIConfig& ui_cfg,
                                                   const ChatInput& username_model,
                                                   const ChatInput& password_model,
                                                   std::string title_text):
        renderer(renderer),
        username_model(username_model),
        password_model(password_model),
        background_texture(renderer, texture::load_surface(ui_cfg.asset_login_bg)),
        logo_texture(renderer, texture::load_surface(ui_cfg.asset_login_logo)),
        background_rect(0, 0, ui_cfg.window_w, ui_cfg.window_h),
        ui_cfg(ui_cfg),
        title_text_(std::move(title_text)) {
    field_font = TTF_OpenFont(ui_cfg.font_path.c_str(), ui_cfg.font_field_size);
    if (!field_font)
        throw std::runtime_error(std::string("TTF_OpenFont failed: ") + TTF_GetError());
    title_font = TTF_OpenFont(ui_cfg.font_path.c_str(), ui_cfg.font_title_size);
    if (!title_font)
        throw std::runtime_error(std::string("TTF_OpenFont failed: ") + TTF_GetError());
    init_shared_layout();
    form_widgets_.emplace(renderer, field_font, ui_cfg);
}

CredentialScreenRenderer::~CredentialScreenRenderer() {
    if (field_font) {
        TTF_CloseFont(field_font);
        field_font = nullptr;
    }
    if (title_font) {
        TTF_CloseFont(title_font);
        title_font = nullptr;
    }
}

void CredentialScreenRenderer::init_shared_layout() {
    const int logo_w = logo_texture.GetWidth();
    const int logo_h = logo_texture.GetHeight();
    const int logo_x = std::max(0, (ui_cfg.window_w - logo_w) / 2);
    const int logo_y = ui_cfg.login_logo_y;
    logo_rect = SDL2pp::Rect(logo_x, logo_y, logo_w, logo_h);

    int title_w = 0, title_h = 0;
    if (title_font)
        TTF_SizeUTF8(title_font, title_text_.c_str(), &title_w, &title_h);
    const int title_x = std::max(0, (ui_cfg.window_w - title_w) / 2);
    const int title_y = logo_y + logo_h + ui_cfg.login_title_spacing;
    title_rect = SDL2pp::Rect(title_x, title_y, title_w, title_h);

    const int field_x = std::max(0, (ui_cfg.window_w - ui_cfg.login_field_w) / 2);
    const int field_start_y = title_y + title_h + ui_cfg.login_field_spacing;
    username_field_rect =
            SDL2pp::Rect(field_x, field_start_y, ui_cfg.login_field_w, ui_cfg.login_field_h);
    const int password_y = field_start_y + ui_cfg.login_field_h + ui_cfg.login_field_gap;
    password_field_rect =
            SDL2pp::Rect(field_x, password_y, ui_cfg.login_field_w, ui_cfg.login_field_h);
}

void CredentialScreenRenderer::render_chrome() {
    renderer.SetDrawColor(0, 0, 0, 255);
    renderer.Clear();
    renderer.Copy(background_texture, SDL2pp::NullOpt, background_rect);
    renderer.Copy(logo_texture, SDL2pp::NullOpt, logo_rect);

    SDL_Surface* title_surface =
            TTF_RenderUTF8_Blended(title_font, title_text_.c_str(), title_color);
    if (title_surface) {
        SDL2pp::Surface wrapped(title_surface);
        SDL2pp::Texture title_texture(renderer, wrapped);
        renderer.Copy(title_texture, SDL2pp::NullOpt, title_rect);
    }

    render_text_field(username_field_rect, username_model.get_text(), username_model.is_focused(),
                      ui_cfg.placeholder_username);
    render_text_field(password_field_rect, password_model.get_text(), password_model.is_focused(),
                      ui_cfg.placeholder_password);
}

void CredentialScreenRenderer::render_text_field(const SDL2pp::Rect& rect, const std::string& text,
                                                 bool focused,
                                                 const std::string& placeholder) const {
    form_widgets_->render_text_field(rect, text, focused, placeholder);
}

void CredentialScreenRenderer::render_error(int below_y) const {
    form_widgets_->render_error_centered(error_text, error_color, ui_cfg.window_w, below_y,
                                         ui_cfg.error_spacing);
}

void CredentialScreenRenderer::set_error(const std::string& text) { error_text = text; }
void CredentialScreenRenderer::clear_error() { error_text.clear(); }

bool CredentialScreenRenderer::is_username_hit(int x, int y) const {
    return point_in_rect(x, y, username_field_rect);
}
bool CredentialScreenRenderer::is_password_hit(int x, int y) const {
    return point_in_rect(x, y, password_field_rect);
}
