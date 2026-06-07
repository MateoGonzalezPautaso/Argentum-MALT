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

struct OverlayEffect {
    std::vector<SDL2pp::Texture> frames;
    SDL2pp::Rect dst;
    bool animated = true;
    uint32_t frame_ms = 70;
    std::size_t current_frame = 0;
    uint32_t last_ticks = 0;
    bool active = false;
};

struct EquipOverlay {
    std::vector<SDL2pp::Texture> frames;
    bool active = false;
    int offset_y = 0;
    bool static_frame = false;
};

class SpriteRenderer {
public:
    SpriteRenderer(SDL2pp::Renderer& renderer, TTF_Font* name_font, int window_w, int window_h,
                   bool has_tilemap, int map_px_w, int map_px_h);

    void load_sprites(const std::vector<SpriteConfig>& sprites_config);
    void set_map_bounds(bool has_tilemap, int map_px_w, int map_px_h);
    void set_name_font(TTF_Font* font) { name_font = font; }

    void set_movable_position(int x, int y);
    void spawn_entity(uint16_t entity_id, int x, int y, const std::string& name,
                      Race race = static_cast<Race>(0),
                      PlayerClass player_class = static_cast<PlayerClass>(0));
    void despawn_entity(uint16_t entity_id);
    void clear_all_entities();
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

    void update_equipment_overlay(uint8_t slot, const std::string& path, int offset_y = 0, bool static_frame = false);
    void clear_equipment_overlay(uint8_t slot);
    void set_body_sprite(const std::string& path);
    void reset_body_sprite();

    void set_direction_src_y(int down, int up, int left, int right);

    void render(const SDL2pp::Rect& cam);
    void render_entity_names(const SDL2pp::Rect& cam);
    void tick_animations(AnimationSystem& anim);

    bool hit_test_entity(int world_x, int world_y, uint16_t& out_entity_id) const;

    void load_damage_overlay();
    void trigger_damage_overlay_at(int world_x, int world_y);
    bool get_entity_world_position(uint16_t entity_id, int& x, int& y) const;
    void tick_overlays(const AnimationSystem& anim);
    void render_overlays(const SDL2pp::Rect& cam);

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
    SpriteConfig resolve_entity_skin(const SpriteConfig& config, Race race,
                                     PlayerClass player_class) const;
    std::vector<SpriteRender> build_entity_parts(int x, int y, Race race, PlayerClass player_class);
    void position_anchored_parts(std::vector<SpriteRender>& parts);
    void create_entity_name_label(uint16_t entity_id, const std::string& name);
    SpriteRender* find_movable_sprite();
    const SpriteRender* find_movable_sprite() const;

    int clamp_x(int value, int sprite_w) const;
    int clamp_y(int value, int sprite_h) const;

    static bool is_visible(const SpriteRender& s, const SDL2pp::Rect& cam);
    static void advance_src_x(SpriteRender& sprite, int step, int frame_count);

    struct Drawable {
        SDL2pp::Texture* texture;
        SDL2pp::Rect* src;
        SDL2pp::Rect dst;
        int foot_y;
        uint8_t alpha;
    };

    void append_sprite_drawables(std::vector<SpriteRender>& src, const SDL2pp::Rect& cam,
                                 std::vector<Drawable>& out);
    void sort_and_render_drawables(std::vector<Drawable>& drawables);

    SDL2pp::Renderer& renderer;
    TTF_Font* name_font;
    std::vector<SpriteRender> sprites;
    std::vector<SpriteConfig> entity_part_configs;
    std::unordered_map<uint16_t, std::vector<SpriteRender>> entity_sprites;
    std::unordered_map<uint16_t, EntityNameRender> entity_name_render;
    SkinConfig skin_config;
    std::vector<OverlayEffect> overlays;
    EquipOverlay equip_overlays_[EQUIP_SLOT_COUNT];
    SDL2pp::Rect static_cache_[EQUIP_SLOT_COUNT];
    std::string default_body_path_;
    int dir_src_y_down_ = 0;
    int dir_src_y_up_ = 48;
    int dir_src_y_left_ = 96;
    int dir_src_y_right_ = 144;
    int window_w;
    int window_h;
    bool has_tilemap;
    int map_px_w;
    int map_px_h;
};

#endif
