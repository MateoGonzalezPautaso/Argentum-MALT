#ifndef SERVER_MAP_H
#define SERVER_MAP_H

#include "../../common/config.h"

class Map {
public:
    explicit Map(const TilemapConfig& tilemap);

    int width_px() const { return map_px_w; }
    int height_px() const { return map_px_h; }
    int tile_size() const { return tilemap.tile_size; }

    int clamp_x(int x, int entity_w) const;
    int clamp_y(int y, int entity_h) const;

    bool is_walkable(int foot_x, int foot_y) const;

private:
    const TilemapConfig& tilemap;
    int map_px_w;
    int map_px_h;
};

#endif  // SERVER_MAP_H
