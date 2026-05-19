#include "map.h"

#include <algorithm>

Map::Map(const TilemapConfig& tilemap):
        tilemap(tilemap),
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
    if (tilemap.mapa.empty() || tilemap.tile_size <= 0) {
        return true;
    }

    const int col = foot_x / tilemap.tile_size;
    const int row = foot_y / tilemap.tile_size;

    if (row < 0 || row >= static_cast<int>(tilemap.mapa.size())) {
        return false;
    }
    if (col < 0 || col >= static_cast<int>(tilemap.mapa[static_cast<std::size_t>(row)].size())) {
        return false;
    }

    const auto& name = tilemap.mapa[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
    auto it = tilemap.tiles.find(name);
    return it == tilemap.tiles.end() || it->second.walkable;
}
