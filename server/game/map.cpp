#include "map.h"

#include <algorithm>

Map::Map(const TilemapConfig& tilemap):
        tilemap_(&tilemap),
        map_px_w(static_cast<int>(tilemap.mapa.empty() ? 0 : tilemap.mapa.front().size()) *
                 tilemap.tile_size),
        map_px_h(static_cast<int>(tilemap.mapa.size()) * tilemap.tile_size) {}

int Map::clamp_x(int x, int entity_w) const {
    if (map_px_w <= 0) {
        return 0;
    }
    const int max_x = map_px_w - entity_w;
    return std::max(0, std::min(x, max_x));
}

int Map::clamp_y(int y, int entity_h) const {
    if (map_px_h <= 0) {
        return 0;
    }
    const int max_y = map_px_h - entity_h;
    return std::max(0, std::min(y, max_y));
}

bool Map::is_walkable(int foot_x, int foot_y) const {
    if (tilemap_->mapa.empty() || tilemap_->tile_size <= 0) {
        return true;
    }

    const int col = foot_x / tilemap_->tile_size;
    const int row = foot_y / tilemap_->tile_size;

    if (row < 0 || row >= static_cast<int>(tilemap_->mapa.size())) {
        return false;
    }
    if (col < 0 || col >= static_cast<int>(tilemap_->mapa[static_cast<std::size_t>(row)].size())) {
        return false;
    }

    const auto& name = tilemap_->mapa[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
    auto it = tilemap_->tiles.find(name);
    if (it != tilemap_->tiles.end() && !it->second.walkable)
        return false;

    if (!tilemap_->prop_map.empty()) {
        for (std::size_t r = 0; r < tilemap_->prop_map.size(); ++r) {
            for (std::size_t c = 0; c < tilemap_->prop_map[r].size(); ++c) {
                const auto& prop_name = tilemap_->prop_map[r][c];
                if (prop_name.empty())
                    continue;

                auto prop_it = tilemap_->props.find(prop_name);
                if (prop_it == tilemap_->props.end())
                    continue;

                const auto& prop = prop_it->second;
                if (prop.hitbox.w <= 0 || prop.hitbox.h <= 0)
                    continue;

                int prop_x = static_cast<int>(c) * tilemap_->tile_size;
                int prop_y = (static_cast<int>(r) + 1) * tilemap_->tile_size - prop.height;

                int hb_left = prop_x + prop.hitbox.x;
                int hb_top = prop_y + prop.hitbox.y;
                int hb_right = hb_left + prop.hitbox.w;
                int hb_bottom = hb_top + prop.hitbox.h;

                if (foot_x >= hb_left && foot_x < hb_right && foot_y >= hb_top &&
                    foot_y < hb_bottom)
                    return false;
            }
        }
    }

    return true;
}
