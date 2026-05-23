#include "prop_renderer.h"

#include <algorithm>

#include "../texture_loader.h"
#include "animation_system.h"

PropRenderer::PropRenderer(SDL2pp::Renderer& renderer): renderer(renderer) {}

void PropRenderer::load(const TilemapConfig& tilemap, int tile_size) {
    prop_tiles_.clear();
    if (tilemap.props.empty())
        return;

    has_tilemap_ = true;
    tile_size_ = tile_size;

    auto tsz = static_cast<std::size_t>(tile_size_);
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
            pr.animated = def.paths.size() > 1;
            pr.hitbox_x = def.hitbox.x;
            pr.hitbox_y = def.hitbox.y;
            pr.hitbox_w = def.hitbox.w;
            pr.hitbox_h = def.hitbox.h;

            for (const auto& path: def.paths) {
                pr.frames.emplace_back(renderer, load_surface(path));
            }
            prop_row.push_back(std::move(pr));
        }
        prop_tiles_.push_back(std::move(prop_row));
    }
}

void PropRenderer::render_conditional(const SDL2pp::Rect& cam, int player_foot_y, bool behind) {
    if (prop_tiles_.empty() || !has_tilemap_)
        return;

    const int first_col = std::max(0, cam.GetX() / tile_size_);
    const int first_row = std::max(0, cam.GetY() / tile_size_);
    const int last_col = std::max(0, (cam.GetX() + cam.GetW() - 1) / tile_size_);
    const int last_row = std::max(0, (cam.GetY() + cam.GetH() - 1) / tile_size_);

    for (int row = first_row; row <= last_row; ++row) {
        if (row < 0 || row >= static_cast<int>(prop_tiles_.size()))
            continue;

        const bool prop_in_front = (row + 1) * tile_size_ > player_foot_y;
        if (behind == prop_in_front)
            continue;

        auto& prop_row = prop_tiles_[static_cast<std::size_t>(row)];
        if (prop_row.empty())
            continue;

        const int row_last_col = std::min(last_col, static_cast<int>(prop_row.size()) - 1);
        for (int col = first_col; col <= row_last_col; ++col) {
            if (col < 0)
                continue;
            auto& prop = prop_row[static_cast<std::size_t>(col)];
            if (prop.frames.empty())
                continue;

            SDL2pp::Rect dst(col * tile_size_ - cam.GetX(),
                             (row + 1) * tile_size_ - cam.GetY() - prop.display_h,
                             prop.display_w, prop.display_h);
            renderer.Copy(prop.frames[prop.current_frame], prop.src, dst);
        }
    }
}

void PropRenderer::render_behind(const SDL2pp::Rect& cam, int player_foot_y) {
    render_conditional(cam, player_foot_y, true);
}

void PropRenderer::render_front(const SDL2pp::Rect& cam, int player_foot_y) {
    render_conditional(cam, player_foot_y, false);
}

void PropRenderer::render_hitboxes(const SDL2pp::Rect& cam) {
    if (prop_tiles_.empty() || !has_tilemap_)
        return;

    const int first_col = std::max(0, cam.GetX() / tile_size_);
    const int first_row = std::max(0, cam.GetY() / tile_size_);
    const int last_col = std::max(0, (cam.GetX() + cam.GetW() - 1) / tile_size_);
    const int last_row = std::max(0, (cam.GetY() + cam.GetH() - 1) / tile_size_);

    SDL_SetRenderDrawBlendMode(renderer.Get(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer.Get(), 255, 0, 0, 128);

    for (int row = first_row; row <= last_row; ++row) {
        if (row < 0 || row >= static_cast<int>(prop_tiles_.size()))
            continue;
        auto& prop_row = prop_tiles_[static_cast<std::size_t>(row)];
        if (prop_row.empty())
            continue;

        const int row_last_col = std::min(last_col, static_cast<int>(prop_row.size()) - 1);
        for (int col = first_col; col <= row_last_col; ++col) {
            if (col < 0)
                continue;
            auto& prop = prop_row[static_cast<std::size_t>(col)];
            if (prop.frames.empty() || prop.hitbox_w <= 0 || prop.hitbox_h <= 0)
                continue;

            const int prop_screen_x = col * tile_size_ - cam.GetX();
            const int prop_screen_y = (row + 1) * tile_size_ - cam.GetY() - prop.display_h;

            SDL2pp::Rect hb_rect(prop_screen_x + prop.hitbox_x, prop_screen_y + prop.hitbox_y,
                                 prop.hitbox_w, prop.hitbox_h);
            renderer.FillRect(hb_rect);
        }
    }
}

void PropRenderer::tick_animations(AnimationSystem& anim) {
    for (auto& row: prop_tiles_) {
        for (auto& prop: row) {
            anim.tick(prop);
        }
    }
}
