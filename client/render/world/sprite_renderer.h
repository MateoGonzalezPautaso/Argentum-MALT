#ifndef CLIENT_SPRITE_RENDERER_H
#define CLIENT_SPRITE_RENDERER_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <SDL2pp/SDL2pp.hh>
#include <SDL_ttf.h>

#include "../../../common/messages.h"
#include "../../config/config.h"

class AnimationSystem;

class SpriteRenderer {
public:
    SpriteRenderer(SDL2pp::Renderer& renderer, TTF_Font* name_font, int window_w, int window_h,
                   bool has_tilemap, int map_px_w, int map_px_h);

    void load_sprites(const std::vector<SpriteConfig>& sprites_config);
    void set_map_bounds(bool has_tilemap, int map_px_w, int map_px_h);
    void set_name_font(TTF_Font* font) { name_font = font; }

    void set_movable_position(int x, int y);
    void spawn_entity(uint16_t entity_id, int x, int y, const std::string& name,
                      Race race = static_cast<Race>(0), PlayerClass player_class = static_cast<PlayerClass>(0));
    void despawn_entity(uint16_t entity_id);
    void move_entity(uint16_t entity_id, int x, int y);
    bool get_movable_position(int& x, int& y) const;
    int movable_foot_y() const;

    void set_movable_src_y(int y);
    void step_movable_src_x(int step, int frame_count);
    void set_anchor_src_y(int y);
    void set_entity_src_y(uint16_t entity_id, int body_src_y, int head_src_y);
    void step_entity_src_x(uint16_t entity_id, int step, int frame_count);
    void set_entity_alpha(uint16_t entity_id, uint8_t alpha);
    void set_movable_alpha(uint8_t alpha);
    void update_anchor_positions();
    void set_local_player_info(Race race, PlayerClass player_class);
    void set_skin_config(const SkinConfig& skin_config);

    void render(const SDL2pp::Rect& cam);
    void tick_animations(AnimationSystem& anim);

    bool hit_test_entity(int world_x, int world_y, uint16_t& out_entity_id) const;

    bool empty() const { return sprites.empty(); }
    int movable_x() const;
    int movable_y() const;
    int movable_w() const;
    int movable_h() const;

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
        uint8_t alpha = 255;
    };

    struct EntityNameRender {
        SDL2pp::Texture texture;
        int w;
        int h;
    };

    SpriteRender build_sprite_render(const SpriteConfig& sprite_config);
    SpriteRender* find_movable_sprite();
    const SpriteRender* find_movable_sprite() const;

    int clamp_x(int value, int sprite_w) const;
    int clamp_y(int value, int sprite_h) const;

    static bool is_visible(const SpriteRender& s, const SDL2pp::Rect& cam);

    SDL2pp::Renderer& renderer;
    TTF_Font* name_font;
    std::vector<SpriteRender> sprites;
    std::vector<SpriteConfig> entity_part_configs;
    std::unordered_map<uint16_t, std::vector<SpriteRender>> entity_sprites;
    std::unordered_map<uint16_t, EntityNameRender> entity_name_render;
    SkinConfig skin_config;
    int window_w;
    int window_h;
    bool has_tilemap;
    int map_px_w;
    int map_px_h;
};

#endif
