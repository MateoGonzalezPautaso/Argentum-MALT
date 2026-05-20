#include "world_renderer.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include <SDL2/SDL.h>

#include "texture_loader.h"

WorldRenderer::WorldRenderer(SDL2pp::Renderer& renderer, const BackgroundConfig& background,
                             const TilemapConfig& tilemap,
                             const std::vector<SpriteConfig>& sprites_config, int window_w,
                             int window_h):
        renderer(renderer),
        background_texture(renderer, load_surface(background.path)),
        background_rect(background.x, background.y, background.width, background.height),
        game_viewport(11, 149, 734, 608),
        window_w(window_w),
        window_h(window_h) {
    init_tilemap(tilemap);
    init_props(tilemap);
    init_sprites(sprites_config);

    if (sprites.empty()) {
        throw std::runtime_error("No sprites to render");
    }
}

void WorldRenderer::render() {
    renderer.SetViewport(game_viewport);

    const SDL2pp::Rect cam = camera_rect();
    render_tilemap_or_background(cam);

    int player_foot_y = 0;
    const SpriteRender* movable = find_movable_sprite();
    if (movable) {
        player_foot_y = movable->dst.GetY() + movable->dst.GetH();
    }

    render_props(cam, player_foot_y, true);

    update_animation();
    update_anchor_positions();
    render_sprites(cam);
    render_props(cam, player_foot_y, false);

    if (show_hitboxes_) { //PARA HITBOX DEBUG ONLY - BORRAR EN PROD
        render_props_hitboxes(cam); //PARA HITBOX DEBUG ONLY - BORRAR EN PROD
    }

    renderer.SetViewport(SDL2pp::NullOpt);
}

void WorldRenderer::set_movable_position(int x, int y) {
    SpriteRender* sprite = find_movable_sprite();
    if (!sprite) {
        return;
    }
    const int max_x = has_tilemap ? std::max(0, map_px_w - sprite->dst.GetW()) :
                                    std::max(0, window_w - sprite->dst.GetW());
    const int max_y = has_tilemap ? std::max(0, map_px_h - sprite->dst.GetH()) :
                                    std::max(0, window_h - sprite->dst.GetH());

    // If server sets something in (x, y), we trust it
    // Just use it in LoginOkEvent, EntitySpawnEvent and EntityMoveEvent
    sprite->dst.SetX(std::clamp(x, 0, max_x));
    sprite->dst.SetY(std::clamp(y, 0, max_y));
}

bool WorldRenderer::get_movable_position(int& x, int& y) const {
    const SpriteRender* sprite = find_movable_sprite();
    if (!sprite) {
        return false;
    }
    x = sprite->dst.GetX();
    y = sprite->dst.GetY();
    return true;
}

void WorldRenderer::get_camera_offset(int& x, int& y) const {
    const SDL2pp::Rect cam = camera_rect();
    x = cam.GetX();
    y = cam.GetY();
}

bool WorldRenderer::screen_to_world(int screen_x, int screen_y, int& world_x, int& world_y) const {
    // convierte click en coordenadas de mundo teniendo en cuenta viewport + camara.
    const int local_x = screen_x - game_viewport.GetX();
    const int local_y = screen_y - game_viewport.GetY();
    if (local_x < 0 || local_y < 0 || local_x >= game_viewport.GetW() ||
        local_y >= game_viewport.GetH()) {
        return false;
    }

    int cam_x = 0;
    int cam_y = 0;
    get_camera_offset(cam_x, cam_y);
    world_x = local_x + cam_x;
    world_y = local_y + cam_y;
    return true;
}

void WorldRenderer::set_movable_src_y(int y) {
    SpriteRender* sprite = find_movable_sprite();
    if (!sprite || !sprite->use_src) {
        return;
    }
    sprite->src.SetY(y);
}

void WorldRenderer::step_movable_src_x(int step, int frame_count) {
    SpriteRender* sprite = find_movable_sprite();
    if (!sprite || !sprite->use_src) {
        return;
    }
    if (frame_count <= 0 || step <= 0) {
        return;
    }
    const int current_index = sprite->src.GetX() / step;
    const int next_index = (current_index + 1) % frame_count;
    sprite->src.SetX(next_index * step);
}

void WorldRenderer::set_anchor_src_y(int y) {
    for (auto& sprite: sprites) {
        if (!sprite.anchor_to_movable || !sprite.use_src) {
            continue;
        }
        sprite.src.SetY(y);
    }
}

void WorldRenderer::init_tilemap(const TilemapConfig& tilemap) {
    if (tilemap.path.empty() || tilemap.mapa.empty() || tilemap.tiles.empty()) {
        has_tilemap = false;
        return;
    }

    has_tilemap = true;
    tile_size = tilemap.tile_size;
    tilemap_tiles_.clear();
    tilemap_tiles_.reserve(tilemap.mapa.size());
    tilemap_textures_.clear();
    map_px_w = 0;
    map_px_h = 0;

    for (const auto& row: tilemap.mapa) {
        std::vector<TileSrcInfo> tile_row;
        tile_row.reserve(row.size());
        map_px_w = std::max(map_px_w, static_cast<int>(row.size()) * tile_size);
        for (const auto& name: row) {
            auto it = tilemap.tiles.find(name);
            if (it == tilemap.tiles.end()) {
                tile_row.push_back({{0, 0, tile_size, tile_size}, tilemap.path});
                continue;
            }
            const TileDef& def = it->second;
            std::string atlas_path = def.path.empty() ? tilemap.path : def.path;
            tile_row.push_back({{def.x, def.y, tile_size, tile_size}, atlas_path});

            if (tilemap_textures_.find(atlas_path) == tilemap_textures_.end()) {
                tilemap_textures_.emplace(atlas_path,
                    SDL2pp::Texture(renderer, load_surface(atlas_path)));
            }
        }
        tilemap_tiles_.push_back(std::move(tile_row));
    }

    map_px_h = static_cast<int>(tilemap_tiles_.size()) * tile_size;
}

void WorldRenderer::init_props(const TilemapConfig& tilemap) {
    prop_tiles_.clear();
    if (tilemap.props.empty()) return;

    auto tsz = static_cast<std::size_t>(tile_size);
    for (const auto& row: tilemap.prop_map) {
        std::vector<PropRender> prop_row;
        prop_row.reserve(row.size());
        for (const auto& name: row) {
            if (name.empty()) {
                prop_row.push_back({});
                continue;
            }
            auto it = tilemap.props.find(name);
            if (it == tilemap.props.end()) {
                prop_row.push_back({});
                continue;
            }
            const PropDef& def = it->second;
            PropRender pr;
            pr.src = SDL2pp::Rect(def.src_x, def.src_y, def.src_w, def.src_h);
            pr.display_w = def.width > 0 ? def.width : static_cast<int>(tsz);
            pr.display_h = def.height > 0 ? def.height : static_cast<int>(tsz);
            pr.frame_ms = def.frame_ms;
            pr.current_frame = 0;
            pr.last_ticks = SDL_GetTicks();
            pr.hitbox_x = def.hitbox.x; //PARA HITBOX DEBUG ONLY - BORRAR EN PROD
            pr.hitbox_y = def.hitbox.y; //PARA HITBOX DEBUG ONLY - BORRAR EN PROD
            pr.hitbox_w = def.hitbox.w; //PARA HITBOX DEBUG ONLY - BORRAR EN PROD
            pr.hitbox_h = def.hitbox.h; //PARA HITBOX DEBUG ONLY - BORRAR EN PROD

            for (const auto& path: def.paths) {
                pr.frames.emplace_back(renderer, load_surface(path));
            }
            prop_row.push_back(std::move(pr));
        }
        prop_tiles_.push_back(std::move(prop_row));
    }
}

void WorldRenderer::init_sprites(const std::vector<SpriteConfig>& sprites_config) {
    sprites.reserve(sprites_config.size());
    for (const auto& sprite_config: sprites_config) {
        SpriteRender sprite = build_sprite_render(sprite_config);
        if (sprite.frames.empty()) {
            continue;
        }
        sprites.push_back(std::move(sprite));
    }
}

WorldRenderer::SpriteRender WorldRenderer::build_sprite_render(const SpriteConfig& sprite_config) {
    SpriteRender sprite;
    sprite.frames.reserve(sprite_config.paths.size());
    for (const auto& path: sprite_config.paths) {
        SDL2pp::Surface surface = load_surface(path);
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

SDL2pp::Rect WorldRenderer::camera_rect() const {
    // camara centrada en el jugador movible y acotada al mapa
    const SpriteRender* sprite = find_movable_sprite();
    int cam_x = 0;
    int cam_y = 0;
    if (sprite) {
        cam_x = sprite->dst.GetX() + sprite->dst.GetW() / 2 - game_viewport.GetW() / 2;
        cam_y = sprite->dst.GetY() + sprite->dst.GetH() / 2 - game_viewport.GetH() / 2;
    }

    if (has_tilemap) {
        const int max_x = std::max(0, map_px_w - game_viewport.GetW());
        const int max_y = std::max(0, map_px_h - game_viewport.GetH());
        cam_x = std::clamp(cam_x, 0, max_x);
        cam_y = std::clamp(cam_y, 0, max_y);
    } else {
        cam_x = std::max(0, cam_x);
        cam_y = std::max(0, cam_y);
    }

    return SDL2pp::Rect(cam_x, cam_y, game_viewport.GetW(), game_viewport.GetH());
}

void WorldRenderer::render_tilemap_or_background(const SDL2pp::Rect& cam) {
    if (has_tilemap) {
        const int first_col = std::max(0, cam.GetX() / tile_size);
        const int first_row = std::max(0, cam.GetY() / tile_size);
        const int last_col = std::max(0, (cam.GetX() + cam.GetW() - 1) / tile_size);
        const int last_row = std::max(0, (cam.GetY() + cam.GetH() - 1) / tile_size);

        for (int row = first_row; row <= last_row; ++row) {
            if (row < 0 || row >= static_cast<int>(tilemap_tiles_.size())) {
                continue;
            }

            const auto& tile_row = tilemap_tiles_[static_cast<std::size_t>(row)];
            const int row_last_col = std::min(last_col, static_cast<int>(tile_row.size()) - 1);
            for (int col = first_col; col <= row_last_col; ++col) {
                if (col < 0) {
                    continue;
                }

                const auto& tile = tile_row[static_cast<std::size_t>(col)];
                auto tex_it = tilemap_textures_.find(tile.atlas_path);
                if (tex_it == tilemap_textures_.end()) {
                    continue;
                }

                SDL2pp::Rect dst(col * tile_size - cam.GetX(), row * tile_size - cam.GetY(),
                                 tile_size, tile_size);
                renderer.Copy(tex_it->second, tile.src, dst);
            }
        }

        return;
    }

    const SDL2pp::Rect gameplay_bg(0, 0, game_viewport.GetW(), game_viewport.GetH());
    renderer.Copy(background_texture, SDL2pp::NullOpt, gameplay_bg);
}

void WorldRenderer::render_props(const SDL2pp::Rect& cam, int player_foot_y, bool behind) {
    if (prop_tiles_.empty() || !has_tilemap) return;

    const int first_col = std::max(0, cam.GetX() / tile_size);
    const int first_row = std::max(0, cam.GetY() / tile_size);
    const int last_col = std::max(0, (cam.GetX() + cam.GetW() - 1) / tile_size);
    const int last_row = std::max(0, (cam.GetY() + cam.GetH() - 1) / tile_size);

    for (int row = first_row; row <= last_row; ++row) {
        if (row < 0 || row >= static_cast<int>(prop_tiles_.size())) continue;

        const int prop_depth = (row + 1) * tile_size;
        if (behind && prop_depth > player_foot_y) continue;
        if (!behind && prop_depth <= player_foot_y) continue;

        auto& prop_row = prop_tiles_[static_cast<std::size_t>(row)];
        if (prop_row.empty()) continue;

        const int row_last_col = std::min(last_col, static_cast<int>(prop_row.size()) - 1);
        for (int col = first_col; col <= row_last_col; ++col) {
            if (col < 0) continue;
            auto& prop = prop_row[static_cast<std::size_t>(col)];
            if (prop.frames.empty()) continue;

            SDL2pp::Rect dst(
                col * tile_size - cam.GetX(),
                (row + 1) * tile_size - cam.GetY() - prop.display_h,
                prop.display_w, prop.display_h);
            renderer.Copy(prop.frames[prop.current_frame], prop.src, dst);
        }
    }
}

void WorldRenderer::render_props_hitboxes(const SDL2pp::Rect& cam) { //PARA HITBOX DEBUG ONLY - BORRAR EN PROD
    if (prop_tiles_.empty() || !has_tilemap) return;

    const int first_col = std::max(0, cam.GetX() / tile_size);
    const int first_row = std::max(0, cam.GetY() / tile_size);
    const int last_col = std::max(0, (cam.GetX() + cam.GetW() - 1) / tile_size);
    const int last_row = std::max(0, (cam.GetY() + cam.GetH() - 1) / tile_size);

    SDL_SetRenderDrawBlendMode(renderer.Get(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer.Get(), 255, 0, 0, 128);

    for (int row = first_row; row <= last_row; ++row) {
        if (row < 0 || row >= static_cast<int>(prop_tiles_.size())) continue;
        auto& prop_row = prop_tiles_[static_cast<std::size_t>(row)];
        if (prop_row.empty()) continue;

        const int row_last_col = std::min(last_col, static_cast<int>(prop_row.size()) - 1);
        for (int col = first_col; col <= row_last_col; ++col) {
            if (col < 0) continue;
            auto& prop = prop_row[static_cast<std::size_t>(col)];
            if (prop.frames.empty() || prop.hitbox_w <= 0 || prop.hitbox_h <= 0) continue;

            int prop_screen_x = col * tile_size - cam.GetX();
            int prop_screen_y = (row + 1) * tile_size - cam.GetY() - prop.display_h;

            SDL2pp::Rect hb_rect(
                prop_screen_x + prop.hitbox_x,
                prop_screen_y + prop.hitbox_y,
                prop.hitbox_w, prop.hitbox_h);
            renderer.FillRect(hb_rect);
        }
    }
}

void WorldRenderer::render_sprites(const SDL2pp::Rect& cam) {
    // dibuja sprites en coordenadas de camara.
    // primero descarta los que no intersectan el viewport para optimizar
    for (auto& sprite: sprites) {
        if (!sprite.visible || sprite.frames.empty()) {
            continue;
        }

        const int sprite_left = sprite.dst.GetX();
        const int sprite_top = sprite.dst.GetY();
        const int sprite_right = sprite_left + sprite.dst.GetW();
        const int sprite_bottom = sprite_top + sprite.dst.GetH();
        const int cam_left = cam.GetX();
        const int cam_top = cam.GetY();
        const int cam_right = cam_left + cam.GetW();
        const int cam_bottom = cam_top + cam.GetH();

        if (sprite_right <= cam_left || sprite_left >= cam_right || sprite_bottom <= cam_top ||
            sprite_top >= cam_bottom) {
            continue;
        }

        SDL2pp::Rect dst(sprite.dst.GetX() - cam.GetX(), sprite.dst.GetY() - cam.GetY(),
                         sprite.dst.GetW(), sprite.dst.GetH());
        if (sprite.use_src) {
            renderer.Copy(sprite.frames[sprite.current_frame], sprite.src, dst);
        } else {
            renderer.Copy(sprite.frames[sprite.current_frame], SDL2pp::NullOpt, dst);
        }
    }
}

void WorldRenderer::update_animation() {
    const uint32_t now = SDL_GetTicks();
    for (auto& sprite: sprites) {
        if (!sprite.animated || sprite.frame_ms == 0) {
            continue;
        }
        if (now - sprite.last_ticks < sprite.frame_ms) {
            continue;
        }
        sprite.last_ticks = now;
        sprite.current_frame = (sprite.current_frame + 1) % sprite.frames.size();
    }

    // Animate props
    for (auto& row : prop_tiles_) {
        for (auto& prop : row) {
            if (prop.frames.size() <= 1 || prop.frame_ms == 0) {
                continue;
            }
            if (now - prop.last_ticks < prop.frame_ms) {
                continue;
            }
            prop.last_ticks = now;
            prop.current_frame = (prop.current_frame + 1) % prop.frames.size();
        }
    }
}

void WorldRenderer::update_anchor_positions() {
    SpriteRender* movable = find_movable_sprite();
    if (!movable) {
        return;
    }

    const int max_x = has_tilemap ? std::max(0, map_px_w - movable->dst.GetW()) :
                                    std::max(0, window_w - movable->dst.GetW());
    const int max_y = has_tilemap ? std::max(0, map_px_h - movable->dst.GetH()) :
                                    std::max(0, window_h - movable->dst.GetH());
    const int base_x = std::clamp(movable->dst.GetX(), 0, max_x);
    const int base_y = std::clamp(movable->dst.GetY(), 0, max_y);

    for (auto& sprite: sprites) {
        if (!sprite.anchor_to_movable) {
            continue;
        }
        const int desired_x = base_x + sprite.anchor_offset_x;
        const int desired_y = base_y + sprite.anchor_offset_y;
        const int clamp_x_max = has_tilemap ? std::max(0, map_px_w - sprite.dst.GetW()) :
                                              std::max(0, window_w - sprite.dst.GetW());
        const int clamp_y_max = has_tilemap ? std::max(0, map_px_h - sprite.dst.GetH()) :
                                              std::max(0, window_h - sprite.dst.GetH());
        const int clamped_x = std::clamp(desired_x, 0, clamp_x_max);
        const int clamped_y = std::clamp(desired_y, 0, clamp_y_max);
        sprite.dst.SetX(clamped_x);
        sprite.dst.SetY(clamped_y);
    }
}

WorldRenderer::SpriteRender* WorldRenderer::find_movable_sprite() {
    auto it = std::find_if(sprites.begin(), sprites.end(),
                           [](const SpriteRender& s) { return s.movable; });
    return it != sprites.end() ? &(*it) : nullptr;
}

const WorldRenderer::SpriteRender* WorldRenderer::find_movable_sprite() const {
    auto it = std::find_if(sprites.begin(), sprites.end(),
                           [](const SpriteRender& s) { return s.movable; });
    return it != sprites.end() ? &(*it) : nullptr;
}
