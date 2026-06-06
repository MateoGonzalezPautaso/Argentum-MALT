#include "sprite_renderer.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <utility>

#include <SDL2/SDL.h>

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

void SpriteRenderer::set_movable_position(int x, int y) {
    SpriteRender* sprite = find_movable_sprite();
    if (!sprite) {
        return;
    }
    sprite->dst.SetX(clamp_x(x, sprite->dst.GetW()));
    sprite->dst.SetY(clamp_y(y, sprite->dst.GetH()));
}

SpriteConfig SpriteRenderer::resolve_entity_skin(const SpriteConfig& config, Race race,
                                                 PlayerClass player_class) const {
    SpriteConfig selected = config;
    if (config.movable && !config.anchor_to_movable) {
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
        int x, int y, Race race, PlayerClass player_class) {
    std::vector<SpriteRender> parts;
    parts.reserve(entity_part_configs.size());

    for (const auto& config: entity_part_configs) {
        SpriteConfig selected = resolve_entity_skin(config, race, player_class);
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

void SpriteRenderer::position_anchored_parts(std::vector<SpriteRender>& parts) {
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
    entity_name_render.emplace(
            entity_id, EntityNameRender{SDL2pp::Texture(renderer, wrapped), text_w, text_h});
}

void SpriteRenderer::spawn_entity(uint16_t entity_id, int x, int y, const std::string& name,
                                  Race race, PlayerClass player_class) {
    if (entity_part_configs.empty()) {
        return;
    }
    auto parts = build_entity_parts(x, y, race, player_class);
    if (parts.empty()) {
        return;
    }
    position_anchored_parts(parts);
    entity_sprites[entity_id] = std::move(parts);
    create_entity_name_label(entity_id, name);
}

void SpriteRenderer::despawn_entity(uint16_t entity_id) {
    entity_sprites.erase(entity_id);
    entity_name_render.erase(entity_id);
}

void SpriteRenderer::clear_all_entities() {
    entity_sprites.clear();
    entity_name_render.clear();
}

void SpriteRenderer::set_skin_config(const SkinConfig& cfg) { skin_config = cfg; }

void SpriteRenderer::set_local_player_info(Race race, PlayerClass player_class) {
    if (entity_part_configs.empty()) {
        return;
    }

    for (std::size_t i = 0; i < entity_part_configs.size() && i < sprites.size(); ++i) {
        const auto& config = entity_part_configs[i];
        SpriteConfig selected = resolve_entity_skin(config, race, player_class);
        sprites[i] = build_sprite_render(selected);
    }
}

void SpriteRenderer::move_entity(uint16_t entity_id, int x, int y) {
    auto it = entity_sprites.find(entity_id);
    if (it == entity_sprites.end()) {
        return;
    }

    auto& parts = it->second;
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

void SpriteRenderer::step_movable_src_x(int step, int frame_count) {
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
    auto it = entity_sprites.find(entity_id);
    if (it == entity_sprites.end()) {
        return;
    }
    for (auto& sprite: it->second) {
        if (!sprite.use_src) {
            continue;
        }
        if (sprite.movable) {
            sprite.src.SetY(body_src_y);
        } else if (sprite.anchor_to_movable) {
            sprite.src.SetY(head_src_y);
        }
    }
}

void SpriteRenderer::step_entity_src_x(uint16_t entity_id, int step, int frame_count) {
    auto it = entity_sprites.find(entity_id);
    if (it == entity_sprites.end()) {
        return;
    }
    for (auto& sprite: it->second) {
        if (!sprite.movable) {
            continue;
        }
        advance_src_x(sprite, step, frame_count);
    }
}

void SpriteRenderer::set_entity_alpha(uint16_t entity_id, uint8_t alpha) {
    auto it = entity_sprites.find(entity_id);
    if (it == entity_sprites.end()) {
        return;
    }
    for (auto& sprite: it->second) {
        sprite.alpha = alpha;
    }
}

void SpriteRenderer::set_movable_alpha(uint8_t alpha) {
    for (auto& sprite: sprites) {
        if (sprite.movable) {
            sprite.alpha = alpha;
        }
    }
}

void SpriteRenderer::update_anchor_positions() {
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

void SpriteRenderer::render(const SDL2pp::Rect& cam) {
    std::vector<Drawable> drawables;

    append_sprite_drawables(sprites, cam, drawables);
    for (auto& pair: entity_sprites) {
        append_sprite_drawables(pair.second, cam, drawables);
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
        out.push_back(Drawable{&sprite.frames[sprite.current_frame],
                               sprite.use_src ? &sprite.src : nullptr, dst,
                               sprite.dst.GetY() + sprite.dst.GetH(), sprite.alpha});
    }
}

void SpriteRenderer::render_entity_names(const SDL2pp::Rect& cam) {
    for (auto& pair: entity_sprites) {
        const uint16_t eid = pair.first;
        auto name_it = entity_name_render.find(eid);
        if (name_it == entity_name_render.end()) {
            continue;
        }
        int body_foot_y = 0;
        auto sit = std::find_if(pair.second.begin(), pair.second.end(),
                                [](const auto& s) { return s.movable; });
        if (sit != pair.second.end())
            body_foot_y = sit->dst.GetY() + sit->dst.GetH();
        if (body_foot_y <= 0) {
            continue;
        }
        auto& body = pair.second[0];
        const int name_x = body.dst.GetX() + (body.dst.GetW() - name_it->second.w) / 2 - cam.GetX();
        const int name_y = body.dst.GetY() - name_it->second.h - 24 - cam.GetY();
        renderer.Copy(name_it->second.texture, SDL2pp::NullOpt,
                      SDL2pp::Rect(name_x, name_y, name_it->second.w, name_it->second.h));
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
        if (d.src) {
            renderer.Copy(*d.texture, *d.src, d.dst);
        } else {
            renderer.Copy(*d.texture, SDL2pp::NullOpt, d.dst);
        }
        if (d.alpha < 255) {
            SDL_SetTextureAlphaMod(tex, 255);
        }
    }
}

void SpriteRenderer::load_damage_overlay() {
    overlays.resize(3);
    for (auto& ov: overlays) {
        ov.frames.reserve(5);
        for (int i = 400; i <= 404; ++i) {
            std::string path = "assets/Graficos/" + std::to_string(i) + ".png";
            SDL2pp::Surface surface = texture::load_surface(path);
            ov.frames.emplace_back(renderer, surface);
        }
        ov.frame_ms = 70;
        ov.dst.SetW(32);
        ov.dst.SetH(32);
    }
}

void SpriteRenderer::load_spell_overlay() {
    std::string path = "assets/Graficos/3470.png";
    SDL2pp::Surface sheet = texture::load_surface(path);
    spell_overlays.resize(3);
    for (auto& ov: spell_overlays) {
        ov.frames.reserve(5);
        for (int i = 0; i < 5; ++i) {
            SDL_Surface* sub = SDL_CreateRGBSurfaceWithFormat(0, 128, 135, 32,
                    sheet.Get()->format->format);
            SDL_Rect src{i * 128, 0, 128, 135};
            SDL_BlitSurface(sheet.Get(), &src, sub, nullptr);
            ov.frames.emplace_back(renderer, SDL2pp::Surface(sub));
        }
        ov.frame_ms = 100;
        ov.dst.SetW(77);
        ov.dst.SetH(81);
    }
}

void SpriteRenderer::trigger_damage_overlay_at(int world_x, int world_y) {
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

void SpriteRenderer::trigger_spell_overlay_at(int world_x, int world_y) {
    for (auto& ov: spell_overlays) {
        if (ov.active)
            continue;
        ov.dst.SetX(world_x - ov.dst.GetW() / 2 - 7);
        ov.dst.SetY(world_y - ov.dst.GetH() / 2 - 15);
        ov.current_frame = 0;
        ov.last_ticks = SDL_GetTicks();
        ov.active = true;
        return;
    }
}

bool SpriteRenderer::get_entity_world_position(uint16_t entity_id, int& x, int& y) const {
    auto it = entity_sprites.find(entity_id);
    if (it == entity_sprites.end())
        return false;
    auto pit = std::find_if(it->second.begin(), it->second.end(),
                            [](const auto& p) { return p.movable; });
    if (pit == it->second.end())
        return false;
    x = pit->dst.GetX() + pit->dst.GetW() / 2;
    y = pit->dst.GetY() + pit->dst.GetH() / 2;
    return true;
}

void SpriteRenderer::tick_overlays(const AnimationSystem& anim) {
    (void)anim;
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
    tick_pool(spell_overlays);
}

void SpriteRenderer::render_overlays(const SDL2pp::Rect& cam) {
    auto render_pool = [&](std::vector<OverlayEffect>& pool) {
        for (auto& ov: pool) {
            if (!ov.active)
                continue;
            SDL2pp::Rect dst(ov.dst.GetX() - cam.GetX(), ov.dst.GetY() - cam.GetY(),
                             ov.dst.GetW(), ov.dst.GetH());
            renderer.Copy(ov.frames[ov.current_frame], SDL2pp::NullOpt, dst);
        }
    };
    render_pool(overlays);
    render_pool(spell_overlays);
}

void SpriteRenderer::tick_animations(AnimationSystem& anim) {
    for (auto& sprite: sprites) {
        anim.tick(sprite);
    }
    for (auto& pair: entity_sprites) {
        for (auto& sprite: pair.second) {
            anim.tick(sprite);
        }
    }
}

bool SpriteRenderer::hit_test_entity(int world_x, int world_y, uint16_t& out_entity_id) const {
    auto hits_point = [&](const SpriteRender& part) {
        return part.movable && point_in_rect(world_x, world_y, part.dst);
    };
    auto it = std::find_if(entity_sprites.begin(), entity_sprites.end(), [&](const auto& pair) {
        return std::any_of(pair.second.begin(), pair.second.end(), hits_point);
    });
    if (it != entity_sprites.end()) {
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
