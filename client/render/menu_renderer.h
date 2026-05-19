#ifndef CLIENT_MENU_RENDERER_H
#define CLIENT_MENU_RENDERER_H

#include <SDL2pp/SDL2pp.hh>

#include "button.h"

class MenuRenderer {
private:
    SDL2pp::Renderer& renderer;
    SDL2pp::Texture menu_background_texture;
    SDL2pp::Texture menu_logo_texture;
    Button start_button;
    SDL2pp::Rect menu_background_rect;
    SDL2pp::Rect menu_logo_rect;
    int window_w;
    int window_h;

public:
    MenuRenderer(SDL2pp::Renderer& renderer, int window_w, int window_h);

    void render();
    bool is_button_hit(int x, int y) const;
    void set_button_hovered(int x, int y);

private:
    void init_layout();
};

#endif
