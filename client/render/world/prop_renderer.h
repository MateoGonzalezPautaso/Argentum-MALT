#ifndef CLIENT_PROP_RENDERER_H
#define CLIENT_PROP_RENDERER_H

#include <cstdint>
#include <string>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2pp/SDL2pp.hh>

#include "../../../common/config.h"
#include "../texture_cache.h"

class AnimationSystem;

class PropRenderer {
public:
    explicit PropRenderer(SDL2pp::Renderer& renderer);

    void load(const TilemapConfig& tilemap, int tile_size);
    void render_behind(const SDL2pp::Rect& cam, int player_foot_y);
    void render_front(const SDL2pp::Rect& cam, int player_foot_y);
    void render_hitboxes(const SDL2pp::Rect& cam);
    void tick_animations(AnimationSystem& anim);
    bool hit_test_prop(int world_x, int world_y, std::string& out_prop_name) const;
    bool has_props() const { return !prop_tiles_.empty() && has_tilemap_; }

private:
    struct PropPart {
        std::vector<SDL2pp::Texture*> frames;
        SDL2pp::Rect src;
        int offset_x = 0;
        int offset_y = 0;
        int display_w = 0;
        int display_h = 0;
        uint32_t frame_ms = 0;
        std::size_t current_frame = 0;
        uint32_t last_ticks = 0;
        bool animated = false;
    };

    struct PropRender {
        std::string name;
        std::vector<SDL2pp::Texture*> frames;
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
        int tile_col = 0;
        int tile_row = 0;
        std::vector<PropPart> parts;
    };

    PropRender build_prop_render(const std::string& name, std::size_t ri, std::size_t ci,
                                 std::size_t tsz, const PropDef& def);
    void render_conditional(const SDL2pp::Rect& cam, int player_foot_y, bool behind);

    SDL2pp::Renderer& renderer;
    TextureCache texture_cache_;
    std::vector<std::vector<PropRender>> prop_tiles_;
    int tile_size_ = 128;
    bool has_tilemap_ = false;
};

#endif
