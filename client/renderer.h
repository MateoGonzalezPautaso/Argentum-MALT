#ifndef CLIENT_RENDERER_H
#define CLIENT_RENDERER_H

#include <cstdint>
#include <string>
#include <vector>

#include <SDL2pp/SDL2pp.hh>

#include "config.h"

class ClientRenderer {
private:
    SDL2pp::Renderer renderer;
    SDL2pp::Texture background_texture;
    SDL2pp::Rect background_rect;
    SDL2pp::Texture menu_background_texture;
    SDL2pp::Texture menu_logo_texture;
    SDL2pp::Texture menu_button_texture;
    SDL2pp::Rect menu_background_rect;
    SDL2pp::Rect menu_logo_rect;
    SDL2pp::Rect menu_button_rect;
    struct SpriteRender {
        std::vector<SDL2pp::Texture> frames;
        SDL2pp::Rect src;
        SDL2pp::Rect dst;
        bool animated = false;
        bool use_src = false;
        uint32_t frame_ms = 0;
        std::size_t current_frame = 0;
        uint32_t last_ticks = 0;
        bool movable = false;
        bool anchor_to_movable = false;
        int anchor_offset_x = 0;
        int anchor_offset_y = 0;
        bool visible = true;
    };
    std::vector<SpriteRender> sprites;
    int window_w;
    int window_h;

public:
    ClientRenderer(SDL2pp::Window& window,
                   const BackgroundConfig& background,
                   const std::vector<SpriteConfig>& sprites_config,
                   int window_w,
                   int window_h);

    void render_frame();
    void render_menu();
    bool is_menu_button_hit(int x, int y) const;
    void move_sprite(int dx, int dy);
    bool get_movable_position(int& x, int& y) const;
    void set_movable_src_y(int y);
    void step_movable_src_x(int step, int frame_count);
    void set_anchor_src_y(int y);

private:
    static SDL2pp::Surface load_surface(const std::string& path);
    void init_menu_layout();
    void init_sprites(const std::vector<SpriteConfig>& sprites_config);
    SpriteRender build_sprite_render(const SpriteConfig& sprite_config);
    void update_animation();
    void update_anchor_positions();
    SpriteRender* find_movable_sprite();
    const SpriteRender* find_movable_sprite() const;
};

#endif
