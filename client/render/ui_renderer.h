#ifndef CLIENT_UI_RENDERER_H
#define CLIENT_UI_RENDERER_H

#include <string>

#include <SDL2pp/SDL2pp.hh>
#include <SDL_ttf.h>

#include "../chat/chat_history.h"
#include "../config/config.h"

class ChatInput;

class UIRenderer {
private:
    SDL2pp::Renderer& renderer;
    const ChatInput& chat_model;
    SDL2pp::Texture ui_frame_texture;
    SDL2pp::Texture hp_bar_texture;
    SDL2pp::Texture mp_bar_texture;
    SDL2pp::Texture exp_bar_texture;
    SDL2pp::Rect ui_frame_rect;
    SDL2pp::Rect chat_input_rect;
    SDL2pp::Rect chat_history_rect;
    TTF_Font* chat_font = nullptr;
    TTF_Font* bar_font = nullptr;
    SDL_Color chat_color{255, 255, 255, 255};
    UIConfig ui_cfg;

public:
    UIRenderer(SDL2pp::Renderer& renderer, const UIConfig& ui_cfg, const ChatInput& chat_model);
    ~UIRenderer();

    void render_frame_background();
    void render_chat_input();
    void render_chat_history(const std::vector<ChatMessage>& messages);
    void render_hp_bar(uint32_t current, uint32_t max);
    void render_mp_bar(uint32_t current, uint32_t max);
    void render_exp_bar(uint32_t current, uint32_t max);
    bool is_chat_input_hit(int x, int y) const;

private:
    void render_stat_bar(SDL2pp::Texture& tex, int x, int y, int w, int h, uint32_t current,
                         uint32_t max) const;
    void render_chat_cursor(int x_offset) const;
    void render_chat_text_line(int& clipped_w) const;
};

#endif
