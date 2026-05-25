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
                  const TilemapConfig& tilemap, const std::vector<SpriteConfig>& sprites_config,
                  const ViewportConfig& viewport_cfg, const FontConfig& font_cfg);
    ~WorldRenderer();

    void render();

    void set_movable_position(int x, int y);
    void spawn_entity(uint16_t entity_id, int x, int y, const std::string& name);
    void despawn_entity(uint16_t entity_id);
    void move_entity(uint16_t entity_id, int x, int y);
    bool get_movable_position(int& x, int& y) const;
    void get_camera_offset(int& x, int& y) const;
    bool screen_to_world(int screen_x, int screen_y, int& world_x, int& world_y) const;
    bool hit_test_entity(int world_x, int world_y, uint16_t& out_entity_id) const;
    void set_movable_src_y(int y);
    void step_movable_src_x(int step, int frame_count);
    void set_anchor_src_y(int y);
    void set_entity_src_y(uint16_t entity_id, int body_src_y, int head_src_y);
    void step_entity_src_x(uint16_t entity_id, int step, int frame_count);
    void set_entity_alpha(uint16_t entity_id, uint8_t alpha);
    void set_movable_alpha(uint8_t alpha);
    void set_show_hitboxes(bool v) { show_hitboxes_ = v; }
    bool get_show_hitboxes() const { return show_hitboxes_; }

private:
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
