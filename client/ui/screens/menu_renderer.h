#ifndef CLIENT_MENU_RENDERER_H
#define CLIENT_MENU_RENDERER_H

#include <SDL2pp/SDL2pp.hh>

#include "../../config/config.h"

#include "../../render/gfx/button.h"

class MenuRenderer {
private:
    SDL2pp::Renderer& renderer;
    SDL2pp::Texture menu_background_texture;
    Button start_button;
    Button audio_button;
    SDL2pp::Texture audio_off_texture;
    bool audio_muted_ = false;
    SDL2pp::Rect menu_background_rect;
    UIConfig ui_cfg;

public:
    MenuRenderer(SDL2pp::Renderer& renderer, const UIConfig& ui_cfg);

    void render();
    bool is_start_hit(int x, int y) const;
    bool is_audio_hit(int x, int y) const;
    void set_start_button_hovered(int x, int y);
    void set_audio_button_hovered(int x, int y);
    void set_audio_muted(bool muted) { audio_muted_ = muted; }

private:
    void init_layout();
};

#endif
