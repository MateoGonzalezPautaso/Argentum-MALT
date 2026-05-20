#ifndef CLIENT_UI_RENDERER_H
#define CLIENT_UI_RENDERER_H

#include <string>

#include <SDL2pp/SDL2pp.hh>
#include <SDL_ttf.h>

class ChatInput;

class UIRenderer {
private:
    static constexpr int HP_BAR_X = 790;
    static constexpr int HP_BAR_Y = 601;
    static constexpr int HP_BAR_W = 218;
    static constexpr int HP_BAR_H = 17;

    SDL2pp::Renderer& renderer;
    const ChatInput& chat_model;
    SDL2pp::Texture ui_frame_texture;
    SDL2pp::Texture hp_bar_texture;
    SDL2pp::Rect ui_frame_rect;
    SDL2pp::Rect chat_input_rect;
    TTF_Font* chat_font = nullptr;
    TTF_Font* hp_font = nullptr;
    SDL_Color chat_color{255, 255, 255, 255};

public:
    UIRenderer(SDL2pp::Renderer& renderer, int window_w, int window_h, const ChatInput& chat_model);
    ~UIRenderer();

    void render_frame_background();
    void render_chat_input();
    void render_hp_bar(uint32_t current, uint32_t max);
    bool is_chat_input_hit(int x, int y) const;

private:
    SDL2pp::Texture make_text_texture(const std::string& text, int& text_w, int& text_h) const;
    void render_chat_cursor(int x_offset) const;
    void render_chat_text_line(int& clipped_w) const;
};

#endif
