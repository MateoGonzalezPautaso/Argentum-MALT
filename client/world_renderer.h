#ifndef CLIENT_WORLD_RENDERER_H
#define CLIENT_WORLD_RENDERER_H

#include <cstdint>
#include <string>
#include <vector>

#include <SDL2pp/SDL2pp.hh>

#include "config.h"

class WorldRenderer {
private:
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

    SDL2pp::Renderer& renderer;
    SDL2pp::Texture background_texture;
    SDL2pp::Rect background_rect;
    SDL2pp::Rect game_viewport;
    SDL2pp::Texture tilemap_texture;
    bool has_tilemap = false;
    int tile_size = 128;
    std::vector<std::vector<SDL2pp::Rect>> tilemap_src;
    int map_px_w = 0;
    int map_px_h = 0;
    std::vector<SpriteRender> sprites;
    int window_w;
    int window_h;

public:
    WorldRenderer(SDL2pp::Renderer& renderer, const BackgroundConfig& background,
                  const TilemapConfig& tilemap, const std::vector<SpriteConfig>& sprites_config,
                  int window_w, int window_h);

    void render();
    void set_movable_position(int x, int y);
    bool get_movable_position(int& x, int& y) const;
    void get_camera_offset(int& x, int& y) const;
    bool screen_to_world(int screen_x, int screen_y, int& world_x, int& world_y) const;
    void set_movable_src_y(int y);
    void step_movable_src_x(int step, int frame_count);
    void set_anchor_src_y(int y);

private:
    void init_tilemap(const TilemapConfig& tilemap);
    void init_sprites(const std::vector<SpriteConfig>& sprites_config);
    SpriteRender build_sprite_render(const SpriteConfig& sprite_config);
    SDL2pp::Rect camera_rect() const;
    void render_tilemap_or_background(const SDL2pp::Rect& cam);
    void render_sprites(const SDL2pp::Rect& cam);
    void update_animation();
    void update_anchor_positions();
    SpriteRender* find_movable_sprite();
    const SpriteRender* find_movable_sprite() const;
};

#endif
