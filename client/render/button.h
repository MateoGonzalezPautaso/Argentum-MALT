#ifndef CLIENT_RENDER_BUTTON_H
#define CLIENT_RENDER_BUTTON_H

#include <utility>

#include <SDL2pp/SDL2pp.hh>

#include "geometry.h"

struct Button {
    SDL2pp::Texture default_tex;
    SDL2pp::Texture hover_tex;
    SDL2pp::Rect rect;
    bool hovered = false;

    Button(SDL2pp::Texture default_tex_, SDL2pp::Texture hover_tex_):
            default_tex(std::move(default_tex_)), hover_tex(std::move(hover_tex_)), rect() {}

    void set_position(int x, int y, int w, int h) { rect = SDL2pp::Rect(x, y, w, h); }

    void render(SDL2pp::Renderer& renderer) {
        SDL2pp::Texture& tex = hovered ? hover_tex : default_tex;
        renderer.Copy(tex, SDL2pp::NullOpt, rect);
    }

    bool is_hit(int x, int y) const { return point_in_rect(x, y, rect); }
};

#endif  // CLIENT_RENDER_BUTTON_H
