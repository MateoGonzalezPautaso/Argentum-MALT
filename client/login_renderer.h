#ifndef CLIENT_LOGIN_RENDERER_H
#define CLIENT_LOGIN_RENDERER_H

#include <string>

#include <SDL2pp/SDL2pp.hh>
#include <SDL_ttf.h>

class ChatInput;

class LoginRenderer {
private:
    SDL2pp::Renderer& renderer;
    const ChatInput& username_model;
    const ChatInput& password_model;

    SDL2pp::Texture background_texture;
    SDL2pp::Texture connect_button_texture;
    SDL2pp::Texture back_button_texture;

    SDL2pp::Rect background_rect;
    SDL2pp::Rect username_field_rect;
    SDL2pp::Rect password_field_rect;
    SDL2pp::Rect connect_button_rect;
    SDL2pp::Rect back_button_rect;

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

private:
    void init_layout();
    void render_text_field(const SDL2pp::Rect& rect, const std::string& text,
                           bool focused) const;
};

#endif
