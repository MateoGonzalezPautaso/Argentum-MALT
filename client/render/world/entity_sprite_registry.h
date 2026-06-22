#ifndef CLIENT_ENTITY_SPRITE_REGISTRY_H
#define CLIENT_ENTITY_SPRITE_REGISTRY_H

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <SDL2pp/SDL2pp.hh>
#include <SDL_ttf.h>

#include "../../../common/messages.h"
#include "../../config/config.h"

class AnimationSystem;

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

struct EquipOverlay {
    std::vector<SDL2pp::Texture> frames;
    bool active = false;
    int offset_y = 0;
    bool static_frame = false;
};

struct Drawable {
    SDL2pp::Texture* texture;
    SDL2pp::Rect src;
    SDL2pp::Rect dst;
    bool use_src = false;
    int foot_y;
    uint8_t alpha;
};


class EntitySpriteRegistry {
public:
    EntitySpriteRegistry(SDL2pp::Renderer& renderer, TTF_Font* name_font, int window_w,
                         int window_h, bool has_tilemap, int map_px_w, int map_px_h);

    // Called after load_sprites() populates the template configs.
    void set_part_templates(const std::vector<SpriteConfig>& configs, const SkinConfig& skin);
    void set_skin_config(const SkinConfig& cfg);
    void set_map_bounds(bool has_tilemap, int map_px_w, int map_px_h);
    void set_name_font(TTF_Font* font);
    void set_direction_src_y(int down, int up, int left, int right);
    void set_walk_anim_timeout(uint32_t ms);
    void set_local_clan_name(const std::string& name);

    void add_entity(uint16_t id, int x, int y, const std::string& name, Race race,
                    PlayerClass pc, uint16_t sprite_id);
    void remove_entity(uint16_t id);
    void clear_entities();
    void move_entity(uint16_t id, int x, int y);

    void set_entity_src_y(uint16_t id, int body_src_y, int head_src_y);
    void advance_entity_src_x(uint16_t id, int step, int frame_count);
    void mark_entity_moved(uint16_t id);
    void tick_animations(AnimationSystem& anim);

    void set_entity_clan_name(uint16_t id, const std::string& name);
    void set_entity_clan_by_username(const std::string& username, const std::string& clan);
    void set_entity_alpha(uint16_t id, uint8_t alpha);

    void update_entity_equipment_overlay(uint16_t id, uint8_t slot, const std::string& path,
                                         int offset_y, bool static_frame);
    void clear_entity_equipment_overlay(uint16_t id, uint8_t slot);
    void set_entity_body_sprite(uint16_t id, const std::string& path);
    void reset_entity_body_sprite(uint16_t id);

    bool hit_test_entity(int world_x, int world_y, uint16_t& out_id) const;
    bool get_entity_world_position(uint16_t id, int& x, int& y) const;
    bool is_npc(uint16_t id) const;

    // Collects entity drawables for the shared Z-sorted render pass.
    void collect_drawables(const SDL2pp::Rect& cam, std::vector<Drawable>& out);
    void render_entity_names(const SDL2pp::Rect& cam);

    // Shared builders — also used by SpriteRenderer for local player sprites.
    static SpriteRender build_sprite_render(SDL2pp::Renderer& renderer, const SpriteConfig& cfg);
    static SpriteConfig resolve_entity_skin(const SkinConfig& skin, const SpriteConfig& cfg,
                                            Race race, PlayerClass pc, uint16_t sprite_id);

private:
    struct EntityNameRender {
        SDL2pp::Texture texture;
        int w;
        int h;
    };

    struct EntityEquipState {
        EquipOverlay overlays[EQUIP_SLOT_COUNT];
        SDL2pp::Rect static_cache[EQUIP_SLOT_COUNT];
        std::string default_body_path;
    };

    struct RenderedEntity {
        std::vector<SpriteRender> parts;
        std::optional<EntityNameRender> name_label;
        EntityEquipState equip;
        uint16_t sprite_id = 0;
        int frame_w = 0;
        int frame_h = 0;
        int base_src_x = 0;
        int base_src_y = 0;
        int speed = 0;
        int move_counter = 0;
        uint32_t last_move_tick = 0;
        std::string clan_name;
        std::string username;
    };

    std::vector<SpriteRender> build_entity_parts(int x, int y, Race race, PlayerClass pc,
                                                  uint16_t sprite_id);
    void reposition_anchored_parts(std::vector<SpriteRender>& parts);
    void create_entity_name_label(uint16_t id, const std::string& name);
    SpriteRender* find_movable(uint16_t id);
    const SpriteRender* find_movable(uint16_t id) const;

    void append_sprite_drawables(std::vector<SpriteRender>& src, const SDL2pp::Rect& cam,
                                 std::vector<Drawable>& out);
    void append_equip_overlay_drawables(const SpriteRender& body, EquipOverlay* overlays,
                                        SDL2pp::Rect* cache, const SDL2pp::Rect& cam,
                                        std::vector<Drawable>& out);
    static bool is_visible(const SpriteRender& s, const SDL2pp::Rect& cam);
    static void advance_src_x(SpriteRender& sprite, int step, int frame_count);
    int clamp_x(int value, int sprite_w) const;
    int clamp_y(int value, int sprite_h) const;

    SDL2pp::Renderer& renderer;
    TTF_Font* name_font;
    std::vector<SpriteConfig> entity_part_configs;
    SkinConfig skin_config;
    std::unordered_map<uint16_t, RenderedEntity> entities;
    std::string local_clan_name;
    uint32_t walk_anim_timeout_ms = 600;
    int dir_src_y_down = 0;
    int dir_src_y_up = 48;
    int dir_src_y_left = 96;
    int dir_src_y_right = 144;
    int window_w;
    int window_h;
    bool has_tilemap;
    int map_px_w;
    int map_px_h;
};

#endif  // CLIENT_ENTITY_SPRITE_REGISTRY_H
