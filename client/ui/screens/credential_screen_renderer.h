#ifndef CLIENT_CREDENTIAL_SCREEN_RENDERER_H
#define CLIENT_CREDENTIAL_SCREEN_RENDERER_H

#include <optional>
#include <string>

#include <SDL2pp/SDL2pp.hh>
#include <SDL_ttf.h>

#include "../../config/config.h"

#include "form_widgets.h"

class ChatInput;

class CredentialScreenRenderer {
public:
    void set_error(const std::string& text);
    void clear_error();
    bool is_username_hit(int x, int y) const;
    bool is_password_hit(int x, int y) const;

protected:
    CredentialScreenRenderer(SDL2pp::Renderer& renderer, const UIConfig& ui_cfg,
                             const ChatInput& username_model, const ChatInput& password_model,
                             std::string title_text);
    ~CredentialScreenRenderer();

    void init_shared_layout();
    void render_chrome();
    void render_text_field(const SDL2pp::Rect& rect, const std::string& text, bool focused,
                           const std::string& placeholder) const;
    void render_error(int below_y) const;

    SDL2pp::Renderer& renderer;
    const ChatInput& username_model;
    const ChatInput& password_model;

    std::string error_text;
    SDL_Color error_color{255, 60, 60, 255};
    SDL_Color text_color{255, 255, 255, 255};
    SDL_Color placeholder_color{160, 160, 160, 255};
    SDL_Color title_color{255, 215, 0, 255};

    SDL2pp::Texture background_texture;
    SDL2pp::Texture logo_texture;
    SDL2pp::Rect background_rect;
    SDL2pp::Rect logo_rect;
    SDL2pp::Rect title_rect;
    SDL2pp::Rect username_field_rect;
    SDL2pp::Rect password_field_rect;

    TTF_Font* field_font = nullptr;
    TTF_Font* title_font = nullptr;
    UIConfig ui_cfg;
    std::optional<FormWidgets> form_widgets_;

private:
    std::string title_text_;
};

#endif  // CLIENT_CREDENTIAL_SCREEN_RENDERER_H
