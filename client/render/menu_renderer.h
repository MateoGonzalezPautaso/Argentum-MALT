#ifndef CLIENT_MENU_RENDERER_H
#define CLIENT_MENU_RENDERER_H

#include <SDL2pp/SDL2pp.hh>

#include "button.h"

class MenuRenderer {
private:
    static constexpr int START_X = 347;
    static constexpr int START_Y = 568;
    static constexpr int START_W = 330;
    static constexpr int START_H = 65;

    static constexpr int SETTINGS_X = 688;
    static constexpr int SETTINGS_Y = 335;
    static constexpr int SETTINGS_W = 270;
    static constexpr int SETTINGS_H = 45;

    SDL2pp::Renderer& renderer;
    SDL2pp::Texture menu_background_texture;
    Button start_button;
    Button settings_button;
    SDL2pp::Rect menu_background_rect;
    int window_w;
    int window_h;

public:
    MenuRenderer(SDL2pp::Renderer& renderer, int window_w, int window_h);

    void render();
    bool is_start_hit(int x, int y) const;
    bool is_settings_hit(int x, int y) const;
    void set_start_button_hovered(int x, int y);
    void set_settings_button_hovered(int x, int y);

private:
    void init_layout();
};

#endif
