#include "map.h"

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

Map::Map(const TilemapConfig& tilemap):
        tilemap_(&tilemap),
        prop_grid_(tilemap),
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

    if (prop_grid_.is_hitbox_at(foot_x, foot_y))
        return false;

    return true;
}

bool Map::is_mob_spawn_tile(int row, int col) const {
    if (tilemap_->mob_spawn_zones.empty())
        return false;

    auto r = static_cast<std::size_t>(row);
    auto c = static_cast<std::size_t>(col);

    if (r >= tilemap_->mob_spawn_zones.size())
        return false;
    if (c >= tilemap_->mob_spawn_zones[r].size())
        return false;

    return tilemap_->mob_spawn_zones[r][c];
}

std::optional<std::pair<int, int>> Map::find_random_mob_spawn_pos(Rng& rng) const {
    if (tilemap_->mob_spawn_zones.empty())
        return std::nullopt;

    struct Candidate {
        int px;
        int py;
    };
    std::vector<Candidate> candidates;
    int tsz = tilemap_->tile_size;

    for (std::size_t r = 0; r < tilemap_->mob_spawn_zones.size(); ++r) {
        for (std::size_t c = 0; c < tilemap_->mob_spawn_zones[r].size(); ++c) {
            if (!tilemap_->mob_spawn_zones[r][c])
                continue;

            int foot_x = static_cast<int>(c) * tsz + tsz / 2;
            int foot_y = static_cast<int>(r) * tsz + tsz / 2;

            if (is_walkable(foot_x, foot_y)) {
                candidates.push_back({foot_x, foot_y});
            }
        }
    }

    if (candidates.empty())
        return std::nullopt;

    int idx = rng.get_random_int(0, static_cast<int>(candidates.size()) - 1);
    return std::make_pair(candidates[idx].px, candidates[idx].py);
}
