#include "ui_renderer.h"

#include <algorithm>
#include <string>

#include "../chat_input.h"
#include "geometry.h"
#include "texture_loader.h"

UIRenderer::UIRenderer(SDL2pp::Renderer& renderer, int window_w, int window_h,
                       const ChatInput& chat_model):
        renderer(renderer),
        chat_model(chat_model),
        ui_frame_texture(renderer, load_surface("assets/interface/en_ventanaprincipal.bmp")),
        hp_bar_texture(renderer, load_surface("assets/interface/en_barradevida.bmp")),
        mp_bar_texture(renderer, load_surface("assets/interface/en_barrademana.bmp")),
        ui_frame_rect(0, 0, window_w, window_h),
        chat_input_rect(41, 122, 565, 20) {
    chat_font = TTF_OpenFont("assets/OUTPUT/Cardo.ttf", 16);
    if (!chat_font) {
        throw std::runtime_error(std::string("TTF_OpenFont failed: ") + TTF_GetError());
    }
    bar_font = TTF_OpenFont("assets/OUTPUT/Cardo.ttf", 11);
    if (!bar_font) {
        throw std::runtime_error(std::string("TTF_OpenFont failed: ") + TTF_GetError());
    }
}

UIRenderer::~UIRenderer() {
    if (chat_font) {
        TTF_CloseFont(chat_font);
        chat_font = nullptr;
    }
    if (bar_font) {
        TTF_CloseFont(bar_font);
        bar_font = nullptr;
    }
}

void UIRenderer::render_frame_background() {
    renderer.Copy(ui_frame_texture, SDL2pp::NullOpt, ui_frame_rect);
}

void UIRenderer::render_chat_input() {
    if (!chat_font) {
        return;
    }

    const bool show_cursor = chat_model.is_focused() && ((SDL_GetTicks() / 500U) % 2U == 0U);

    int clipped_w = 0;
    const std::string& text = chat_model.get_text();
    if (!text.empty()) {
        render_chat_text_line(clipped_w);
    }

    if (show_cursor) {
        render_chat_cursor(clipped_w);
    }
}

bool UIRenderer::is_chat_input_hit(int x, int y) const {
    return point_in_rect(x, y, chat_input_rect);
}

void UIRenderer::render_hp_bar(uint32_t current, uint32_t max) {
    render_stat_bar(hp_bar_texture, HP_BAR_X, HP_BAR_Y, HP_BAR_W, HP_BAR_H, current, max);
}

void UIRenderer::render_mp_bar(uint32_t current, uint32_t max) {
    render_stat_bar(mp_bar_texture, MP_BAR_X, MP_BAR_Y, MP_BAR_W, MP_BAR_H, current, max);
}

void UIRenderer::render_stat_bar(SDL2pp::Texture& tex, int x, int y, int w, int h,
                                 uint32_t current, uint32_t max) const {
    if (max == 0) {
        return;
    }

    const float ratio = std::min(1.0f, static_cast<float>(current) / static_cast<float>(max));
    const int filled_w = static_cast<int>(w * ratio);
    if (filled_w == 0) {
        return;
    }

    SDL2pp::Rect src(0, 0, filled_w, h);
    SDL2pp::Rect dst(x, y, filled_w, h);
    renderer.Copy(tex, src, dst);

    if (!bar_font) {
        return;
    }

    std::string text = std::to_string(current) + "/" + std::to_string(max);
    SDL_Surface* text_surface = TTF_RenderUTF8_Blended(bar_font, text.c_str(), chat_color);
    if (!text_surface) {
        return;
    }
    SDL2pp::Surface wrapped(text_surface);

    int text_w = 0;
    int text_h = 0;
    if (TTF_SizeUTF8(bar_font, text.c_str(), &text_w, &text_h) != 0) {
        return;
    }

    SDL2pp::Texture text_texture(renderer, wrapped);
    const int text_x = x + (w - text_w) / 2;
    const int text_y = y + (h - text_h) / 2;
    SDL2pp::Rect text_dst(text_x, text_y, text_w, text_h);
    renderer.Copy(text_texture, SDL2pp::NullOpt, text_dst);
}

SDL2pp::Texture UIRenderer::make_text_texture(const std::string& text, int& text_w,
                                              int& text_h) const {
    SDL_Surface* text_surface = TTF_RenderUTF8_Blended(chat_font, text.c_str(), chat_color);
    if (!text_surface) {
        throw std::runtime_error(std::string("TTF_RenderUTF8_Blended failed: ") + TTF_GetError());
    }

    SDL2pp::Surface wrapped_surface(text_surface);
    if (TTF_SizeUTF8(chat_font, text.c_str(), &text_w, &text_h) != 0) {
        throw std::runtime_error(std::string("TTF_SizeUTF8 failed: ") + TTF_GetError());
    }

    return SDL2pp::Texture(renderer, wrapped_surface);
}

void UIRenderer::render_chat_text_line(int& clipped_w) const {
    try {
        int text_w = 0;
        int text_h = 0;
        SDL2pp::Texture text_texture = make_text_texture(chat_model.get_text(), text_w, text_h);

        clipped_w = std::min(text_w, chat_input_rect.GetW());
        const int clipped_h = std::min(text_h, chat_input_rect.GetH());
        SDL2pp::Rect src_rect(0, 0, clipped_w, clipped_h);
        SDL2pp::Rect dst_rect(chat_input_rect.GetX(), chat_input_rect.GetY(), clipped_w, clipped_h);
        renderer.Copy(text_texture, src_rect, dst_rect);
    } catch (const std::runtime_error&) {
        clipped_w = 0;
    }
}

void UIRenderer::render_chat_cursor(int x_offset) const {
    try {
        int cursor_w = 0;
        int cursor_h = 0;
        SDL2pp::Texture cursor_texture = make_text_texture("|", cursor_w, cursor_h);
        const int cursor_x = std::min(chat_input_rect.GetX() + x_offset,
                                      chat_input_rect.GetX() + chat_input_rect.GetW() - cursor_w);
        SDL2pp::Rect cursor_dst(cursor_x, chat_input_rect.GetY(),
                                std::min(cursor_w, chat_input_rect.GetW()),
                                std::min(cursor_h, chat_input_rect.GetH()));
        renderer.Copy(cursor_texture, SDL2pp::NullOpt, cursor_dst);
    } catch (const std::runtime_error&) {
        return;
    }
}
