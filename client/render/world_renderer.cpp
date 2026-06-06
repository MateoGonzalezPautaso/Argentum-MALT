#include "world_renderer.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include <SDL2/SDL.h>

#include "texture_loader.h"

WorldRenderer::WorldRenderer(SDL2pp::Renderer& renderer, const BackgroundConfig& background,
                             const TilemapConfig& tilemap,
                             const std::vector<SpriteConfig>& sprites_config,
                             const ViewportConfig& viewport_cfg, const FontConfig& font_cfg,
                             const SkinConfig& skin_config):
        renderer(renderer),
        background_texture(renderer, texture::load_surface(background.path)),
        background_rect(background.x, background.y, background.width, background.height),
        window_w(viewport_cfg.logical_w),
        window_h(viewport_cfg.logical_h),
        tilemap_renderer(renderer),
        prop_renderer(renderer),
        camera(viewport_cfg, 0, 0, false, window_w, window_h),
        sprite_renderer(renderer, nullptr, window_w, window_h, false, 0, 0) {
    name_font = TTF_OpenFont(font_cfg.path.c_str(), font_cfg.name_size);
    if (!name_font) {
        throw std::runtime_error(std::string("TTF_OpenFont failed: ") + TTF_GetError());
    }

    sprite_renderer.set_name_font(name_font);

    tilemap_renderer.load(tilemap);

    const bool has_tm = tilemap_renderer.is_loaded();
    const int mpw = tilemap_renderer.pixel_width();
    const int mph = tilemap_renderer.pixel_height();

    camera.reconfigure(mpw, mph, has_tm);
    sprite_renderer.set_map_bounds(has_tm, mpw, mph);

    if (has_tm) {
        prop_renderer.load(tilemap, tilemap_renderer.tile_size());
    }

    sprite_renderer.load_sprites(sprites_config);
    sprite_renderer.load_damage_overlay();
    sprite_renderer.load_spell_sheets();
    sprite_renderer.set_skin_config(skin_config);

    if (sprite_renderer.empty()) {
        throw std::runtime_error("No sprites to render");
    }
}

void WorldRenderer::load_map(const TilemapConfig& tilemap) {
    tilemap_renderer.load(tilemap);

    const bool has_tm = tilemap_renderer.is_loaded();
    const int mpw = tilemap_renderer.pixel_width();
    const int mph = tilemap_renderer.pixel_height();

    camera.reconfigure(mpw, mph, has_tm);
    sprite_renderer.set_map_bounds(has_tm, mpw, mph);

    if (has_tm) {
        prop_renderer.load(tilemap, tilemap_renderer.tile_size());
    }
}

void WorldRenderer::clear_entities() { sprite_renderer.clear_all_entities(); }

WorldRenderer::~WorldRenderer() {
    if (name_font) {
        TTF_CloseFont(name_font);
    }
}

void WorldRenderer::render_background_fallback(const SDL2pp::Rect& cam) {
    (void)cam;
    const SDL2pp::Rect gameplay_bg(0, 0, camera.viewport().GetW(), camera.viewport().GetH());
    renderer.Copy(background_texture, SDL2pp::NullOpt, gameplay_bg);
}

void WorldRenderer::render() {
    renderer.SetViewport(camera.viewport());

    const SDL2pp::Rect cam =
            camera.compute_rect(sprite_renderer.movable_x(), sprite_renderer.movable_y(),
                                sprite_renderer.movable_w(), sprite_renderer.movable_h());

    if (tilemap_renderer.is_loaded()) {
        tilemap_renderer.render(cam);
    } else {
        render_background_fallback(cam);
    }

    const int foot_y = sprite_renderer.movable_foot_y();
    prop_renderer.render_behind(cam, foot_y);

    const uint32_t now = SDL_GetTicks();
    anim_system.set_now(now);
    sprite_renderer.tick_animations(anim_system);
    sprite_renderer.tick_overlays(anim_system);
    prop_renderer.tick_animations(anim_system);
    sprite_renderer.update_anchor_positions();
    sprite_renderer.render(cam);
    sprite_renderer.render_overlays(cam);
    prop_renderer.render_front(cam, foot_y);
    sprite_renderer.render_entity_names(cam);

    if (show_hitboxes_) {
        prop_renderer.render_hitboxes(cam);
    }

    renderer.SetViewport(SDL2pp::NullOpt);
}

void WorldRenderer::set_movable_position(int x, int y) {
    sprite_renderer.set_movable_position(x, y);
}

void WorldRenderer::spawn_entity(uint16_t entity_id, int x, int y, const std::string& name,
                                 Race race, PlayerClass player_class) {
    sprite_renderer.spawn_entity(entity_id, x, y, name, race, player_class);
}

void WorldRenderer::set_local_player_info(Race race, PlayerClass player_class) {
    sprite_renderer.set_local_player_info(race, player_class);
}

void WorldRenderer::despawn_entity(uint16_t entity_id) {
    sprite_renderer.despawn_entity(entity_id);
}

void WorldRenderer::move_entity(uint16_t entity_id, int x, int y) {
    sprite_renderer.move_entity(entity_id, x, y);
}

bool WorldRenderer::get_movable_position(int& x, int& y) const {
    return sprite_renderer.get_movable_position(x, y);
}

int WorldRenderer::movable_w() const { return sprite_renderer.movable_w(); }

int WorldRenderer::movable_h() const { return sprite_renderer.movable_h(); }

void WorldRenderer::get_camera_offset(int& x, int& y) const {
    x = camera.offset_x();
    y = camera.offset_y();
}

bool WorldRenderer::screen_to_world(int screen_x, int screen_y, int& world_x, int& world_y) const {
    return camera.screen_to_world(screen_x, screen_y, world_x, world_y);
}

bool WorldRenderer::hit_test_entity(int world_x, int world_y, uint16_t& out_entity_id) const {
    return sprite_renderer.hit_test_entity(world_x, world_y, out_entity_id);
}

bool WorldRenderer::hit_test_prop(int world_x, int world_y, std::string& out_prop_name) const {
    return prop_renderer.hit_test_prop(world_x, world_y, out_prop_name);
}

void WorldRenderer::set_movable_src_y(int y) { sprite_renderer.set_movable_src_y(y); }

void WorldRenderer::step_movable_src_x(int step, int frame_count) {
    sprite_renderer.step_movable_src_x(step, frame_count);
}

void WorldRenderer::set_anchor_src_y(int y) { sprite_renderer.set_anchor_src_y(y); }

void WorldRenderer::set_entity_src_y(uint16_t entity_id, int body_src_y, int head_src_y) {
    sprite_renderer.set_entity_src_y(entity_id, body_src_y, head_src_y);
}

void WorldRenderer::step_entity_src_x(uint16_t entity_id, int step, int frame_count) {
    sprite_renderer.step_entity_src_x(entity_id, step, frame_count);
}

void WorldRenderer::set_entity_alpha(uint16_t entity_id, uint8_t alpha) {
    sprite_renderer.set_entity_alpha(entity_id, alpha);
}

void WorldRenderer::set_movable_alpha(uint8_t alpha) { sprite_renderer.set_movable_alpha(alpha); }

void WorldRenderer::trigger_damage_overlay_at(int world_x, int world_y) {
    sprite_renderer.trigger_damage_overlay_at(world_x, world_y);
}

void WorldRenderer::trigger_spell_overlay_at(int world_x, int world_y) {
    sprite_renderer.trigger_spell_overlay_at(world_x, world_y);
}

void WorldRenderer::trigger_target_spell_overlay_at(int world_x, int world_y) {
    sprite_renderer.trigger_target_spell_overlay_at(world_x, world_y);
}

void WorldRenderer::trigger_missile_overlay_at(int world_x, int world_y) {
    sprite_renderer.trigger_missile_overlay_at(world_x, world_y);
}

bool WorldRenderer::get_entity_world_position(uint16_t entity_id, int& x, int& y) const {
    return sprite_renderer.get_entity_world_position(entity_id, x, y);
}
