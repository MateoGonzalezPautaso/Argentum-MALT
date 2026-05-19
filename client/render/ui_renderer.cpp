#include "ui_renderer.h"

#include <algorithm>

#include "../chat_input.h"
#include "texture_loader.h"

UIRenderer::UIRenderer(SDL2pp::Renderer& renderer, int window_w, int window_h,
                       const ChatInput& chat_model):
        renderer(renderer),
        chat_model(chat_model),
        ui_frame_texture(renderer, load_surface("assets/interface/en_ventanaprincipal.bmp")),
        ui_frame_rect(0, 0, window_w, window_h),
        chat_input_rect(41, 122, 565, 20) {
    chat_font = TTF_OpenFont("assets/OUTPUT/Cardo.ttf", 16);
    if (!chat_font) {
        throw std::runtime_error(std::string("TTF_OpenFont failed: ") + TTF_GetError());
    }
}

UIRenderer::~UIRenderer() {
    if (chat_font) {
        TTF_CloseFont(chat_font);
        chat_font = nullptr;
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
    const int left = chat_input_rect.GetX();
    const int top = chat_input_rect.GetY();
    const int right = left + chat_input_rect.GetW();
    const int bottom = top + chat_input_rect.GetH();
    return x >= left && x <= right && y >= top && y <= bottom;
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
