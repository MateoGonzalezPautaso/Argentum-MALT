#include "form_widgets.h"

#include <algorithm>

#include <SDL2/SDL.h>

#include "text_renderer.h"

FormWidgets::FormWidgets(SDL2pp::Renderer& renderer, TTF_Font* font, const UIConfig& ui_cfg):
        renderer_(renderer), font_(font), ui_cfg_(ui_cfg) {}

void FormWidgets::render_text_field(const SDL2pp::Rect& rect, const std::string& text, bool focused,
                                    const std::string& placeholder) const {
    if (!font_)
        return;

    renderer_.SetDrawColor(focused ? SDL_Color{45, 45, 45, 220} : SDL_Color{30, 30, 30, 220});
    renderer_.FillRect(rect);
    renderer_.SetDrawColor(focused ? SDL_Color{200, 180, 80, 255} : SDL_Color{100, 100, 100, 255});
    renderer_.DrawRect(rect);

    static constexpr SDL_Color kTextColor{255, 255, 255, 255};
    static constexpr SDL_Color kPlaceholderColor{160, 160, 160, 255};

    int clipped_w = 0;
    if (!text.empty())
        clipped_w = texture::render_text_clipped(renderer_, font_, text, kTextColor, rect, 4, 2,
                                                 true);
    else if (!focused && !placeholder.empty())
        texture::render_text_clipped(renderer_, font_, placeholder, kPlaceholderColor, rect, 4, 2,
                                     true);

    if (focused && (SDL_GetTicks() / ui_cfg_.cursor_blink_ms) % 2U == 0U) {
        const int cursor_x = rect.GetX() + ui_cfg_.cursor_padding + clipped_w;
        renderer_.SetDrawColor(255, 255, 255, 255);
        SDL2pp::Rect cursor_rect(cursor_x, rect.GetY() + ui_cfg_.cursor_padding,
                                 ui_cfg_.cursor_width, rect.GetH() - ui_cfg_.cursor_height_shrink);
        renderer_.FillRect(cursor_rect);
    }
}

void FormWidgets::render_error_centered(const std::string& error_text, SDL_Color error_color,
                                        int window_w, int below_y, int error_spacing) const {
    if (error_text.empty() || !font_)
        return;
    auto result = texture::render_text(renderer_, font_, error_text, error_color);
    if (result.w == 0)
        return;
    const int x = std::max(0, (window_w - result.w) / 2);
    renderer_.Copy(result.texture, SDL2pp::NullOpt,
                   SDL2pp::Rect(x, below_y + error_spacing, result.w, result.h));
}
