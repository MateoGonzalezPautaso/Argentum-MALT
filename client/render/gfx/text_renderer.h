#ifndef CLIENT_TEXT_RENDERER_H
#define CLIENT_TEXT_RENDERER_H

#include <string>

#include <SDL2pp/SDL2pp.hh>
#include <SDL_ttf.h>

namespace texture {

struct RenderedText {
    SDL2pp::Texture texture;
    int w = 0;
    int h = 0;
};

RenderedText render_text(SDL2pp::Renderer& renderer, TTF_Font* font, const std::string& text,
                         SDL_Color color);

int render_text_clipped(SDL2pp::Renderer& renderer, TTF_Font* font, const std::string& text,
                        SDL_Color color, const SDL2pp::Rect& rect, int pad_x = 0, int pad_y = 0,
                        bool center_y = false);

}  // namespace texture

#endif  // CLIENT_TEXT_RENDERER_H
