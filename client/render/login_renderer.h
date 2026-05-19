#ifndef CLIENT_LOGIN_RENDERER_H
#define CLIENT_LOGIN_RENDERER_H

#include <string>

#include <SDL2pp/SDL2pp.hh>
#include <SDL_ttf.h>

#include "button.h"

class ChatInput;

class LoginRenderer {
private:
    SDL2pp::Renderer& renderer;
    const ChatInput& username_model;
    const ChatInput& password_model;

    SDL2pp::Texture background_texture;
    Button connect_button;
    Button back_button;

    SDL2pp::Rect background_rect;
    SDL2pp::Rect username_field_rect;
    SDL2pp::Rect password_field_rect;

    TTF_Font* field_font = nullptr;
    SDL_Color text_color{255, 255, 255, 255};
    int window_w;
    int window_h;

public:
    LoginRenderer(SDL2pp::Renderer& renderer, int window_w, int window_h,
                  const ChatInput& username_model, const ChatInput& password_model);
    ~LoginRenderer();

    void render();

    bool is_username_hit(int x, int y) const;
    bool is_password_hit(int x, int y) const;
    bool is_connect_button_hit(int x, int y) const;
    bool is_back_button_hit(int x, int y) const;

    void set_connect_button_hovered(int x, int y);
    void set_back_button_hovered(int x, int y);

private:
    void init_layout();
    void render_text_field(const SDL2pp::Rect& rect, const std::string& text,
                           bool focused) const;
};

#endif
