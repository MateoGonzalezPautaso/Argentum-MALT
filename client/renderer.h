#ifndef CLIENT_RENDERER_H
#define CLIENT_RENDERER_H

#include <cstdint>
#include <memory>

#include <SDL2pp/SDL2pp.hh>

#include "config.h"

class MenuRenderer;
class WorldRenderer;

class ClientRenderer {
private:
    SDL2pp::Renderer renderer;
    std::unique_ptr<MenuRenderer> menu_renderer;
    std::unique_ptr<WorldRenderer> world_renderer;

public:
    ClientRenderer(SDL2pp::Window& window,
                   const BackgroundConfig& background,
                   const TilemapConfig& tilemap,
                   const std::vector<SpriteConfig>& sprites_config,
                   int window_w,
                   int window_h);
    ~ClientRenderer();

    void render_frame();
    void render_menu();
    bool is_menu_button_hit(int x, int y) const;
    void move_sprite(int dx, int dy);
    bool get_movable_position(int& x, int& y) const;
    void get_camera_offset(int& x, int& y) const;
    bool screen_to_world(int screen_x, int screen_y, int& world_x, int& world_y) const;
    void set_movable_src_y(int y);
    void step_movable_src_x(int step, int frame_count);
    void set_anchor_src_y(int y);
};

#endif
