#ifndef CLIENT_LOGIN_RENDERER_H
#define CLIENT_LOGIN_RENDERER_H

#include <string>

#include <SDL2pp/SDL2pp.hh>
#include <SDL_ttf.h>

#include "../config/config.h"

#include "button.h"

class ChatInput;

class LoginRenderer {
private:
    SDL2pp::Renderer& renderer;
    const ChatInput& username_model;
    const ChatInput& password_model;

    std::string error_text;
    SDL_Color error_color{255, 60, 60, 255};

    SDL2pp::Texture background_texture;
    SDL2pp::Texture logo_texture;
    Button connect_button;
    Button back_button;
    Button new_account_button;

    SDL2pp::Rect background_rect;
    SDL2pp::Rect logo_rect;
    SDL2pp::Rect title_rect;
    SDL2pp::Rect username_field_rect;
    SDL2pp::Rect password_field_rect;

    TTF_Font* field_font = nullptr;
    TTF_Font* title_font = nullptr;
    SDL_Color text_color{255, 255, 255, 255};
    SDL_Color placeholder_color{160, 160, 160, 255};
    SDL_Color title_color{255, 215, 0, 255};
    UIConfig ui_cfg;

public:
    LoginRenderer(SDL2pp::Renderer& renderer, const UIConfig& ui_cfg,
                  const ChatInput& username_model, const ChatInput& password_model);
    ~LoginRenderer();

    void render();

    bool is_username_hit(int x, int y) const;
    bool is_password_hit(int x, int y) const;
    bool is_connect_button_hit(int x, int y) const;
    bool is_back_button_hit(int x, int y) const;

    void set_connect_button_hovered(int x, int y);
    void set_back_button_hovered(int x, int y);
    void set_new_account_button_hovered(int x, int y);

    void set_error(const std::string& text);
    void clear_error();

    bool is_new_account_hit(int x, int y) const;

private:
    void init_layout();
    void render_text_field(const SDL2pp::Rect& rect, const std::string& text, bool focused,
                           const std::string& placeholder) const;
    void render_text_in_rect(const SDL2pp::Rect& rect, const std::string& text, SDL_Color color,
                             int& clipped_w) const;
    void render_error() const;
};

#endif
