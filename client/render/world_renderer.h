#ifndef CLIENT_WORLD_RENDERER_H
#define CLIENT_WORLD_RENDERER_H

#include <cstdint>
#include <string>
#include <vector>

#include <SDL2pp/SDL2pp.hh>
#include <SDL_ttf.h>

#include "../config/config.h"
#include "world/animation_system.h"
#include "world/camera.h"
#include "world/prop_renderer.h"
#include "world/sprite_renderer.h"
#include "world/tilemap_renderer.h"

class WorldRenderer {
public:
    WorldRenderer(SDL2pp::Renderer& renderer, const BackgroundConfig& background,
                  const ViewportConfig& viewport_cfg, const FontConfig& font_cfg);
    void load_assets(const TilemapConfig& tilemap, const std::vector<SpriteConfig>& sprites_config,
                     const SkinConfig& skin_config);
    ~WorldRenderer();

    void render();

    void load_map(const TilemapConfig& tilemap);
    void clear_entities();

    void set_movable_position(int x, int y);
    void spawn_entity(uint16_t entity_id, int x, int y, const std::string& name,
                      Race race = static_cast<Race>(0),
                      PlayerClass player_class = static_cast<PlayerClass>(0));
    void despawn_entity(uint16_t entity_id);
    void move_entity(uint16_t entity_id, int x, int y);
    bool get_movable_position(int& x, int& y) const;
    int movable_w() const;
    int movable_h() const;
    void get_camera_offset(int& x, int& y) const;
    bool screen_to_world(int screen_x, int screen_y, int& world_x, int& world_y) const;
    bool hit_test_entity(int world_x, int world_y, uint16_t& out_entity_id) const;
    bool hit_test_prop(int world_x, int world_y, std::string& out_prop_name) const;
    void set_movable_src_y(int y);
    void step_movable_src_x(int step, int frame_count);
    void set_anchor_src_y(int y);
    void set_entity_src_y(uint16_t entity_id, int body_src_y, int head_src_y);
    void step_entity_src_x(uint16_t entity_id, int step, int frame_count);
    void set_entity_alpha(uint16_t entity_id, uint8_t alpha);
    void set_movable_alpha(uint8_t alpha);
    void set_local_player_info(Race race, PlayerClass player_class);
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
    void trigger_damage_overlay_at(int world_x, int world_y);
    void trigger_spell_effect(uint8_t effect_type, int world_x, int world_y);
    bool get_entity_world_position(uint16_t entity_id, int& x, int& y) const;
    void set_show_hitboxes(bool v) { show_hitboxes_ = v; }
    bool get_show_hitboxes() const { return show_hitboxes_; }

private:
    void load_tilemap_data(const TilemapConfig& tilemap);
    void render_background_fallback(const SDL2pp::Rect& cam);

    SDL2pp::Renderer& renderer;
    SDL2pp::Texture background_texture;
    SDL2pp::Rect background_rect;
    TTF_Font* name_font = nullptr;
    int window_w;
    int window_h;

    TilemapRenderer tilemap_renderer;
    PropRenderer prop_renderer;
    Camera camera;
    SpriteRenderer sprite_renderer;
    AnimationSystem anim_system;
    bool show_hitboxes_ = false;
};

#endif
