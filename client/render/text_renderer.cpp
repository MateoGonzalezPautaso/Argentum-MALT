#include "text_renderer.h"

#include <algorithm>

namespace texture {

RenderedText render_text(SDL2pp::Renderer& renderer, TTF_Font* font, const std::string& text,
                         SDL_Color color) {
    if (!font || text.empty()) {
        return {SDL2pp::Texture(static_cast<SDL_Texture*>(nullptr)), 0, 0};
    }

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface) {
        return {SDL2pp::Texture(static_cast<SDL_Texture*>(nullptr)), 0, 0};
    }

    SDL2pp::Surface wrapped(surface);

    int text_w = 0;
    int text_h = 0;
    if (TTF_SizeUTF8(font, text.c_str(), &text_w, &text_h) != 0) {
        return {SDL2pp::Texture(static_cast<SDL_Texture*>(nullptr)), 0, 0};
    }

    return {SDL2pp::Texture(renderer, wrapped), text_w, text_h};
}

int render_text_clipped(SDL2pp::Renderer& renderer, TTF_Font* font, const std::string& text,
                        SDL_Color color, const SDL2pp::Rect& rect, int pad_x, int pad_y,
                        bool center_y) {
    auto result = render_text(renderer, font, text, color);
    if (result.w == 0) {
        return 0;
    }

    int avail_w = rect.GetW() - pad_x * 2;
    int avail_h = rect.GetH() - pad_y * 2;
    int clipped_w = std::min(result.w, avail_w);
    int clipped_h = std::min(result.h, avail_h);

    SDL2pp::Rect src(0, 0, clipped_w, clipped_h);

    int dst_y = rect.GetY() + pad_y;
    if (center_y) {
        dst_y = rect.GetY() + (rect.GetH() - clipped_h) / 2;
    }

    SDL2pp::Rect dst(rect.GetX() + pad_x, dst_y, clipped_w, clipped_h);
    renderer.Copy(result.texture, src, dst);
    return clipped_w;
}

}  // namespace texture
