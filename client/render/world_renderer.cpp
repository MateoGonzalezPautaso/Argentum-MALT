#include "world_renderer.h"

#include <stdexcept>

#include <SDL2/SDL.h>

#include "texture_loader.h"

WorldRenderer::WorldRenderer(SDL2pp::Renderer& renderer, const BackgroundConfig& background,
                             const ViewportConfig& viewport_cfg, const FontConfig& font_cfg):
        renderer(renderer),
        background_texture(renderer, texture::load_surface(background.path)),
        background_rect(background.x, background.y, background.width, background.height),
        window_w(viewport_cfg.logical_w),
        window_h(viewport_cfg.logical_h),
        tilemap_renderer(renderer),
        prop_renderer(renderer),
        ground_item_renderer(renderer, item_sprites_),
        camera(viewport_cfg, 0, 0, false, window_w, window_h),
        sprite_renderer(renderer, nullptr, window_w, window_h, false, 0, 0) {
    name_font = TTF_OpenFont(font_cfg.path.c_str(), font_cfg.name_size);
    if (!name_font) {
        throw std::runtime_error(std::string("TTF_OpenFont failed: ") + TTF_GetError());
    }

    sprite_renderer.set_name_font(name_font);
}

void WorldRenderer::load_tilemap_data(const TilemapConfig& tilemap) {
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

void WorldRenderer::load_assets(const TilemapConfig& tilemap,
                                const std::vector<SpriteConfig>& sprites_config,
                                const SkinConfig& skin_config,
                                const std::unordered_map<uint8_t, ItemSpriteDef>& item_sprites,
                                const GroundItemConfig& ground_item_cfg,
                                const DamageOverlayConfig& damage_overlay,
                                const std::vector<SpellSheetConfig>& spell_sheets,
                                uint32_t walk_anim_timeout_ms) {
    item_sprites_ = item_sprites;
    ground_item_renderer.apply_config(ground_item_cfg);
    load_tilemap_data(tilemap);

    sprite_renderer.load_sprites(sprites_config);
    sprite_renderer.load_damage_overlay(damage_overlay);
    sprite_renderer.load_spell_sheets(spell_sheets);
    sprite_renderer.set_walk_anim_timeout(walk_anim_timeout_ms);
    sprite_renderer.set_skin_config(skin_config);

    if (sprite_renderer.empty()) {
        throw std::runtime_error("No sprites to render");
    }
}

void WorldRenderer::load_map(const TilemapConfig& tilemap) {
    load_tilemap_data(tilemap);
}

WorldRenderer::~WorldRenderer() {
    if (name_font) {
        TTF_CloseFont(name_font);
    }
}

void WorldRenderer::render_background_fallback() {
    const SDL2pp::Rect gameplay_bg(0, 0, camera.viewport().GetW(), camera.viewport().GetH());
    renderer.Copy(background_texture, SDL2pp::NullOpt, gameplay_bg);
}

void WorldRenderer::render() {
    // SDL_RenderSetViewport multiplies its input by the scale factor but does NOT add
    // the centering offset from SDL_RenderSetLogicalSize. We read the current base
    // viewport (which contains the centering offset in scaled-logical units) and add
    // the game area offset so that SDL's internal multiply produces correct physical coords.
    renderer.SetLogicalSize(window_w, window_h);
    SDL_Rect base_vp;
    SDL_RenderGetViewport(renderer.Get(), &base_vp);
    const SDL2pp::Rect& gv = camera.viewport();
    SDL_Rect adjusted_gv = {
        base_vp.x + gv.GetX(),
        base_vp.y + gv.GetY(),
        gv.GetW(),
        gv.GetH(),
    };
    SDL_RenderSetViewport(renderer.Get(), &adjusted_gv);

    const SDL2pp::Rect cam =
            camera.compute_rect(sprite_renderer.movable_x(), sprite_renderer.movable_y(),
                                sprite_renderer.movable_w(), sprite_renderer.movable_h());

    if (tilemap_renderer.is_loaded()) {
        tilemap_renderer.render(cam);
    } else {
        render_background_fallback();
    }

    const int foot_y = sprite_renderer.movable_foot_y();
    prop_renderer.render_behind(cam, foot_y);
    ground_item_renderer.render(cam);

    const uint32_t now = SDL_GetTicks();
    anim_system.set_now(now);
    sprite_renderer.tick_animations(anim_system);
    sprite_renderer.advance_overlays();
    prop_renderer.tick_animations(anim_system);
    sprite_renderer.update_anchor_positions();
    sprite_renderer.render(cam);
    sprite_renderer.render_overlays(cam);
    prop_renderer.render_front(cam, foot_y);
    sprite_renderer.render_entity_names(cam);

    if (show_hitboxes_) {
        prop_renderer.render_hitboxes(cam);
    }

    renderer.SetLogicalSize(window_w, window_h);
}

void WorldRenderer::get_camera_offset(int& x, int& y) const {
    x = camera.offset_x();
    y = camera.offset_y();
}

bool WorldRenderer::screen_to_world(int screen_x, int screen_y, int& world_x, int& world_y) const {
    return camera.screen_to_world(screen_x, screen_y, world_x, world_y);
}

bool WorldRenderer::hit_test_prop(int world_x, int world_y, std::string& out_prop_name) const {
    return prop_renderer.hit_test_prop(world_x, world_y, out_prop_name);
}

