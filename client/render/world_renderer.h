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
#include "world/ground_item_renderer.h"
#include "world/prop_renderer.h"
#include "world/sprite_renderer.h"
#include "world/tilemap_renderer.h"

class WorldRenderer {
public:
    WorldRenderer(SDL2pp::Renderer& renderer, const BackgroundConfig& background,
                  const ViewportConfig& viewport_cfg, const FontConfig& font_cfg);
    void load_assets(const TilemapConfig& tilemap, const std::vector<SpriteConfig>& sprites_config,
                     const SkinConfig& skin_config,
                     const std::unordered_map<uint8_t, ItemSpriteDef>& item_sprites,
                     const GroundItemConfig& ground_item_cfg = {},
                     const DamageOverlayConfig& damage_overlay = {},
                     const std::vector<SpellSheetConfig>& spell_sheets = {},
                     uint32_t walk_anim_timeout_ms = 600);
    ~WorldRenderer();

    void render();

    void load_map(const TilemapConfig& tilemap);
    void get_camera_offset(int& x, int& y) const;
    bool screen_to_world(int screen_x, int screen_y, int& world_x, int& world_y) const;
    bool hit_test_prop(int world_x, int world_y, std::string& out_prop_name) const;
    void set_show_hitboxes(bool v) { show_hitboxes_ = v; }
    bool get_show_hitboxes() const { return show_hitboxes_; }

    SpriteRenderer& sprites() { return sprite_renderer; }
    const SpriteRenderer& sprites() const { return sprite_renderer; }
    GroundItemRenderer& ground_items() { return ground_item_renderer; }
    const GroundItemRenderer& ground_items() const { return ground_item_renderer; }

private:
    void load_tilemap_data(const TilemapConfig& tilemap);
    void render_background_fallback(const SDL2pp::Rect& cam);

    SDL2pp::Renderer& renderer;
    SDL2pp::Texture background_texture;
    SDL2pp::Rect background_rect;
    TTF_Font* name_font = nullptr;
    int window_w;
    int window_h;

    std::unordered_map<uint8_t, ItemSpriteDef> item_sprites_;
    TilemapRenderer tilemap_renderer;
    PropRenderer prop_renderer;
    GroundItemRenderer ground_item_renderer;
    Camera camera;
    SpriteRenderer sprite_renderer;
    AnimationSystem anim_system;
    bool show_hitboxes_ = false;
};

#endif
