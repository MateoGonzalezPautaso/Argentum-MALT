#include "sprite_renderer.h"

#include <algorithm>
#include <cstdint>

#include <SDL2/SDL.h>

#include "../../../common/messages.h"
#include "../gfx/geometry.h"
#include "../gfx/texture_loader.h"
#include "animation_system.h"

SpriteRenderer::SpriteRenderer(SDL2pp::Renderer& renderer, TTF_Font* name_font, int window_w,
                               int window_h, bool has_tilemap, int map_px_w, int map_px_h):
        renderer(renderer),
        name_font(name_font),
        window_w(window_w),
        window_h(window_h),
        has_tilemap(has_tilemap),
        map_px_w(map_px_w),
        map_px_h(map_px_h),
        entity_registry_(renderer, name_font, window_w, window_h, has_tilemap, map_px_w, map_px_h),
        effects_(renderer) {}

void SpriteRenderer::set_map_bounds(bool has_tm, int mpw, int mph) {
    has_tilemap = has_tm;
    map_px_w = mpw;
    map_px_h = mph;
    entity_registry_.set_map_bounds(has_tm, mpw, mph);
}

void SpriteRenderer::set_name_font(TTF_Font* font) {
    name_font = font;
    entity_registry_.set_name_font(font);
}

void SpriteRenderer::load_sprites(const std::vector<SpriteConfig>& sprites_config) {
    sprites.reserve(sprites_config.size());
    for (const auto& cfg: sprites_config) {
        SpriteRender sprite = EntitySpriteRegistry::build_sprite_render(renderer, cfg);
        if (sprite.frames.empty())
            continue;
        entity_part_configs.push_back(cfg);
        sprites.push_back(std::move(sprite));
    }
    entity_registry_.set_part_templates(entity_part_configs, skin_config);
}

void SpriteRenderer::set_skin_config(const SkinConfig& cfg) {
    skin_config = cfg;
    entity_registry_.set_skin_config(cfg);
}

int SpriteRenderer::clamp_x(int value, int sprite_w) const {
    const int max_x =
            has_tilemap ? std::max(0, map_px_w - sprite_w) : std::max(0, window_w - sprite_w);
    return std::clamp(value, 0, max_x);
}

int SpriteRenderer::clamp_y(int value, int sprite_h) const {
    const int max_y =
            has_tilemap ? std::max(0, map_px_h - sprite_h) : std::max(0, window_h - sprite_h);
    return std::clamp(value, 0, max_y);
}

bool SpriteRenderer::is_visible(const SpriteRender& s, const SDL2pp::Rect& cam) {
    return s.dst.GetX() + s.dst.GetW() > cam.GetX() &&
           s.dst.GetX() < cam.GetX() + cam.GetW() &&
           s.dst.GetY() + s.dst.GetH() > cam.GetY() &&
           s.dst.GetY() < cam.GetY() + cam.GetH();
}

void SpriteRenderer::advance_src_x(SpriteRender& sprite, int step, int frame_count) {
    if (!sprite.use_src || frame_count <= 0 || step <= 0)
        return;
    const int current_index = sprite.src.GetX() / step;
    sprite.src.SetX((current_index + 1) % frame_count * step);
}

// ── Local player movement & animation ────────────────────────────────────────

void SpriteRenderer::move_movable(int x, int y) {
    SpriteRender* sprite = find_movable_sprite();
    if (!sprite)
        return;
    sprite->dst.SetX(clamp_x(x, sprite->dst.GetW()));
    sprite->dst.SetY(clamp_y(y, sprite->dst.GetH()));
}

bool SpriteRenderer::get_movable_position(int& x, int& y) const {
    const SpriteRender* sprite = find_movable_sprite();
    if (!sprite)
        return false;
    x = sprite->dst.GetX();
    y = sprite->dst.GetY();
    return true;
}

int SpriteRenderer::movable_foot_y() const {
    const SpriteRender* sprite = find_movable_sprite();
    return sprite ? sprite->dst.GetY() + sprite->dst.GetH() : 0;
}

int SpriteRenderer::movable_x() const {
    const SpriteRender* s = find_movable_sprite();
    return s ? s->dst.GetX() : 0;
}

int SpriteRenderer::movable_y() const {
    const SpriteRender* s = find_movable_sprite();
    return s ? s->dst.GetY() : 0;
}

int SpriteRenderer::movable_w() const {
    const SpriteRender* s = find_movable_sprite();
    return s ? s->dst.GetW() : 0;
}

int SpriteRenderer::movable_h() const {
    const SpriteRender* s = find_movable_sprite();
    return s ? s->dst.GetH() : 0;
}

void SpriteRenderer::set_movable_src_y(int y) {
    SpriteRender* sprite = find_movable_sprite();
    if (!sprite || !sprite->use_src)
        return;
    sprite->src.SetY(y);
}

void SpriteRenderer::advance_movable_src_x(int step, int frame_count) {
    SpriteRender* sprite = find_movable_sprite();
    if (!sprite)
        return;
    advance_src_x(*sprite, step, frame_count);
}

void SpriteRenderer::set_anchor_src_y(int y) {
    for (auto& sprite: sprites) {
        if (!sprite.anchor_to_movable || !sprite.use_src)
            continue;
        sprite.src.SetY(y);
    }
}

void SpriteRenderer::set_movable_alpha(uint8_t alpha) {
    for (auto& sprite: sprites) {
        if (sprite.movable || sprite.anchor_to_movable)
            sprite.alpha = alpha;
    }
}

void SpriteRenderer::reposition_anchored_sprites() {
    SpriteRender* movable = find_movable_sprite();
    if (!movable)
        return;
    const int base_x = clamp_x(movable->dst.GetX(), movable->dst.GetW());
    const int base_y = clamp_y(movable->dst.GetY(), movable->dst.GetH());
    for (auto& sprite: sprites) {
        if (!sprite.anchor_to_movable)
            continue;
        sprite.dst.SetX(clamp_x(base_x + sprite.anchor_offset_x, sprite.dst.GetW()));
        sprite.dst.SetY(clamp_y(base_y + sprite.anchor_offset_y, sprite.dst.GetH()));
    }
}

void SpriteRenderer::rebuild_local_player_sprites(Race race, PlayerClass player_class) {
    if (entity_part_configs.empty())
        return;
    for (std::size_t i = 0; i < entity_part_configs.size() && i < sprites.size(); ++i) {
        const auto& cfg = entity_part_configs[i];
        SpriteConfig selected =
                EntitySpriteRegistry::resolve_entity_skin(skin_config, cfg, race, player_class, 0);
        if (cfg.movable && !cfg.anchor_to_movable)
            default_body_path_ = selected.paths.empty() ? "" : selected.paths[0];
        sprites[i] = EntitySpriteRegistry::build_sprite_render(renderer, selected);
    }
}

// ── Local player equipment ────────────────────────────────────────────────────

void SpriteRenderer::update_equipment_overlay(uint8_t slot, const std::string& path, int offset_y,
                                              bool static_frame) {
    if (slot >= EQUIP_SLOT_COUNT)
        return;
    EquipOverlay& overlay = equip_overlays_[slot];
    try {
        overlay.frames.clear();
        overlay.frames.emplace_back(renderer, texture::load_surface(path));
        overlay.active = true;
        overlay.offset_y = offset_y;
        overlay.static_frame = static_frame;
    } catch (...) {
        overlay.frames.clear();
        overlay.active = false;
    }
}

void SpriteRenderer::clear_equipment_overlay(uint8_t slot) {
    if (slot >= EQUIP_SLOT_COUNT)
        return;
    equip_overlays_[slot].frames.clear();
    equip_overlays_[slot].active = false;
}

void SpriteRenderer::set_body_sprite(const std::string& path) {
    SpriteRender* movable = find_movable_sprite();
    if (!movable || path.empty())
        return;
    try {
        movable->frames.clear();
        movable->frames.emplace_back(renderer, texture::load_surface(path));
        movable->current_frame = 0;
    } catch (...) {
        reset_body_sprite();
    }
}

void SpriteRenderer::reset_body_sprite() {
    if (!default_body_path_.empty())
        set_body_sprite(default_body_path_);
}

// ── Direction src_y ───────────────────────────────────────────────────────────

void SpriteRenderer::set_direction_src_y(int down, int up, int left, int right) {
    dir_src_y_down_ = down;
    dir_src_y_up_ = up;
    dir_src_y_left_ = left;
    dir_src_y_right_ = right;
    entity_registry_.set_direction_src_y(down, up, left, right);
}

// ── Entity delegation ─────────────────────────────────────────────────────────

void SpriteRenderer::add_entity(uint16_t id, int x, int y, const std::string& name, Race race,
                                PlayerClass pc, uint16_t sprite_id) {
    entity_registry_.add_entity(id, x, y, name, race, pc, sprite_id);
}

void SpriteRenderer::remove_entity(uint16_t id) { entity_registry_.remove_entity(id); }
void SpriteRenderer::clear_entities() { entity_registry_.clear_entities(); }
void SpriteRenderer::move_entity(uint16_t id, int x, int y) {
    entity_registry_.move_entity(id, x, y);
}
void SpriteRenderer::set_entity_src_y(uint16_t id, int body_src_y, int head_src_y) {
    entity_registry_.set_entity_src_y(id, body_src_y, head_src_y);
}
void SpriteRenderer::advance_entity_src_x(uint16_t id, int step, int frame_count) {
    entity_registry_.advance_entity_src_x(id, step, frame_count);
}
void SpriteRenderer::mark_entity_moved(uint16_t id) { entity_registry_.mark_entity_moved(id); }
void SpriteRenderer::set_local_clan_name(const std::string& name) {
    entity_registry_.set_local_clan_name(name);
}
void SpriteRenderer::set_entity_clan_name(uint16_t id, const std::string& name) {
    entity_registry_.set_entity_clan_name(id, name);
}
void SpriteRenderer::set_entity_clan_by_username(const std::string& username,
                                                 const std::string& clan) {
    entity_registry_.set_entity_clan_by_username(username, clan);
}
void SpriteRenderer::set_entity_alpha(uint16_t id, uint8_t alpha) {
    entity_registry_.set_entity_alpha(id, alpha);
}
void SpriteRenderer::update_entity_equipment_overlay(uint16_t id, uint8_t slot,
                                                     const std::string& path, int offset_y,
                                                     bool static_frame) {
    entity_registry_.update_entity_equipment_overlay(id, slot, path, offset_y, static_frame);
}
void SpriteRenderer::clear_entity_equipment_overlay(uint16_t id, uint8_t slot) {
    entity_registry_.clear_entity_equipment_overlay(id, slot);
}
void SpriteRenderer::set_entity_body_sprite(uint16_t id, const std::string& path) {
    entity_registry_.set_entity_body_sprite(id, path);
}
void SpriteRenderer::reset_entity_body_sprite(uint16_t id) {
    entity_registry_.reset_entity_body_sprite(id);
}
bool SpriteRenderer::hit_test_entity(int world_x, int world_y, uint16_t& out_id) const {
    return entity_registry_.hit_test_entity(world_x, world_y, out_id);
}
bool SpriteRenderer::get_entity_world_position(uint16_t id, int& x, int& y) const {
    return entity_registry_.get_entity_world_position(id, x, y);
}
void SpriteRenderer::set_walk_anim_timeout(uint32_t ms) {
    entity_registry_.set_walk_anim_timeout(ms);
}
void SpriteRenderer::render_entity_names(const SDL2pp::Rect& cam) {
    entity_registry_.render_entity_names(cam);
}

// ── Effect delegation ─────────────────────────────────────────────────────────

void SpriteRenderer::load_damage_overlay(const DamageOverlayConfig& cfg) {
    effects_.load_damage_overlay(cfg);
}
void SpriteRenderer::trigger_damage_effect(int world_x, int world_y) {
    effects_.trigger_damage_effect(world_x, world_y);
}
void SpriteRenderer::load_spell_sheets(const std::vector<SpellSheetConfig>& sheets) {
    effects_.load_spell_sheets(sheets);
}
void SpriteRenderer::trigger_spell_effect(uint8_t effect_type, int world_x, int world_y) {
    effects_.trigger_spell_effect(effect_type, world_x, world_y);
}
void SpriteRenderer::advance_overlays() { effects_.advance(); }
void SpriteRenderer::render_overlays(const SDL2pp::Rect& cam) { effects_.render(cam); }

// ── Animations ────────────────────────────────────────────────────────────────

void SpriteRenderer::tick_animations(AnimationSystem& anim) {
    for (auto& sprite: sprites)
        anim.tick(sprite);
    entity_registry_.tick_animations(anim);
}

// ── Render ────────────────────────────────────────────────────────────────────

void SpriteRenderer::append_sprite_drawables(std::vector<SpriteRender>& src,
                                             const SDL2pp::Rect& cam, std::vector<Drawable>& out) {
    for (auto& sprite: src) {
        if (!sprite.visible || sprite.frames.empty() || !is_visible(sprite, cam))
            continue;
        SDL2pp::Rect dst(sprite.dst.GetX() - cam.GetX(), sprite.dst.GetY() - cam.GetY(),
                         sprite.dst.GetW(), sprite.dst.GetH());
        const bool use_src = sprite.use_src || sprite.src.GetW() > 0;
        out.push_back(Drawable{&sprite.frames[sprite.current_frame], sprite.src, dst, use_src,
                               sprite.dst.GetY() + sprite.dst.GetH(), sprite.alpha});
    }
}

void SpriteRenderer::append_equip_overlay_drawables(const SpriteRender& body,
                                                    EquipOverlay* overlays,
                                                    SDL2pp::Rect* static_cache,
                                                    const SDL2pp::Rect& cam,
                                                    std::vector<Drawable>& out) {
    const int body_foot_y = body.dst.GetY() + body.dst.GetH();
    const int src_y = body.src.GetY();
    const bool behind = (src_y == dir_src_y_up_ || src_y == dir_src_y_right_);
    for (uint8_t i = 0; i < EQUIP_SLOT_COUNT; ++i) {
        if (i == static_cast<uint8_t>(EquipSlot::ARMOR))
            continue;
        EquipOverlay& overlay = overlays[i];
        if (!overlay.active || overlay.frames.empty())
            continue;
        SDL2pp::Rect dst(body.dst.GetX() - cam.GetX(),
                         body.dst.GetY() - cam.GetY() + overlay.offset_y,
                         body.dst.GetW(), body.dst.GetH());
        const int overlay_foot_y = behind ? (body_foot_y - 1 - i) : (body_foot_y + 1 + i);
        if (overlay.static_frame) {
            static_cache[i] = body.src;
            static_cache[i].SetX(0);
            out.push_back(
                    Drawable{&overlay.frames[0], static_cache[i], dst, true, overlay_foot_y, body.alpha});
        } else {
            out.push_back(
                    Drawable{&overlay.frames[0], body.src, dst, true, overlay_foot_y, body.alpha});
        }
    }
}

void SpriteRenderer::render(const SDL2pp::Rect& cam) {
    std::vector<Drawable> drawables;

    append_sprite_drawables(sprites, cam, drawables);
    const SpriteRender* movable = find_movable_sprite();
    if (movable && movable->use_src && is_visible(*movable, cam))
        append_equip_overlay_drawables(*movable, equip_overlays_, static_cache_, cam, drawables);

    entity_registry_.collect_drawables(cam, drawables);

    sort_and_render_drawables(drawables);
}

void SpriteRenderer::sort_and_render_drawables(std::vector<Drawable>& drawables) {
    std::sort(drawables.begin(), drawables.end(),
              [](const Drawable& a, const Drawable& b) { return a.foot_y < b.foot_y; });
    for (auto& d: drawables) {
        SDL_Texture* tex = d.texture->Get();
        if (d.alpha < 255) {
            SDL_SetTextureAlphaMod(tex, d.alpha);
            SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        }
        if (d.use_src) {
            SDL_Rect sr = {d.src.GetX(), d.src.GetY(), d.src.GetW(), d.src.GetH()};
            SDL_Rect dr = {d.dst.GetX(), d.dst.GetY(), d.dst.GetW(), d.dst.GetH()};
            SDL_RenderCopy(renderer.Get(), tex, &sr, &dr);
        } else {
            renderer.Copy(*d.texture, SDL2pp::NullOpt, d.dst);
        }
        if (d.alpha < 255)
            SDL_SetTextureAlphaMod(tex, 255);
    }
}

// ── Private helpers ───────────────────────────────────────────────────────────

SpriteRender* SpriteRenderer::find_movable_sprite() {
    auto it = std::find_if(sprites.begin(), sprites.end(),
                           [](const SpriteRender& s) { return s.movable; });
    return it != sprites.end() ? &(*it) : nullptr;
}

const SpriteRender* SpriteRenderer::find_movable_sprite() const {
    auto it = std::find_if(sprites.begin(), sprites.end(),
                           [](const SpriteRender& s) { return s.movable; });
    return it != sprites.end() ? &(*it) : nullptr;
}
