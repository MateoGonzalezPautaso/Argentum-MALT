#ifndef SERVER_MAP_H
#define SERVER_MAP_H

#include <optional>
#include <utility>

#include "../../common/config.h"
#include "../../common/rng.h"

#include "prop_grid.h"

class Map {
public:
    explicit Map(const TilemapConfig& tilemap);

    int width_px() const { return map_px_w; }
    int height_px() const { return map_px_h; }
    int tile_size() const { return tilemap_->tile_size; }

    const TilemapConfig& config() const { return *tilemap_; }
    const PropGrid& prop_grid() const { return prop_grid_; }

    int clamp_x(int x, int entity_w) const;
    int clamp_y(int y, int entity_h) const;

    bool is_walkable(int foot_x, int foot_y) const;
    bool is_mob_spawn_tile(int row, int col) const;
    std::optional<std::pair<int, int>> find_random_mob_spawn_pos(Rng& rng) const;

    bool is_position_in_spawn_zone(int x, int y) const;

private:
    const TilemapConfig* tilemap_;
    PropGrid prop_grid_;
    int map_px_w;
    int map_px_h;
};

#endif  // SERVER_MAP_H
