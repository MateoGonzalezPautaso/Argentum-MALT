#include "text_renderer.h"

namespace texture {

RenderedText render_text(SDL2pp::Renderer& renderer, TTF_Font* font,
                          const std::string& text, SDL_Color color) {
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

}  // namespace texture
