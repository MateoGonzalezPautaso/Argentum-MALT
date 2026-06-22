#ifndef CLIENT_CAMERA_H
#define CLIENT_CAMERA_H

#include <SDL2pp/SDL2pp.hh>

#include "../../config/config.h"

class Camera {
public:
    Camera() = default;
    Camera(const ViewportConfig& viewport_cfg, int map_px_w, int map_px_h, bool has_tilemap,
           int window_w, int window_h);

    void reconfigure(int map_px_w, int map_px_h, bool has_tilemap);

    SDL2pp::Rect compute_view_rect(int movable_x, int movable_y, int movable_w,
                                   int movable_h) const;
    int offset_x() const { return cached_ox; }
    int offset_y() const { return cached_oy; }
    bool screen_to_world(int screen_x, int screen_y, int& world_x, int& world_y) const;
    const SDL2pp::Rect& viewport() const { return game_viewport; }

private:
    SDL2pp::Rect game_viewport;
    int map_px_w = 0;
    int map_px_h = 0;
    bool has_tilemap = false;
    int window_w = 0;
    int window_h = 0;
    mutable int cached_ox = 0;
    mutable int cached_oy = 0;
};

#endif
