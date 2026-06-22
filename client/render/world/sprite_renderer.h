#ifndef CLIENT_SPRITE_RENDERER_H
#define CLIENT_SPRITE_RENDERER_H

#include <cstdint>
#include <string>
#include <vector>

#include <SDL2pp/SDL2pp.hh>
#include <SDL_ttf.h>

#include "../../../common/messages.h"
#include "../../config/config.h"
#include "effect_overlay_system.h"
#include "entity_sprite_registry.h"

class AnimationSystem;

class SpriteRenderer {
public:
    SpriteRenderer(SDL2pp::Renderer& renderer, TTF_Font* name_font, int window_w, int window_h,
                   bool has_tilemap, int map_px_w, int map_px_h);

    void load_sprites(const std::vector<SpriteConfig>& sprites_config);
    void set_map_bounds(bool has_tilemap, int map_px_w, int map_px_h);
    void set_name_font(TTF_Font* font);

    void move_movable(int x, int y);
    void add_entity(uint16_t entity_id, int x, int y, const std::string& name,
                    Race race = Race::HUMAN, PlayerClass player_class = PlayerClass::MAGE,
                    uint16_t sprite_id = 0);
    void remove_entity(uint16_t entity_id);
    void clear_entities();
    void move_entity(uint16_t entity_id, int x, int y);
    bool get_movable_position(int& x, int& y) const;
    int movable_foot_y() const;

    void set_movable_src_y(int y);
    void advance_movable_src_x(int step, int frame_count);
    void set_anchor_src_y(int y);
    void set_entity_src_y(uint16_t entity_id, int body_src_y, int head_src_y);
    void advance_entity_src_x(uint16_t entity_id, int step, int frame_count);
    void mark_entity_moved(uint16_t entity_id);
    void set_local_clan_name(const std::string& name);
    void set_entity_clan_name(uint16_t entity_id, const std::string& name);
    void set_entity_clan_by_username(const std::string& username, const std::string& clan);
    void set_entity_alpha(uint16_t entity_id, uint8_t alpha);
    void set_movable_alpha(uint8_t alpha);
    void reposition_anchored_sprites();
    void rebuild_local_player_sprites(Race race, PlayerClass player_class);
    void set_skin_config(const SkinConfig& skin_config);

    void update_equipment_overlay(uint8_t slot, const std::string& path, int offset_y = 0,
                                  bool static_frame = false);
    void clear_equipment_overlay(uint8_t slot);
    void update_entity_equipment_overlay(uint16_t entity_id, uint8_t slot, const std::string& path,
                                         int offset_y = 0, bool static_frame = false);
    void clear_entity_equipment_overlay(uint16_t entity_id, uint8_t slot);
    void set_entity_body_sprite(uint16_t entity_id, const std::string& path);
    void reset_entity_body_sprite(uint16_t entity_id);
    void set_body_sprite(const std::string& path);
    void reset_body_sprite();

    void set_direction_src_y(int down, int up, int left, int right);

    void render(const SDL2pp::Rect& cam);
    void render_entity_names(const SDL2pp::Rect& cam);
    void tick_animations(AnimationSystem& anim);

    bool hit_test_entity(int world_x, int world_y, uint16_t& out_entity_id) const;

    void load_damage_overlay(const DamageOverlayConfig& cfg);
    void trigger_damage_effect(int world_x, int world_y);
    void load_spell_sheets(const std::vector<SpellSheetConfig>& sheets);
    void trigger_spell_effect(uint8_t effect_type, int world_x, int world_y);
    void set_walk_anim_timeout(uint32_t ms);
    bool get_entity_world_position(uint16_t entity_id, int& x, int& y) const;
    void advance_overlays();
    void render_overlays(const SDL2pp::Rect& cam);

    bool empty() const { return sprites.empty(); }
    int movable_x() const;
    int movable_y() const;
    int movable_w() const;
    int movable_h() const;

private:
    SpriteRender* find_movable_sprite();
    const SpriteRender* find_movable_sprite() const;

    int clamp_x(int value, int sprite_w) const;
    int clamp_y(int value, int sprite_h) const;

    static bool is_visible(const SpriteRender& s, const SDL2pp::Rect& cam);
    static void advance_src_x(SpriteRender& sprite, int step, int frame_count);

    void append_sprite_drawables(std::vector<SpriteRender>& src, const SDL2pp::Rect& cam,
                                 std::vector<Drawable>& out);
    void append_equip_overlay_drawables(const SpriteRender& body, EquipOverlay* overlays,
                                        SDL2pp::Rect* static_cache, const SDL2pp::Rect& cam,
                                        std::vector<Drawable>& out);
    void sort_and_render_drawables(std::vector<Drawable>& drawables);

    SDL2pp::Renderer& renderer;
    TTF_Font* name_font;
    std::vector<SpriteRender> sprites;
    std::vector<SpriteConfig> entity_part_configs;
    SkinConfig skin_config;
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

    EntitySpriteRegistry entity_registry_;
    EffectOverlaySystem effects_;
};

#endif
