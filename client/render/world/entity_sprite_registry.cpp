#include "entity_sprite_registry.h"

#include <algorithm>

#include <SDL2/SDL.h>

#include "../gfx/geometry.h"
#include "../gfx/texture_loader.h"
#include "animation_system.h"

namespace {
constexpr SDL_Color kNameColorDefault = {255, 255, 255, 255};
constexpr SDL_Color kNameColorEnemy = {255, 80, 80, 255};
constexpr SDL_Color kNameColorAlly = {80, 255, 80, 255};
}  // namespace


EntitySpriteRegistry::EntitySpriteRegistry(SDL2pp::Renderer& renderer, TTF_Font* name_font,
                                           int window_w, int window_h, bool has_tilemap,
                                           int map_px_w, int map_px_h):
        renderer(renderer),
        name_font(name_font),
        window_w(window_w),
        window_h(window_h),
        has_tilemap(has_tilemap),
        map_px_w(map_px_w),
        map_px_h(map_px_h) {}

void EntitySpriteRegistry::set_part_templates(const std::vector<SpriteConfig>& configs,
                                              const SkinConfig& skin) {
    entity_part_configs = configs;
    skin_config = skin;
}

void EntitySpriteRegistry::set_skin_config(const SkinConfig& cfg) { skin_config = cfg; }

void EntitySpriteRegistry::set_map_bounds(bool has_tm, int mpw, int mph) {
    has_tilemap = has_tm;
    map_px_w = mpw;
    map_px_h = mph;
}

void EntitySpriteRegistry::set_name_font(TTF_Font* font) { name_font = font; }

void EntitySpriteRegistry::set_direction_src_y(int down, int up, int left, int right) {
    dir_src_y_down = down;
    dir_src_y_up = up;
    dir_src_y_left = left;
    dir_src_y_right = right;
}

void EntitySpriteRegistry::set_walk_anim_timeout(uint32_t ms) { walk_anim_timeout_ms = ms; }
void EntitySpriteRegistry::set_local_clan_name(const std::string& name) { local_clan_name = name; }


SpriteRender EntitySpriteRegistry::build_sprite_render(SDL2pp::Renderer& renderer,
                                                       const SpriteConfig& cfg) {
    SpriteRender sprite;
    sprite.frames.reserve(cfg.paths.size());
    for (const auto& path: cfg.paths)
        sprite.frames.emplace_back(renderer, texture::load_surface(path));

    if (sprite.frames.empty())
        return sprite;

    sprite.dst = SDL2pp::Rect(cfg.x, cfg.y, cfg.width, cfg.height);
    if (cfg.src_width > 0 && cfg.src_height > 0) {
        sprite.src = SDL2pp::Rect(cfg.src_x, cfg.src_y, cfg.src_width, cfg.src_height);
        sprite.use_src = true;
    }
    sprite.animated = sprite.frames.size() > 1;
    sprite.frame_ms = cfg.frame_ms;
    sprite.current_frame = 0;
    sprite.last_ticks = SDL_GetTicks();
    sprite.movable = cfg.movable;
    sprite.anchor_to_movable = cfg.anchor_to_movable;
    sprite.anchor_offset_x = cfg.anchor_offset_x;
    sprite.anchor_offset_y = cfg.anchor_offset_y;
    sprite.visible = cfg.visible;
    return sprite;
}

SpriteConfig EntitySpriteRegistry::resolve_entity_skin(const SkinConfig& skin,
                                                       const SpriteConfig& cfg, Race race,
                                                       PlayerClass pc, uint16_t sprite_id) {
    SpriteConfig selected = cfg;
    if (sprite_id > 0) {
        if (cfg.anchor_to_movable) {
            selected.paths.clear();
        } else {
            const auto& def = skin.npc_def(sprite_id);
            if (!def.path.empty() && def.frame_w > 0 && def.frame_h > 0) {
                selected.paths = {def.path};
                selected.x = 0;
                selected.y = 0;
                selected.width = def.frame_w;
                selected.height = def.frame_h;
                selected.src_y = def.src_y;
                selected.src_width = def.frame_w;
                selected.src_height = def.frame_h;
                selected.visible = true;
                selected.src_x = def.frame_positions.empty() ? def.src_x : def.frame_positions[0];
            }
        }
    } else if (cfg.movable && !cfg.anchor_to_movable) {
        std::string path = skin.body_path_for(pc);
        if (!path.empty())
            selected.paths = {std::move(path)};
    } else if (cfg.anchor_to_movable) {
        std::string path = skin.head_path_for(race);
        if (!path.empty())
            selected.paths = {std::move(path)};
    }
    return selected;
}


int EntitySpriteRegistry::clamp_x(int value, int sprite_w) const {
    const int max_x =
            has_tilemap ? std::max(0, map_px_w - sprite_w) : std::max(0, window_w - sprite_w);
    return std::clamp(value, 0, max_x);
}

int EntitySpriteRegistry::clamp_y(int value, int sprite_h) const {
    const int max_y =
            has_tilemap ? std::max(0, map_px_h - sprite_h) : std::max(0, window_h - sprite_h);
    return std::clamp(value, 0, max_y);
}

bool EntitySpriteRegistry::is_visible(const SpriteRender& s, const SDL2pp::Rect& cam) {
    return s.dst.GetX() + s.dst.GetW() > cam.GetX() &&
           s.dst.GetX() < cam.GetX() + cam.GetW() &&
           s.dst.GetY() + s.dst.GetH() > cam.GetY() &&
           s.dst.GetY() < cam.GetY() + cam.GetH();
}

void EntitySpriteRegistry::advance_src_x(SpriteRender& sprite, int step, int frame_count) {
    if (!sprite.use_src || frame_count <= 0 || step <= 0)
        return;
    const int next = (sprite.src.GetX() / step + 1) % frame_count;
    sprite.src.SetX(next * step);
}

void EntitySpriteRegistry::append_sprite_drawables(std::vector<SpriteRender>& src,
                                                   const SDL2pp::Rect& cam,
                                                   std::vector<Drawable>& out) {
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

void EntitySpriteRegistry::append_equip_overlay_drawables(const SpriteRender& body,
                                                          EquipOverlay* overlays,
                                                          SDL2pp::Rect* cache,
                                                          const SDL2pp::Rect& cam,
                                                          std::vector<Drawable>& out) {
    const int body_foot_y = body.dst.GetY() + body.dst.GetH();
    const int src_y = body.src.GetY();
    const bool behind = (src_y == dir_src_y_up || src_y == dir_src_y_right);

    for (uint8_t i = 0; i < EQUIP_SLOT_COUNT; ++i) {
        if (i == static_cast<uint8_t>(EquipSlot::ARMOR))
            continue;
        EquipOverlay& ov = overlays[i];
        if (!ov.active || ov.frames.empty())
            continue;
        SDL2pp::Rect dst(body.dst.GetX() - cam.GetX(),
                         body.dst.GetY() - cam.GetY() + ov.offset_y,
                         body.dst.GetW(), body.dst.GetH());
        const int ov_foot_y = behind ? (body_foot_y - 1 - i) : (body_foot_y + 1 + i);
        if (ov.static_frame) {
            cache[i] = body.src;
            cache[i].SetX(0);
            out.push_back(Drawable{&ov.frames[0], cache[i], dst, true, ov_foot_y, body.alpha});
        } else {
            out.push_back(Drawable{&ov.frames[0], body.src, dst, true, ov_foot_y, body.alpha});
        }
    }
}

std::vector<SpriteRender> EntitySpriteRegistry::build_entity_parts(int x, int y, Race race,
                                                                    PlayerClass pc,
                                                                    uint16_t sprite_id) {
    std::vector<SpriteRender> parts;
    parts.reserve(entity_part_configs.size());
    for (const auto& cfg: entity_part_configs) {
        SpriteConfig selected = resolve_entity_skin(skin_config, cfg, race, pc, sprite_id);
        SpriteRender part = build_sprite_render(renderer, selected);
        if (part.frames.empty())
            continue;
        if (part.movable) {
            part.dst.SetX(x);
            part.dst.SetY(y);
        }
        parts.push_back(std::move(part));
    }
    return parts;
}

void EntitySpriteRegistry::reposition_anchored_parts(std::vector<SpriteRender>& parts) {
    auto it = std::find_if(parts.begin(), parts.end(), [](const SpriteRender& p) { return p.movable; });
    if (it == parts.end())
        return;
    SpriteRender& body = *it;
    for (auto& part: parts) {
        if (!part.anchor_to_movable)
            continue;
        part.dst.SetX(clamp_x(body.dst.GetX() + part.anchor_offset_x, part.dst.GetW()));
        part.dst.SetY(clamp_y(body.dst.GetY() + part.anchor_offset_y, part.dst.GetH()));
    }
}

void EntitySpriteRegistry::create_entity_name_label(uint16_t id, const std::string& name) {
    if (name.empty() || !name_font)
        return;
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface* surface = TTF_RenderUTF8_Blended(name_font, name.c_str(), white);
    if (!surface)
        return;
    SDL2pp::Surface wrapped(surface);
    int text_w = 0, text_h = 0;
    TTF_SizeUTF8(name_font, name.c_str(), &text_w, &text_h);
    entities[id].name_label.emplace(
            EntityNameRender{SDL2pp::Texture(renderer, wrapped), text_w, text_h});
}

SpriteRender* EntitySpriteRegistry::find_movable(uint16_t id) {
    auto it = entities.find(id);
    if (it == entities.end())
        return nullptr;
    auto pit = std::find_if(it->second.parts.begin(), it->second.parts.end(),
                            [](const SpriteRender& s) { return s.movable; });
    return pit != it->second.parts.end() ? &(*pit) : nullptr;
}

const SpriteRender* EntitySpriteRegistry::find_movable(uint16_t id) const {
    auto it = entities.find(id);
    if (it == entities.end())
        return nullptr;
    auto pit = std::find_if(it->second.parts.begin(), it->second.parts.end(),
                            [](const SpriteRender& s) { return s.movable; });
    return pit != it->second.parts.end() ? &(*pit) : nullptr;
}

// ── Entity lifecycle ──────────────────────────────────────────────────────────

void EntitySpriteRegistry::add_entity(uint16_t id, int x, int y, const std::string& name,
                                      Race race, PlayerClass pc, uint16_t sprite_id) {
    if (entity_part_configs.empty())
        return;

    RenderedEntity ent;
    if (sprite_id > 0) {
        ent.sprite_id = sprite_id;
        const auto& def = skin_config.npc_def(sprite_id);
        ent.frame_w = def.frame_w;
        ent.frame_h = def.frame_h;
        ent.base_src_y = def.src_y;
        ent.speed = def.speed;
        ent.base_src_x = def.frame_positions.empty() ? def.src_x : def.frame_positions[0];
    }
    auto parts = build_entity_parts(x, y, race, pc, sprite_id);
    if (parts.empty())
        return;
    reposition_anchored_parts(parts);
    ent.parts = std::move(parts);

    auto body_it = std::find_if(entity_part_configs.begin(), entity_part_configs.end(),
                                [](const auto& c) { return c.movable && !c.anchor_to_movable; });
    if (body_it != entity_part_configs.end()) {
        SpriteConfig sel = resolve_entity_skin(skin_config, *body_it, race, pc, sprite_id);
        ent.equip.default_body_path = sel.paths.empty() ? "" : sel.paths[0];
    }

    ent.username = name;
    entities[id] = std::move(ent);
    create_entity_name_label(id, name);
}

void EntitySpriteRegistry::remove_entity(uint16_t id) { entities.erase(id); }
void EntitySpriteRegistry::clear_entities() { entities.clear(); }
void EntitySpriteRegistry::mark_entity_moved(uint16_t id) {
    entities[id].last_move_tick = SDL_GetTicks();
}

void EntitySpriteRegistry::move_entity(uint16_t id, int x, int y) {
    auto it = entities.find(id);
    if (it == entities.end())
        return;
    auto& parts = it->second.parts;
    auto body_it = std::find_if(parts.begin(), parts.end(),
                                [](const SpriteRender& p) { return p.movable; });
    if (body_it == parts.end())
        return;
    SpriteRender& body = *body_it;
    body.dst.SetX(clamp_x(x, body.dst.GetW()));
    body.dst.SetY(clamp_y(y, body.dst.GetH()));
    for (auto& part: parts) {
        if (!part.anchor_to_movable)
            continue;
        part.dst.SetX(clamp_x(body.dst.GetX() + part.anchor_offset_x, part.dst.GetW()));
        part.dst.SetY(clamp_y(body.dst.GetY() + part.anchor_offset_y, part.dst.GetH()));
    }
}

// ── Entity animation ──────────────────────────────────────────────────────────

void EntitySpriteRegistry::set_entity_src_y(uint16_t id, int body_src_y, int head_src_y) {
    auto it = entities.find(id);
    if (it == entities.end())
        return;
    RenderedEntity& e = it->second;

    const auto& def = skin_config.npc_def(e.sprite_id);
    int dir_index = body_src_y / 48;
    if (e.sprite_id > 0 && def.swap_lr) {
        if (dir_index == 2) dir_index = 3;
        else if (dir_index == 3) dir_index = 2;
    }
    const bool is_walking = (SDL_GetTicks() - e.last_move_tick) < walk_anim_timeout_ms;

    for (auto& sprite: e.parts) {
        if (!sprite.use_src)
            continue;
        if (sprite.movable) {
            if (e.frame_h > 0) {
                const int offset = is_walking ? def.walk_row_offset : 0;
                const int row = dir_index + offset;
                const auto& rp = def.row_positions;
                if (!rp.empty()) {
                    sprite.src.SetY(rp[std::clamp(row, 0, static_cast<int>(rp.size()) - 1)]);
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

void EntitySpriteRegistry::advance_entity_src_x(uint16_t id, int step, int frame_count) {
    auto it = entities.find(id);
    if (it == entities.end())
        return;
    RenderedEntity& e = it->second;
    e.move_counter++;
    if (e.speed == 1 && e.move_counter % 2 != 0)
        return;

    for (auto& sprite: e.parts) {
        if (!sprite.movable)
            continue;
        if (e.frame_w > 0) {
            const auto& fp = skin_config.npc_def(e.sprite_id).frame_positions;
            if (!fp.empty()) {
                int current_x = sprite.src.GetX();
                int current_idx = 0;
                for (size_t i = 0; i < fp.size(); ++i)
                    if (fp[i] == current_x) { current_idx = static_cast<int>(i); break; }
                sprite.src.SetX(fp[(current_idx + 1) % static_cast<int>(fp.size())]);
            } else {
                int npc_frames = skin_config.npc_def(e.sprite_id).frames_per_dir;
                if (npc_frames <= 0) npc_frames = frame_count;
                if (npc_frames <= 1) continue;
                const int current = std::max(0, (sprite.src.GetX() - e.base_src_x) / e.frame_w);
                sprite.src.SetX(e.base_src_x + (current + 1) % npc_frames * e.frame_w);
            }
        } else {
            advance_src_x(sprite, step, frame_count);
        }
    }
}

void EntitySpriteRegistry::tick_animations(AnimationSystem& anim) {
    for (auto& [id, e]: entities)
        for (auto& sprite: e.parts)
            anim.tick(sprite);
}

// ── Entity state ──────────────────────────────────────────────────────────────

void EntitySpriteRegistry::set_entity_clan_name(uint16_t id, const std::string& name) {
    if (!name.empty())
        entities[id].clan_name = name;
}

void EntitySpriteRegistry::set_entity_clan_by_username(const std::string& username,
                                                       const std::string& clan) {
    for (auto& [eid, ent]: entities)
        if (ent.username == username) { ent.clan_name = clan; return; }
}

void EntitySpriteRegistry::set_entity_alpha(uint16_t id, uint8_t alpha) {
    auto it = entities.find(id);
    if (it == entities.end()) return;
    for (auto& sprite: it->second.parts)
        sprite.alpha = alpha;
}

// ── Entity equipment ──────────────────────────────────────────────────────────

void EntitySpriteRegistry::update_entity_equipment_overlay(uint16_t id, uint8_t slot,
                                                           const std::string& path, int offset_y,
                                                           bool static_frame) {
    if (slot >= EQUIP_SLOT_COUNT) return;
    auto it = entities.find(id);
    if (it == entities.end()) return;
    EquipOverlay& ov = it->second.equip.overlays[slot];
    try {
        ov.frames.clear();
        ov.frames.emplace_back(renderer, texture::load_surface(path));
        ov.active = true;
        ov.offset_y = offset_y;
        ov.static_frame = static_frame;
    } catch (...) {
        ov.frames.clear();
        ov.active = false;
    }
}

void EntitySpriteRegistry::clear_entity_equipment_overlay(uint16_t id, uint8_t slot) {
    if (slot >= EQUIP_SLOT_COUNT) return;
    auto it = entities.find(id);
    if (it == entities.end()) return;
    EquipOverlay& ov = it->second.equip.overlays[slot];
    ov.frames.clear();
    ov.active = false;
}

void EntitySpriteRegistry::set_entity_body_sprite(uint16_t id, const std::string& path) {
    SpriteRender* movable = find_movable(id);
    if (!movable || path.empty()) return;
    try {
        movable->frames.clear();
        movable->frames.emplace_back(renderer, texture::load_surface(path));
        movable->current_frame = 0;
    } catch (...) {
        reset_entity_body_sprite(id);
    }
}

void EntitySpriteRegistry::reset_entity_body_sprite(uint16_t id) {
    auto it = entities.find(id);
    if (it == entities.end() || it->second.equip.default_body_path.empty()) return;
    set_entity_body_sprite(id, it->second.equip.default_body_path);
}

// ── Queries ───────────────────────────────────────────────────────────────────

bool EntitySpriteRegistry::hit_test_entity(int world_x, int world_y, uint16_t& out_id) const {
    auto hits = [&](const SpriteRender& p) {
        return p.movable && point_in_rect(world_x, world_y, p.dst);
    };
    auto it = std::find_if(entities.begin(), entities.end(), [&](const auto& pair) {
        return std::any_of(pair.second.parts.begin(), pair.second.parts.end(), hits);
    });
    if (it == entities.end()) return false;
    out_id = it->first;
    return true;
}

bool EntitySpriteRegistry::get_entity_world_position(uint16_t id, int& x, int& y) const {
    auto it = entities.find(id);
    if (it == entities.end()) return false;
    auto pit = std::find_if(it->second.parts.begin(), it->second.parts.end(),
                            [](const auto& p) { return p.movable; });
    if (pit == it->second.parts.end()) return false;
    x = pit->dst.GetX() + pit->dst.GetW() / 2;
    y = pit->dst.GetY() + pit->dst.GetH() / 2;
    return true;
}

// ── Rendering ─────────────────────────────────────────────────────────────────

void EntitySpriteRegistry::collect_drawables(const SDL2pp::Rect& cam, std::vector<Drawable>& out) {
    for (auto& [entity_id, e]: entities) {
        append_sprite_drawables(e.parts, cam, out);

        const bool is_npc = e.sprite_id > 0;
        auto body_it = std::find_if(e.parts.begin(), e.parts.end(),
                                    [](const SpriteRender& p) { return p.movable; });
        if (body_it == e.parts.end() || (!body_it->use_src && !is_npc))
            continue;
        if (!is_visible(*body_it, cam))
            continue;
        append_equip_overlay_drawables(*body_it, e.equip.overlays, e.equip.static_cache, cam, out);
    }
}

void EntitySpriteRegistry::render_entity_names(const SDL2pp::Rect& cam) {
    for (auto& [eid, e]: entities) {
        if (!e.name_label.has_value())
            continue;
        auto sit = std::find_if(e.parts.begin(), e.parts.end(),
                                [](const auto& s) { return s.movable; });
        if (sit == e.parts.end() || sit->dst.GetY() + sit->dst.GetH() <= 0)
            continue;

        auto& body = e.parts[0];
        const int name_x =
                body.dst.GetX() + (body.dst.GetW() - e.name_label->w) / 2 - cam.GetX();
        const int name_y = body.dst.GetY() - e.name_label->h - 24 - cam.GetY();

        SDL_Color color = kNameColorDefault;
        if (e.sprite_id > 0)
            color = kNameColorEnemy;
        else if (!local_clan_name.empty() && !e.clan_name.empty() &&
                 e.clan_name == local_clan_name)
            color = kNameColorAlly;

        SDL_Texture* tex = e.name_label->texture.Get();
        SDL_SetTextureColorMod(tex, color.r, color.g, color.b);
        renderer.Copy(e.name_label->texture, SDL2pp::NullOpt,
                      SDL2pp::Rect(name_x, name_y, e.name_label->w, e.name_label->h));
        SDL_SetTextureColorMod(tex, 255, 255, 255);
    }
}
