#ifndef CLIENT_RENDERER_H
#define CLIENT_RENDERER_H

#include <cstdint>
#include <string>
#include <vector>

#include <SDL2pp/SDL2pp.hh>

#include "config.h"

class ClientRenderer {
private:
    SDL2pp::Window& window;
    SDL2pp::Renderer renderer;
    SDL2pp::Surface background_surface;
    SDL2pp::Texture background_texture;
    SDL2pp::Rect background_rect;
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
        bool visible = true;
    };
    std::vector<SpriteRender> sprites;
    int window_w;
    int window_h;
    bool show_extra_sprite;

public:
    ClientRenderer(SDL2pp::Window& window,
                   const BackgroundConfig& background,
                   const std::vector<SpriteConfig>& sprites_config,
                   int window_w,
                   int window_h);

    void render_frame();
    void move_sprite(int dx, int dy);
    void toggle_extra_sprite();
    void set_movable_src_y(int y);
    void step_movable_src_x(int step, int max_x);

private:
    static SDL2pp::Surface load_surface(const std::string& path);
    SDL2pp::Rect extra_sprite_dst(const SpriteRender& sprite) const;
    void update_animation();
    SpriteRender* find_movable_sprite();
};

#endif
