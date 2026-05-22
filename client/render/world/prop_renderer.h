#ifndef CLIENT_PROP_RENDERER_H
#define CLIENT_PROP_RENDERER_H

#include <cstdint>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2pp/SDL2pp.hh>

#include "../../config/config.h"

class AnimationSystem;

class PropRenderer {
public:
    explicit PropRenderer(SDL2pp::Renderer& renderer);

    void load(const TilemapConfig& tilemap, int tile_size);
    void render_behind(const SDL2pp::Rect& cam, int player_foot_y);
    void render_front(const SDL2pp::Rect& cam, int player_foot_y);
    void render_hitboxes(const SDL2pp::Rect& cam);
    void tick_animations(AnimationSystem& anim);
    bool has_props() const { return !prop_tiles_.empty() && has_tilemap_; }

private:
    struct PropRender {
        std::vector<SDL2pp::Texture> frames;
        SDL2pp::Rect src;
        int display_w = 0;
        int display_h = 0;
        uint32_t frame_ms = 0;
        std::size_t current_frame = 0;
        uint32_t last_ticks = 0;
        bool animated = false;
        int hitbox_x = 0;
        int hitbox_y = 0;
        int hitbox_w = 0;
        int hitbox_h = 0;
    };

    SDL2pp::Renderer& renderer;
    std::vector<std::vector<PropRender>> prop_tiles_;
    int tile_size_ = 128;
    bool has_tilemap_ = false;
};

#endif
