#ifndef CLIENT_MENU_RENDERER_H
#define CLIENT_MENU_RENDERER_H

#include <SDL2pp/SDL2pp.hh>

#include "../config/config.h"

#include "button.h"

class MenuRenderer {
private:
    SDL2pp::Renderer& renderer;
    SDL2pp::Texture menu_background_texture;
    Button start_button;
    Button settings_button;
    SDL2pp::Rect menu_background_rect;
    UIConfig ui_cfg;

public:
    MenuRenderer(SDL2pp::Renderer& renderer, const UIConfig& ui_cfg);

    void render();
    bool is_start_hit(int x, int y) const;
    bool is_settings_hit(int x, int y) const;
    void set_start_button_hovered(int x, int y);
    void set_settings_button_hovered(int x, int y);

private:
    void init_layout();
};

#endif
