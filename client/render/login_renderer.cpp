#include "login_renderer.h"

#include <algorithm>

#include "../chat_input.h"
#include "texture_loader.h"

LoginRenderer::LoginRenderer(SDL2pp::Renderer& renderer, int window_w, int window_h,
                             const ChatInput& username_model, const ChatInput& password_model):
        renderer(renderer),
        username_model(username_model),
        password_model(password_model),
        background_texture(renderer,
                           load_surface("assets/BabelUI/static/media/leather_brown..png")),
        connect_button_texture(renderer,
                               load_surface("assets/interface/en_boton-conectar-default.bmp")),
        back_button_texture(renderer,
                            load_surface("assets/interface/en_boton-volver-default.bmp")),
        background_rect(0, 0, window_w, window_h),
        connect_button_rect(0, 0, 0, 0),
        back_button_rect(0, 0, 0, 0),
        window_w(window_w),
        window_h(window_h) {
    field_font = TTF_OpenFont("assets/OUTPUT/Cardo.ttf", 18);
    if (!field_font) {
        throw std::runtime_error(std::string("TTF_OpenFont failed: ") + TTF_GetError());
    }
    init_layout();
}

LoginRenderer::~LoginRenderer() {
    if (field_font) {
        TTF_CloseFont(field_font);
        field_font = nullptr;
    }
}

void LoginRenderer::render() {
    renderer.Copy(background_texture, SDL2pp::NullOpt, background_rect);

    render_text_field(username_field_rect, username_model.get_text(),
                      username_model.is_focused());
    render_text_field(password_field_rect, password_model.get_text(),
                      password_model.is_focused());

    renderer.Copy(connect_button_texture, SDL2pp::NullOpt, connect_button_rect);
    renderer.Copy(back_button_texture, SDL2pp::NullOpt, back_button_rect);
}

bool LoginRenderer::is_username_hit(int x, int y) const {
    const int left = username_field_rect.GetX();
    const int top = username_field_rect.GetY();
    return x >= left && x <= left + username_field_rect.GetW() &&
           y >= top && y <= top + username_field_rect.GetH();
}

bool LoginRenderer::is_password_hit(int x, int y) const {
    const int left = password_field_rect.GetX();
    const int top = password_field_rect.GetY();
    return x >= left && x <= left + password_field_rect.GetW() &&
           y >= top && y <= top + password_field_rect.GetH();
}

bool LoginRenderer::is_connect_button_hit(int x, int y) const {
    const int left = connect_button_rect.GetX();
    const int top = connect_button_rect.GetY();
    return x >= left && x <= left + connect_button_rect.GetW() &&
           y >= top && y <= top + connect_button_rect.GetH();
}

bool LoginRenderer::is_back_button_hit(int x, int y) const {
    const int left = back_button_rect.GetX();
    const int top = back_button_rect.GetY();
    return x >= left && x <= left + back_button_rect.GetW() &&
           y >= top && y <= top + back_button_rect.GetH();
}

void LoginRenderer::init_layout() {
    const int field_w = 320;
    const int field_h = 34;
    const int field_x = std::max(0, (window_w - field_w) / 2);
    const int username_y = window_h / 2 - 80;
    const int password_y = window_h / 2 - 20;

    username_field_rect = SDL2pp::Rect(field_x, username_y, field_w, field_h);
    password_field_rect = SDL2pp::Rect(field_x, password_y, field_w, field_h);

    const int button_w = connect_button_texture.GetWidth();
    const int button_h = connect_button_texture.GetHeight();
    const int button_x = std::max(0, (window_w - button_w) / 2);
    const int button_y = password_y + field_h + 30;
    connect_button_rect = SDL2pp::Rect(button_x, button_y, button_w, button_h);

    const int back_w = back_button_texture.GetWidth();
    const int back_h = back_button_texture.GetHeight();
    back_button_rect = SDL2pp::Rect(10, 10, back_w, back_h);
}

void LoginRenderer::render_text_field(const SDL2pp::Rect& rect, const std::string& text,
                                      bool focused) const {
    if (!field_font) {
        return;
    }

    // field background
    renderer.SetDrawColor(30, 30, 30, 220);
    renderer.FillRect(rect);
    renderer.SetDrawColor(100, 100, 100, 255);
    renderer.DrawRect(rect);

    if (text.empty()) {
        return;
    }

    SDL_Surface* text_surface = TTF_RenderUTF8_Blended(field_font, text.c_str(), text_color);
    if (!text_surface) {
        return;
    }
    SDL2pp::Surface wrapped(text_surface);

    int text_w = 0;
    int text_h = 0;
    if (TTF_SizeUTF8(field_font, text.c_str(), &text_w, &text_h) != 0) {
        return;
    }

    SDL2pp::Texture text_texture(renderer, wrapped);
    const int clipped_w = std::min(text_w, rect.GetW() - 8);
    const int clipped_h = std::min(text_h, rect.GetH() - 4);
    SDL2pp::Rect src_rect(0, 0, clipped_w, clipped_h);
    SDL2pp::Rect dst_rect(rect.GetX() + 4, rect.GetY() + (rect.GetH() - clipped_h) / 2,
                          clipped_w, clipped_h);
    renderer.Copy(text_texture, src_rect, dst_rect);

    if (focused && (SDL_GetTicks() / 500U) % 2U == 0U) {
        const int cursor_x = rect.GetX() + 4 + clipped_w;
        renderer.SetDrawColor(255, 255, 255, 255);
        SDL2pp::Rect cursor_rect(cursor_x, rect.GetY() + 4, 2, rect.GetH() - 8);
        renderer.FillRect(cursor_rect);
    }
}
