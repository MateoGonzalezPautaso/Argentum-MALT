#include "prop_renderer.h"

#include <algorithm>
#include <string>
#include <utility>

#include "../texture_loader.h"

#include "animation_system.h"

PropRenderer::PropRenderer(SDL2pp::Renderer& renderer): renderer(renderer) {}

void PropRenderer::load(const TilemapConfig& tilemap, int tile_size) {
    prop_tiles_.clear();
    frame_cache_.clear();
    if (tilemap.props.empty())
        return;

    has_tilemap_ = true;
    tile_size_ = tile_size;

    auto tsz = static_cast<std::size_t>(tile_size_);
    for (std::size_t ri = 0; ri < tilemap.prop_map.size(); ++ri) {
        const auto& row = tilemap.prop_map[ri];
        std::vector<PropRender> prop_row;
        prop_row.reserve(row.size());
        for (std::size_t ci = 0; ci < row.size(); ++ci) {
            const auto& name = row[ci];
            if (name.empty()) {
                PropRender empty_pr;
                empty_pr.tile_col = static_cast<int>(ci);
                empty_pr.tile_row = static_cast<int>(ri);
                prop_row.push_back(std::move(empty_pr));
                continue;
            }
            auto it = tilemap.props.find(name);
            if (it == tilemap.props.end()) {
                PropRender empty_pr;
                empty_pr.tile_col = static_cast<int>(ci);
                empty_pr.tile_row = static_cast<int>(ri);
                prop_row.push_back(std::move(empty_pr));
                continue;
            }
            const PropDef& def = it->second;
            PropRender pr;
            pr.name = name;
            pr.tile_col = static_cast<int>(ci);
            pr.tile_row = static_cast<int>(ri);
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
                auto it = frame_cache_.find(path);
                if (it == frame_cache_.end()) {
                    it = frame_cache_
                                 .emplace(path,
                                          std::make_shared<SDL2pp::Texture>(
                                                  renderer, texture::load_surface(path)))
                                 .first;
                }
                pr.frames.push_back(it->second);
            }

            for (const auto& part_def: def.parts) {
                PropPart pp;
                pp.src = SDL2pp::Rect(part_def.src_x, part_def.src_y, part_def.src_w,
                                      part_def.src_h);
                pp.offset_x = part_def.offset_x;
                pp.offset_y = part_def.offset_y;
                pp.display_w = part_def.src_w;
                pp.display_h = part_def.src_h;
                auto it = frame_cache_.find(part_def.path);
                if (it == frame_cache_.end()) {
                    it = frame_cache_
                                 .emplace(part_def.path,
                                          std::make_shared<SDL2pp::Texture>(
                                                  renderer, texture::load_surface(part_def.path)))
                                 .first;
                }
                pp.frames.push_back(it->second);
                pr.parts.push_back(std::move(pp));
            }
            if (!pr.parts.empty() && def.width > 0 && def.height > 0) {
                int min_x = 0, min_y = 0, max_x = 0, max_y = 0;
                {
                    const auto& p0 = pr.parts[0];
                    min_x = p0.offset_x;
                    max_x = p0.offset_x + p0.display_w;
                    min_y = p0.offset_y;
                    max_y = p0.offset_y + p0.display_h;
                }
                for (std::size_t i = 1; i < pr.parts.size(); ++i) {
                    const auto& p = pr.parts[i];
                    min_x = std::min(min_x, p.offset_x);
                    min_y = std::min(min_y, p.offset_y);
                    max_x = std::max(max_x, p.offset_x + p.display_w);
                    max_y = std::max(max_y, p.offset_y + p.display_h);
                }
                int nat_w = max_x - min_x;
                int nat_h = max_y - min_y;
                if (nat_w > 0 && nat_h > 0) {
                    float sx = static_cast<float>(def.width) / nat_w;
                    float sy = static_cast<float>(def.height) / nat_h;
                    for (auto& part: pr.parts) {
                        part.offset_x = static_cast<int>((part.offset_x - min_x) * sx);
                        part.offset_y = static_cast<int>((part.offset_y - min_y) * sy);
                        part.display_w = std::max(1, static_cast<int>(part.display_w * sx));
                        part.display_h = std::max(1, static_cast<int>(part.display_h * sy));
                    }
                }
            }

            prop_row.push_back(std::move(pr));
        }
        prop_tiles_.push_back(std::move(prop_row));
    }
}

void PropRenderer::render_conditional(const SDL2pp::Rect& cam, int player_foot_y, bool behind) {
    if (prop_tiles_.empty() || !has_tilemap_)
        return;

    constexpr int extra = 2;
    const int first_col = std::max(0, cam.GetX() / tile_size_ - extra);
    const int first_row = std::max(0, cam.GetY() / tile_size_ - extra);
    const int last_col = (cam.GetX() + cam.GetW() - 1) / tile_size_ + extra;
    const int last_row = (cam.GetY() + cam.GetH() - 1) / tile_size_ + extra;

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
            if (prop.frames.empty() && prop.parts.empty())
                continue;

            SDL2pp::Rect dst(col * tile_size_ - cam.GetX(),
                             (row + 1) * tile_size_ - cam.GetY() - prop.display_h, prop.display_w,
                             prop.display_h);
            if (prop.parts.empty()) {
                renderer.Copy(*prop.frames[prop.current_frame], prop.src, dst);
            } else {
                for (auto& part: prop.parts) {
                    SDL2pp::Rect part_dst(dst.GetX() + part.offset_x, dst.GetY() + part.offset_y,
                                          part.display_w, part.display_h);
                    renderer.Copy(*part.frames[part.current_frame], part.src, part_dst);
                }
            }
        }
    }
}

bool PropRenderer::hit_test_prop(int world_x, int world_y, std::string& out_prop_name) const {
    for (int ri = static_cast<int>(prop_tiles_.size()) - 1; ri >= 0; --ri) {
        for (int ci = static_cast<int>(prop_tiles_[static_cast<std::size_t>(ri)].size()) - 1;
             ci >= 0; --ci) {
            const auto& prop =
                    prop_tiles_[static_cast<std::size_t>(ri)][static_cast<std::size_t>(ci)];
            if (prop.frames.empty() && prop.parts.empty())
                continue;
            int prop_world_x = ci * tile_size_;
            int prop_world_y = (ri + 1) * tile_size_ - prop.display_h;
            SDL2pp::Rect rect(prop_world_x, prop_world_y, prop.display_w, prop.display_h);
            if (world_x >= rect.GetX() && world_x < rect.GetX() + rect.GetW() &&
                world_y >= rect.GetY() && world_y < rect.GetY() + rect.GetH()) {
                out_prop_name = prop.name;
                return true;
            }
        }
    }
    return false;
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

    constexpr int extra = 2;
    const int first_col = std::max(0, cam.GetX() / tile_size_ - extra);
    const int first_row = std::max(0, cam.GetY() / tile_size_ - extra);
    const int last_col = (cam.GetX() + cam.GetW() - 1) / tile_size_ + extra;
    const int last_row = (cam.GetY() + cam.GetH() - 1) / tile_size_ + extra;

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
            if ((prop.frames.empty() && prop.parts.empty()) || prop.hitbox_w <= 0 ||
                prop.hitbox_h <= 0)
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
            for (auto& part: prop.parts) {
                anim.tick(part);
            }
        }
    }
}
