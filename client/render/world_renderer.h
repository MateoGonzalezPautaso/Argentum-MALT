#ifndef CLIENT_WORLD_RENDERER_H
#define CLIENT_WORLD_RENDERER_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <SDL2pp/SDL2pp.hh>
#include <SDL_ttf.h>

#include "../config/config.h"

class WorldRenderer {
private:
    struct TileSrcInfo {
        SDL2pp::Rect src;
        std::string atlas_path;
    };

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

    struct EntityNameRender {
        SDL2pp::Texture texture;
        int w;
        int h;
    };

    struct PropRender {
        std::vector<SDL2pp::Texture> frames;
        SDL2pp::Rect src;
        int display_w = 0;
        int display_h = 0;
        uint32_t frame_ms = 0;
        std::size_t current_frame = 0;
        uint32_t last_ticks = 0;
        int hitbox_x = 0; //PARA HITBOX DEBUG ONLY - BORRAR EN PROD
        int hitbox_y = 0; //PARA HITBOX DEBUG ONLY - BORRAR EN PROD
        int hitbox_w = 0; //PARA HITBOX DEBUG ONLY - BORRAR EN PROD
        int hitbox_h = 0; //PARA HITBOX DEBUG ONLY - BORRAR EN PROD
    };

    SDL2pp::Renderer& renderer;
    SDL2pp::Texture background_texture;
    SDL2pp::Rect background_rect;
    SDL2pp::Rect game_viewport;
    std::unordered_map<std::string, SDL2pp::Texture> tilemap_textures_;
    bool has_tilemap = false;
    int tile_size = 128;
    std::vector<std::vector<TileSrcInfo>> tilemap_tiles_;
    std::vector<std::vector<PropRender>> prop_tiles_;
    int map_px_w = 0;
    int map_px_h = 0;
    std::vector<SpriteRender> sprites;
    TTF_Font* name_font = nullptr;
    std::unordered_map<uint16_t, std::vector<SpriteRender>> entity_sprites;
    std::unordered_map<uint16_t, EntityNameRender> entity_name_render;
    std::vector<SpriteConfig> entity_part_configs;
    int window_w;
    int window_h;
    bool show_hitboxes_ = false; //PARA HITBOX DEBUG ONLY - BORRAR EN PROD

public:
    void set_show_hitboxes(bool v) { show_hitboxes_ = v; } //PARA HITBOX DEBUG ONLY - BORRAR EN PROD
    bool get_show_hitboxes() const { return show_hitboxes_; } //PARA HITBOX DEBUG ONLY - BORRAR EN PROD
    WorldRenderer(SDL2pp::Renderer& renderer, const BackgroundConfig& background,
                  const TilemapConfig& tilemap, const std::vector<SpriteConfig>& sprites_config,
                  int window_w, int window_h);
    ~WorldRenderer();

    void render();
    void set_movable_position(int x, int y);
    void spawn_entity(uint16_t entity_id, int x, int y, const std::string& name);
    void despawn_entity(uint16_t entity_id);
    void move_entity(uint16_t entity_id, int x, int y);
    bool get_movable_position(int& x, int& y) const;
    void get_camera_offset(int& x, int& y) const;
    bool screen_to_world(int screen_x, int screen_y, int& world_x, int& world_y) const;
    void set_movable_src_y(int y);
    void step_movable_src_x(int step, int frame_count);
    void set_anchor_src_y(int y);
    void set_entity_src_y(uint16_t entity_id, int body_src_y, int head_src_y);
    void step_entity_src_x(uint16_t entity_id, int step, int frame_count);

private:
    void init_tilemap(const TilemapConfig& tilemap);
    void init_props(const TilemapConfig& tilemap);
    void init_sprites(const std::vector<SpriteConfig>& sprites_config);
    SpriteRender build_sprite_render(const SpriteConfig& sprite_config);
    SDL2pp::Rect camera_rect() const;
    void render_tilemap_or_background(const SDL2pp::Rect& cam);
    void render_props(const SDL2pp::Rect& cam, int player_foot_y, bool behind);
    void render_props_hitboxes(const SDL2pp::Rect& cam); //PARA HITBOX DEBUG ONLY - BORRAR EN PROD
    void render_sprites(const SDL2pp::Rect& cam);
    void update_animation();
    void update_anchor_positions();
    SpriteRender* find_movable_sprite();
    const SpriteRender* find_movable_sprite() const;
};

#endif
