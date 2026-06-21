#ifndef CLIENT_RENDER_FORM_WIDGETS_H
#define CLIENT_RENDER_FORM_WIDGETS_H

#include <string>

#include <SDL2pp/SDL2pp.hh>
#include <SDL_ttf.h>

#include "../config/config.h"

class FormWidgets {
public:
    FormWidgets(SDL2pp::Renderer& renderer, TTF_Font* font, const UIConfig& ui_cfg);

    // Renders a styled text input field with focus highlight and blinking cursor.
    void render_text_field(const SDL2pp::Rect& rect, const std::string& text, bool focused,
                           const std::string& placeholder) const;

    // Renders `error_text` centered horizontally, `error_spacing` pixels below `below_y`.
    void render_error_centered(const std::string& error_text, SDL_Color error_color, int window_w,
                               int below_y, int error_spacing) const;

private:
    SDL2pp::Renderer& renderer_;
    TTF_Font* font_;
    const UIConfig& ui_cfg_;
};

#endif  // CLIENT_RENDER_FORM_WIDGETS_H
