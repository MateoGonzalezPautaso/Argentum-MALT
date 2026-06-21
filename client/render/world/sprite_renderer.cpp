#include "sprite_renderer.h"

#include <algorithm>
#include <cstdint>
#include <utility>

#include <SDL2/SDL.h>

namespace {
constexpr SDL_Color kNameColorDefault = {255, 255, 255, 255};
constexpr SDL_Color kNameColorEnemy   = {255,  80,  80, 255};
constexpr SDL_Color kNameColorAlly    = { 80, 255,  80, 255};
}

#include "../../../common/messages.h"
#include "../geometry.h"
#include "../texture_loader.h"

#include "animation_system.h"

SpriteRenderer::SpriteRenderer(SDL2pp::Renderer& renderer, TTF_Font* name_font, int window_w,
                               int window_h, bool has_tilemap, int map_px_w, int map_px_h):
        renderer(renderer),
        name_font(name_font),
        window_w(window_w),
        window_h(window_h),
        has_tilemap(has_tilemap),
        map_px_w(map_px_w),
        map_px_h(map_px_h) {}

void SpriteRenderer::set_map_bounds(bool has_tilemap, int map_px_w, int map_px_h) {
    this->has_tilemap = has_tilemap;
    this->map_px_w = map_px_w;
    this->map_px_h = map_px_h;
}

void SpriteRenderer::load_sprites(const std::vector<SpriteConfig>& sprites_config) {
    sprites.reserve(sprites_config.size());
    for (const auto& sprite_config: sprites_config) {
        SpriteRender sprite = build_sprite_render(sprite_config);
        if (sprite.frames.empty()) {
            continue;
        }
        entity_part_configs.push_back(sprite_config);
        sprites.push_back(std::move(sprite));
    }
}

SpriteRenderer::SpriteRender SpriteRenderer::build_sprite_render(
        const SpriteConfig& sprite_config) {
    SpriteRender sprite;
    sprite.frames.reserve(sprite_config.paths.size());
    for (const auto& path: sprite_config.paths) {
        SDL2pp::Surface surface = texture::load_surface(path);
        sprite.frames.emplace_back(renderer, surface);
    }

    if (sprite.frames.empty()) {
        return sprite;
    }

    sprite.dst = SDL2pp::Rect(sprite_config.x, sprite_config.y, sprite_config.width,
                              sprite_config.height);
    if (sprite_config.src_width > 0 && sprite_config.src_height > 0) {
        sprite.src = SDL2pp::Rect(sprite_config.src_x, sprite_config.src_y, sprite_config.src_width,
                                  sprite_config.src_height);
        sprite.use_src = true;
    }
    sprite.animated = sprite.frames.size() > 1;
    sprite.frame_ms = sprite_config.frame_ms;
    sprite.current_frame = 0;
    sprite.last_ticks = SDL_GetTicks();
    sprite.movable = sprite_config.movable;
    sprite.anchor_to_movable = sprite_config.anchor_to_movable;
    sprite.anchor_offset_x = sprite_config.anchor_offset_x;
    sprite.anchor_offset_y = sprite_config.anchor_offset_y;
    sprite.visible = sprite_config.visible;
    return sprite;
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

void SpriteRenderer::move_movable(int x, int y) {
    SpriteRender* sprite = find_movable_sprite();
    if (!sprite) {
        return;
    }
    sprite->dst.SetX(clamp_x(x, sprite->dst.GetW()));
    sprite->dst.SetY(clamp_y(y, sprite->dst.GetH()));
}

SpriteConfig SpriteRenderer::resolve_entity_skin(const SpriteConfig& config, Race race,
                                                 PlayerClass player_class,
                                                 uint16_t sprite_id) const {
    SpriteConfig selected = config;
    if (sprite_id > 0) {
        if (config.anchor_to_movable) {
            selected.paths.clear();
        } else {
            std::string path = skin_config.npc_path_for(sprite_id);
            int fw = skin_config.npc_frame_w(sprite_id);
            int fh = skin_config.npc_frame_h(sprite_id);
            if (!path.empty() && fw > 0 && fh > 0) {
                selected.paths = {std::move(path)};
                selected.x = 0;
                selected.y = 0;
                selected.width = fw;
                selected.height = fh;
                selected.src_x = skin_config.npc_src_x(sprite_id);
                selected.src_y = skin_config.npc_src_y(sprite_id);
                selected.src_width = fw;
                selected.src_height = fh;
                selected.visible = true;
            }
        }
    } else if (config.movable && !config.anchor_to_movable) {
        std::string path = skin_config.body_path_for(player_class);
        if (!path.empty()) {
            selected.paths = {std::move(path)};
        }
    } else if (config.anchor_to_movable) {
        std::string path = skin_config.head_path_for(race);
        if (!path.empty()) {
            selected.paths = {std::move(path)};
        }
    }
    return selected;
}

std::vector<SpriteRenderer::SpriteRender> SpriteRenderer::build_entity_parts(
        int x, int y, Race race, PlayerClass player_class, uint16_t sprite_id) {
    std::vector<SpriteRender> parts;
    parts.reserve(entity_part_configs.size());

    for (const auto& config: entity_part_configs) {
        SpriteConfig selected = resolve_entity_skin(config, race, player_class, sprite_id);
        SpriteRender part = build_sprite_render(selected);
        if (part.frames.empty()) {
            continue;
        }
        if (part.movable) {
            part.dst.SetX(x);
            part.dst.SetY(y);
        }
        parts.push_back(std::move(part));
    }
    return parts;
}

void SpriteRenderer::reposition_anchored_parts(std::vector<SpriteRender>& parts) {
    auto body_it = std::find_if(parts.begin(), parts.end(),
                                [](const SpriteRender& p) { return p.movable; });
    if (body_it == parts.end()) {
        return;
    }
    SpriteRender& body = *body_it;
    for (auto& part: parts) {
        if (!part.anchor_to_movable) {
            continue;
        }
        const int desired_x = body.dst.GetX() + part.anchor_offset_x;
        const int desired_y = body.dst.GetY() + part.anchor_offset_y;
        part.dst.SetX(clamp_x(desired_x, part.dst.GetW()));
        part.dst.SetY(clamp_y(desired_y, part.dst.GetH()));
    }
}

void SpriteRenderer::create_entity_name_label(uint16_t entity_id, const std::string& name) {
    if (name.empty() || !name_font) {
        return;
    }
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface* surface = TTF_RenderUTF8_Blended(name_font, name.c_str(), white);
    if (!surface) {
        return;
    }
    SDL2pp::Surface wrapped(surface);
    int text_w = 0, text_h = 0;
    TTF_SizeUTF8(name_font, name.c_str(), &text_w, &text_h);
    entities_[entity_id].name_label.emplace(
            EntityNameRender{SDL2pp::Texture(renderer, wrapped), text_w, text_h});
}

void SpriteRenderer::add_entity(uint16_t entity_id, int x, int y, const std::string& name,
                                Race race, PlayerClass player_class, uint16_t sprite_id) {
    if (entity_part_configs.empty()) {
        return;
    }

    RenderedEntity ent;
    if (sprite_id > 0) {
        ent.sprite_id = sprite_id;
        ent.frame_w = skin_config.npc_frame_w(sprite_id);
        ent.frame_h = skin_config.npc_frame_h(sprite_id);
        ent.base_src_x = skin_config.npc_src_x(sprite_id);
        ent.base_src_y = skin_config.npc_src_y(sprite_id);
        ent.speed = skin_config.speed(sprite_id);
    }
    auto parts = build_entity_parts(x, y, race, player_class, sprite_id);
    if (parts.empty()) {
        return;
    }
    reposition_anchored_parts(parts);
    ent.parts = std::move(parts);

    auto body_it = std::find_if(
            entity_part_configs.begin(), entity_part_configs.end(),
            [](const auto& config) { return config.movable && !config.anchor_to_movable; });
    if (body_it != entity_part_configs.end()) {
        SpriteConfig selected = resolve_entity_skin(*body_it, race, player_class, sprite_id);
        ent.equip.default_body_path = selected.paths.empty() ? "" : selected.paths[0];
    }

    ent.username = name;
    entities_[entity_id] = std::move(ent);
    create_entity_name_label(entity_id, name);
}

void SpriteRenderer::mark_entity_moved(uint16_t entity_id) {
    entities_[entity_id].last_move_tick = SDL_GetTicks();
}

void SpriteRenderer::set_entity_clan_name(uint16_t entity_id, const std::string& name) {
    if (!name.empty())
        entities_[entity_id].clan_name = name;
}

void SpriteRenderer::set_entity_clan_by_username(const std::string& username,
                                                 const std::string& clan) {
    for (auto& [eid, ent]: entities_) {
        if (ent.username == username) {
            ent.clan_name = clan;
            return;
        }
    }
}

void SpriteRenderer::remove_entity(uint16_t entity_id) {
    entities_.erase(entity_id);
}

void SpriteRenderer::clear_entities() {
    entities_.clear();
}

void SpriteRenderer::set_skin_config(const SkinConfig& cfg) { skin_config = cfg; }

void SpriteRenderer::update_equipment_overlay(uint8_t slot, const std::string& path, int offset_y,
                                              bool static_frame) {
    if (slot >= EQUIP_SLOT_COUNT)
        return;
    EquipOverlay& overlay = equip_overlays_[slot];
    try {
        SDL2pp::Surface surface = texture::load_surface(path);
        overlay.frames.clear();
        overlay.frames.emplace_back(renderer, surface);
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
    EquipOverlay& overlay = equip_overlays_[slot];
    overlay.frames.clear();
    overlay.active = false;
}

void SpriteRenderer::update_entity_equipment_overlay(uint16_t entity_id, uint8_t slot,
                                                     const std::string& path, int offset_y,
                                                     bool static_frame) {
    if (slot >= EQUIP_SLOT_COUNT)
        return;
    auto it = entities_.find(entity_id);
    if (it == entities_.end())
        return;
    EquipOverlay& overlay = it->second.equip.overlays[slot];
    try {
        SDL2pp::Surface surface = texture::load_surface(path);
        overlay.frames.clear();
        overlay.frames.emplace_back(renderer, surface);
        overlay.active = true;
        overlay.offset_y = offset_y;
        overlay.static_frame = static_frame;
    } catch (...) {
        overlay.frames.clear();
        overlay.active = false;
    }
}

void SpriteRenderer::clear_entity_equipment_overlay(uint16_t entity_id, uint8_t slot) {
    if (slot >= EQUIP_SLOT_COUNT)
        return;
    auto it = entities_.find(entity_id);
    if (it == entities_.end())
        return;
    EquipOverlay& overlay = it->second.equip.overlays[slot];
    overlay.frames.clear();
    overlay.active = false;
}

void SpriteRenderer::rebuild_local_player_sprites(Race race, PlayerClass player_class) {
    if (entity_part_configs.empty()) {
        return;
    }

    for (std::size_t i = 0; i < entity_part_configs.size() && i < sprites.size(); ++i) {
        const auto& config = entity_part_configs[i];
        SpriteConfig selected = resolve_entity_skin(config, race, player_class, 0);
        if (config.movable && !config.anchor_to_movable) {
            default_body_path_ = selected.paths.empty() ? "" : selected.paths[0];
        }
        sprites[i] = build_sprite_render(selected);
    }
}

void SpriteRenderer::set_body_sprite(const std::string& path) {
    SpriteRender* movable = find_movable_sprite();
    if (!movable || path.empty()) {
        return;
    }
    try {
        SDL2pp::Surface surface = texture::load_surface(path);
        movable->frames.clear();
        movable->frames.emplace_back(renderer, surface);
        movable->current_frame = 0;
    } catch (...) {
        reset_body_sprite();
    }
}

void SpriteRenderer::set_entity_body_sprite(uint16_t entity_id, const std::string& path) {
    SpriteRender* movable = find_entity_movable_sprite(entity_id);
    if (!movable || path.empty()) {
        return;
    }
    try {
        SDL2pp::Surface surface = texture::load_surface(path);
        movable->frames.clear();
        movable->frames.emplace_back(renderer, surface);
        movable->current_frame = 0;
    } catch (...) {
        reset_entity_body_sprite(entity_id);
    }
}

void SpriteRenderer::reset_body_sprite() {
    if (default_body_path_.empty()) {
        return;
    }
    set_body_sprite(default_body_path_);
}

void SpriteRenderer::reset_entity_body_sprite(uint16_t entity_id) {
    auto it = entities_.find(entity_id);
    if (it == entities_.end())
        return;
    if (it->second.equip.default_body_path.empty())
        return;
    set_entity_body_sprite(entity_id, it->second.equip.default_body_path);
}

void SpriteRenderer::set_direction_src_y(int down, int up, int left, int right) {
    dir_src_y_down_ = down;
    dir_src_y_up_ = up;
    dir_src_y_left_ = left;
    dir_src_y_right_ = right;
}

void SpriteRenderer::move_entity(uint16_t entity_id, int x, int y) {
    auto it = entities_.find(entity_id);
    if (it == entities_.end()) {
        return;
    }

    auto& parts = it->second.parts;
    auto body_it = std::find_if(parts.begin(), parts.end(),
                                [](const SpriteRender& p) { return p.movable; });
    SpriteRender* body = nullptr;
    if (body_it != parts.end())
        body = &(*body_it);
    if (!body) {
        return;
    }

    body->dst.SetX(clamp_x(x, body->dst.GetW()));
    body->dst.SetY(clamp_y(y, body->dst.GetH()));

    for (auto& part: parts) {
        if (!part.anchor_to_movable) {
            continue;
        }
        const int desired_x = body->dst.GetX() + part.anchor_offset_x;
        const int desired_y = body->dst.GetY() + part.anchor_offset_y;
        part.dst.SetX(clamp_x(desired_x, part.dst.GetW()));
        part.dst.SetY(clamp_y(desired_y, part.dst.GetH()));
    }
}

bool SpriteRenderer::get_movable_position(int& x, int& y) const {
    const SpriteRender* sprite = find_movable_sprite();
    if (!sprite) {
        return false;
    }
    x = sprite->dst.GetX();
    y = sprite->dst.GetY();
    return true;
}

int SpriteRenderer::movable_foot_y() const {
    const SpriteRender* sprite = find_movable_sprite();
    if (!sprite) {
        return 0;
    }
    return sprite->dst.GetY() + sprite->dst.GetH();
}

void SpriteRenderer::set_movable_src_y(int y) {
    SpriteRender* sprite = find_movable_sprite();
    if (!sprite || !sprite->use_src) {
        return;
    }
    sprite->src.SetY(y);
}

void SpriteRenderer::advance_src_x(SpriteRender& sprite, int step, int frame_count) {
    if (!sprite.use_src || frame_count <= 0 || step <= 0) {
        return;
    }
    const int current_index = sprite.src.GetX() / step;
    const int next_index = (current_index + 1) % frame_count;
    sprite.src.SetX(next_index * step);
}

void SpriteRenderer::advance_movable_src_x(int step, int frame_count) {
    SpriteRender* sprite = find_movable_sprite();
    if (!sprite) {
        return;
    }
    advance_src_x(*sprite, step, frame_count);
}

void SpriteRenderer::set_anchor_src_y(int y) {
    for (auto& sprite: sprites) {
        if (!sprite.anchor_to_movable || !sprite.use_src) {
            continue;
        }
        sprite.src.SetY(y);
    }
}

void SpriteRenderer::set_entity_src_y(uint16_t entity_id, int body_src_y, int head_src_y) {
    auto it = entities_.find(entity_id);
    if (it == entities_.end()) {
        return;
    }
    RenderedEntity& e = it->second;

    int dir_index = body_src_y / 48;  // 0=down, 1=up, 2=left, 3=right
    if (e.sprite_id > 0 && skin_config.npc_swap_lr(e.sprite_id)) {
        if (dir_index == 2)
            dir_index = 3;
        else if (dir_index == 3)
            dir_index = 2;
    }
    bool is_walking = false;
    uint32_t elapsed = SDL_GetTicks() - e.last_move_tick;
    if (elapsed < walk_anim_timeout_ms_)  // walking animation visible after last move
        is_walking = true;

    for (auto& sprite: e.parts) {
        if (!sprite.use_src) {
            continue;
        }
        if (sprite.movable) {
            if (e.frame_h > 0) {
                int offset = is_walking ? skin_config.npc_walk_row_offset(e.sprite_id) : 0;
                int row = dir_index + offset;
                const auto& rp = skin_config.npc_row_positions(e.sprite_id);
                if (!rp.empty()) {
                    int idx = std::min(row, static_cast<int>(rp.size()) - 1);
                    idx = std::max(idx, 0);
                    sprite.src.SetY(rp[idx]);
                } else {
                    sprite.src.SetY(e.base_src_y + row * e.frame_h);
                }
            } else {
                sprite.src.SetY(body_src_y);
            }
        } else if (sprite.anchor_to_movable) {
            sprite.src.SetY(head_src_y);
        }
    }
}

void SpriteRenderer::advance_entity_src_x(uint16_t entity_id, int step, int frame_count) {
    auto it = entities_.find(entity_id);
    if (it == entities_.end()) {
        return;
    }
    RenderedEntity& e = it->second;

    e.move_counter++;

    if (e.speed == 1 && e.move_counter % 2 != 0)
        return;

    for (auto& sprite: e.parts) {
        if (!sprite.movable) {
            continue;
        }
        if (e.frame_w > 0) {
            const auto& fp = skin_config.npc_frame_positions(e.sprite_id);
            if (!fp.empty()) {
                int current_x = sprite.src.GetX();
                int current_idx = -1;
                for (size_t i = 0; i < fp.size(); ++i) {
                    if (fp[i] == current_x) {
                        current_idx = static_cast<int>(i);
                        break;
                    }
                }
                if (current_idx < 0)
                    current_idx = 0;
                int next_idx = (current_idx + 1) % static_cast<int>(fp.size());
                sprite.src.SetX(fp[next_idx]);
            } else {
                int npc_step = e.frame_w;
                int npc_frames = skin_config.npc_frames_per_dir(e.sprite_id);
                if (npc_frames <= 0)
                    npc_frames = frame_count;
                if (npc_frames <= 1)
                    continue;
                int current = (sprite.src.GetX() - e.base_src_x) / npc_step;
                if (current < 0)
                    current = 0;
                int next = (current + 1) % npc_frames;
                sprite.src.SetX(e.base_src_x + next * npc_step);
            }
        } else {
            advance_src_x(sprite, step, frame_count);
        }
    }
}

void SpriteRenderer::set_entity_alpha(uint16_t entity_id, uint8_t alpha) {
    auto it = entities_.find(entity_id);
    if (it == entities_.end()) {
        return;
    }
    for (auto& sprite: it->second.parts) {
        sprite.alpha = alpha;
    }
}

void SpriteRenderer::set_movable_alpha(uint8_t alpha) {
    for (auto& sprite: sprites) {
        if (sprite.movable || sprite.anchor_to_movable) {
            sprite.alpha = alpha;
        }
    }
}

void SpriteRenderer::reposition_anchored_sprites() {
    SpriteRender* movable = find_movable_sprite();
    if (!movable) {
        return;
    }

    const int base_x = clamp_x(movable->dst.GetX(), movable->dst.GetW());
    const int base_y = clamp_y(movable->dst.GetY(), movable->dst.GetH());

    for (auto& sprite: sprites) {
        if (!sprite.anchor_to_movable) {
            continue;
        }
        const int desired_x = base_x + sprite.anchor_offset_x;
        const int desired_y = base_y + sprite.anchor_offset_y;
        sprite.dst.SetX(clamp_x(desired_x, sprite.dst.GetW()));
        sprite.dst.SetY(clamp_y(desired_y, sprite.dst.GetH()));
    }
}

bool SpriteRenderer::is_visible(const SpriteRender& s, const SDL2pp::Rect& cam) {
    return s.dst.GetX() + s.dst.GetW() > cam.GetX() && s.dst.GetX() < cam.GetX() + cam.GetW() &&
           s.dst.GetY() + s.dst.GetH() > cam.GetY() && s.dst.GetY() < cam.GetY() + cam.GetH();
}

void SpriteRenderer::append_equip_overlay_drawables(const SpriteRender& body,
                                                    EquipOverlay* overlays,
                                                    SDL2pp::Rect* static_cache,
                                                    const SDL2pp::Rect& cam,
                                                    std::vector<Drawable>& out) {
    int body_foot_y = body.dst.GetY() + body.dst.GetH();
    int src_y = body.src.GetY();
    bool behind = (src_y == dir_src_y_up_ || src_y == dir_src_y_right_);

    for (uint8_t i = 0; i < EQUIP_SLOT_COUNT; ++i) {
        if (i == static_cast<uint8_t>(EquipSlot::ARMOR))
            continue;
        EquipOverlay& overlay = overlays[i];
        if (!overlay.active || overlay.frames.empty())
            continue;

        SDL2pp::Rect dst(body.dst.GetX() - cam.GetX(),
                         body.dst.GetY() - cam.GetY() + overlay.offset_y,
                         body.dst.GetW(), body.dst.GetH());
        int overlay_foot_y = behind ? (body_foot_y - 1 - i) : (body_foot_y + 1 + i);
        if (overlay.static_frame) {
            static_cache[i] = body.src;
            static_cache[i].SetX(0);
            out.push_back(Drawable{&overlay.frames[0], static_cache[i], dst, true,
                                   overlay_foot_y, body.alpha});
        } else {
            out.push_back(Drawable{&overlay.frames[0], body.src, dst, true,
                                   overlay_foot_y, body.alpha});
        }
    }
}

void SpriteRenderer::render(const SDL2pp::Rect& cam) {
    std::vector<Drawable> drawables;

    append_sprite_drawables(sprites, cam, drawables);

    SpriteRender* movable = find_movable_sprite();
    if (movable && movable->use_src && is_visible(*movable, cam)) {
        append_equip_overlay_drawables(*movable, equip_overlays_, static_cache_, cam, drawables);
    }

    for (auto& [entity_id, e]: entities_) {
        append_sprite_drawables(e.parts, cam, drawables);

        bool is_npc = e.sprite_id > 0;

        auto body_it = std::find_if(e.parts.begin(), e.parts.end(),
                                    [](const SpriteRender& p) { return p.movable; });
        if (body_it == e.parts.end() || (!body_it->use_src && !is_npc))
            continue;
        SpriteRender& movable_entity = *body_it;
        if (!is_visible(movable_entity, cam))
            continue;

        append_equip_overlay_drawables(movable_entity, e.equip.overlays, e.equip.static_cache,
                                       cam, drawables);
    }

    sort_and_render_drawables(drawables);
}

void SpriteRenderer::append_sprite_drawables(std::vector<SpriteRender>& src,
                                             const SDL2pp::Rect& cam, std::vector<Drawable>& out) {
    for (auto& sprite: src) {
        if (!sprite.visible || sprite.frames.empty() || !is_visible(sprite, cam)) {
            continue;
        }
        SDL2pp::Rect dst(sprite.dst.GetX() - cam.GetX(), sprite.dst.GetY() - cam.GetY(),
                         sprite.dst.GetW(), sprite.dst.GetH());
        bool use_src = sprite.use_src || sprite.src.GetW() > 0;
        out.push_back(Drawable{&sprite.frames[sprite.current_frame], sprite.src, dst, use_src,
                               sprite.dst.GetY() + sprite.dst.GetH(), sprite.alpha});
    }
}

void SpriteRenderer::render_entity_names(const SDL2pp::Rect& cam) {
    for (auto& [eid, e]: entities_) {
        if (!e.name_label.has_value()) {
            continue;
        }
        int body_foot_y = 0;
        auto sit = std::find_if(e.parts.begin(), e.parts.end(),
                                [](const auto& s) { return s.movable; });
        if (sit != e.parts.end())
            body_foot_y = sit->dst.GetY() + sit->dst.GetH();
        if (body_foot_y <= 0) {
            continue;
        }
        auto& body = e.parts[0];
        const int name_x =
                body.dst.GetX() + (body.dst.GetW() - e.name_label->w) / 2 - cam.GetX();
        const int name_y = body.dst.GetY() - e.name_label->h - 24 - cam.GetY();

        SDL_Color color = kNameColorDefault;
        if (e.sprite_id > 0) {
            color = kNameColorEnemy;
        } else if (!local_clan_name.empty() && !e.clan_name.empty()
                   && e.clan_name == local_clan_name) {
            color = kNameColorAlly;
        }

        SDL_Texture* tex = e.name_label->texture.Get();
        SDL_SetTextureColorMod(tex, color.r, color.g, color.b);
        renderer.Copy(e.name_label->texture, SDL2pp::NullOpt,
                      SDL2pp::Rect(name_x, name_y, e.name_label->w, e.name_label->h));
        SDL_SetTextureColorMod(tex, 255, 255, 255);
    }
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
            SDL_RenderCopy(renderer.Get(), d.texture->Get(), &sr, &dr);
        } else {
            renderer.Copy(*d.texture, SDL2pp::NullOpt, d.dst);
        }
        if (d.alpha < 255) {
            SDL_SetTextureAlphaMod(tex, 255);
        }
    }
}

void SpriteRenderer::load_damage_overlay(const DamageOverlayConfig& cfg) {
    overlays.resize(3);
    for (auto& ov: overlays) {
        ov.frames.reserve(cfg.last_graphic - cfg.first_graphic + 1);
        for (int i = cfg.first_graphic; i <= cfg.last_graphic; ++i) {
            std::string path = "assets/Graficos/" + std::to_string(i) + ".png";
            SDL2pp::Surface surface = texture::load_surface(path);
            ov.frames.emplace_back(renderer, surface);
        }
        ov.frame_ms = cfg.frame_ms;
        ov.dst.SetW(cfg.display_w);
        ov.dst.SetH(cfg.display_h);
    }
}

void SpriteRenderer::load_spell_sheets(const std::vector<SpellSheetConfig>& sheets) {
    for (uint8_t idx = 0; idx < static_cast<uint8_t>(sheets.size()); ++idx) {
        const SpellSheetConfig& s = sheets[idx];
        spell_offsets_[idx] = {s.offset_x, s.offset_y};
        SDL2pp::Surface sheet = texture::load_surface(s.path);
        std::vector<OverlayEffect>& pool = spell_overlay_pools[idx];
        pool.resize(3);
        int total = s.cols * s.rows;
        for (auto& ov: pool) {
            ov.frames.reserve(total);
            for (int r = 0; r < s.rows; ++r) {
                for (int c = 0; c < s.cols; ++c) {
                    SDL_Surface* sub = SDL_CreateRGBSurfaceWithFormat(
                            0, s.frame_w, s.frame_h, 32, SDL_PIXELFORMAT_ABGR8888);
                    SDL_Rect src{c * s.frame_w, r * s.frame_h, s.frame_w, s.frame_h};
                    SDL_SetSurfaceBlendMode(sheet.Get(), SDL_BLENDMODE_NONE);
                    SDL_BlitSurface(sheet.Get(), &src, sub, nullptr);
                    auto& tex = ov.frames.emplace_back(renderer, SDL2pp::Surface(sub));
                    tex.SetBlendMode(SDL_BLENDMODE_BLEND);
                }
            }
            ov.frame_ms = s.frame_ms;
            ov.dst.SetW(s.display_w);
            ov.dst.SetH(s.display_h);
        }
    }
}

void SpriteRenderer::trigger_damage_effect(int world_x, int world_y) {
    for (auto& ov: overlays) {
        if (ov.active)
            continue;
        ov.dst.SetX(world_x - ov.dst.GetW() / 2);
        ov.dst.SetY(world_y - ov.dst.GetH() / 2);
        ov.current_frame = 0;
        ov.last_ticks = SDL_GetTicks();
        ov.active = true;
        return;
    }
}

void SpriteRenderer::trigger_spell_effect(uint8_t effect_type, int world_x, int world_y) {
    auto it = spell_overlay_pools.find(effect_type);
    if (it == spell_overlay_pools.end())
        return;
    for (auto& ov: it->second) {
        if (ov.active)
            continue;
        int ox = 0, oy = 0;
        auto off_it = spell_offsets_.find(effect_type);
        if (off_it != spell_offsets_.end()) {
            ox = off_it->second.first;
            oy = off_it->second.second;
        }
        ov.dst.SetX(world_x - ov.dst.GetW() / 2 + ox);
        ov.dst.SetY(world_y - ov.dst.GetH() / 2 + oy);
        ov.current_frame = 0;
        ov.last_ticks = SDL_GetTicks();
        ov.active = true;
        return;
    }
}

bool SpriteRenderer::get_entity_world_position(uint16_t entity_id, int& x, int& y) const {
    auto it = entities_.find(entity_id);
    if (it == entities_.end())
        return false;
    auto pit = std::find_if(it->second.parts.begin(), it->second.parts.end(),
                            [](const auto& p) { return p.movable; });
    if (pit == it->second.parts.end())
        return false;
    x = pit->dst.GetX() + pit->dst.GetW() / 2;
    y = pit->dst.GetY() + pit->dst.GetH() / 2;
    return true;
}

void SpriteRenderer::advance_overlays() {
    uint32_t now = SDL_GetTicks();
    auto tick_pool = [&](std::vector<OverlayEffect>& pool) {
        for (auto& ov: pool) {
            if (!ov.active)
                continue;
            if (now - ov.last_ticks < ov.frame_ms)
                continue;
            ov.last_ticks = now;
            ov.current_frame++;
            if (ov.current_frame >= ov.frames.size()) {
                ov.active = false;
            }
        }
    };
    tick_pool(overlays);
    for (auto& [key, pool]: spell_overlay_pools) tick_pool(pool);
}

void SpriteRenderer::render_overlays(const SDL2pp::Rect& cam) {
    auto render_pool = [&](std::vector<OverlayEffect>& pool) {
        for (auto& ov: pool) {
            if (!ov.active)
                continue;
            SDL2pp::Rect dst(ov.dst.GetX() - cam.GetX(), ov.dst.GetY() - cam.GetY(), ov.dst.GetW(),
                             ov.dst.GetH());
            renderer.Copy(ov.frames[ov.current_frame], SDL2pp::NullOpt, dst);
        }
    };
    render_pool(overlays);
    for (auto& [key, pool]: spell_overlay_pools) render_pool(pool);
}

void SpriteRenderer::tick_animations(AnimationSystem& anim) {
    for (auto& sprite: sprites) {
        anim.tick(sprite);
    }
    for (auto& [id, e]: entities_) {
        for (auto& sprite: e.parts) {
            anim.tick(sprite);
        }
    }
}

bool SpriteRenderer::hit_test_entity(int world_x, int world_y, uint16_t& out_entity_id) const {
    auto hits_point = [&](const SpriteRender& part) {
        return part.movable && point_in_rect(world_x, world_y, part.dst);
    };
    auto it = std::find_if(entities_.begin(), entities_.end(), [&](const auto& pair) {
        return std::any_of(pair.second.parts.begin(), pair.second.parts.end(), hits_point);
    });
    if (it != entities_.end()) {
        out_entity_id = it->first;
        return true;
    }
    return false;
}

SpriteRenderer::SpriteRender* SpriteRenderer::find_movable_sprite() {
    auto it = std::find_if(sprites.begin(), sprites.end(),
                           [](const SpriteRender& s) { return s.movable; });
    return it != sprites.end() ? &(*it) : nullptr;
}

SpriteRenderer::SpriteRender* SpriteRenderer::find_entity_movable_sprite(uint16_t entity_id) {
    auto it = entities_.find(entity_id);
    if (it == entities_.end())
        return nullptr;
    auto pit = std::find_if(it->second.parts.begin(), it->second.parts.end(),
                            [](const SpriteRender& s) { return s.movable; });
    return pit != it->second.parts.end() ? &(*pit) : nullptr;
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

const SpriteRenderer::SpriteRender* SpriteRenderer::find_movable_sprite() const {
    auto it = std::find_if(sprites.begin(), sprites.end(),
                           [](const SpriteRender& s) { return s.movable; });
    return it != sprites.end() ? &(*it) : nullptr;
}

const SpriteRenderer::SpriteRender* SpriteRenderer::find_entity_movable_sprite(
        uint16_t entity_id) const {
    auto it = entities_.find(entity_id);
    if (it == entities_.end())
        return nullptr;
    auto pit = std::find_if(it->second.parts.begin(), it->second.parts.end(),
                            [](const SpriteRender& s) { return s.movable; });
    return pit != it->second.parts.end() ? &(*pit) : nullptr;
}
