#ifndef CLIENT_MENU_CONTROLLER_H
#define CLIENT_MENU_CONTROLLER_H

#include <SDL2pp/SDL2pp.hh>

#include "../config/config.h"
#include "../render/menu_renderer.h"

class MenuController {
public:
    explicit MenuController(SDL2pp::Renderer& renderer, const UIConfig& ui_cfg);

    void handle_mouse_motion(int x, int y);
    bool is_start_hit(int x, int y) const;
    bool is_audio_hit(int x, int y) const;
    void set_audio_muted(bool muted);
    void render();

private:
    MenuRenderer menu_renderer;
};

#endif
