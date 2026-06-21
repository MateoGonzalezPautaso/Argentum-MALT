#include "camera.h"

#include <algorithm>

Camera::Camera(const ViewportConfig& viewport_cfg, int map_px_w, int map_px_h, bool has_tilemap,
               int window_w, int window_h):
        game_viewport(viewport_cfg.game_x, viewport_cfg.game_y, viewport_cfg.game_w,
                      viewport_cfg.game_h),
        map_px_w(map_px_w),
        map_px_h(map_px_h),
        has_tilemap(has_tilemap),
        window_w(window_w),
        window_h(window_h) {}

void Camera::reconfigure(int map_px_w, int map_px_h, bool has_tilemap) {
    this->map_px_w = map_px_w;
    this->map_px_h = map_px_h;
    this->has_tilemap = has_tilemap;
}

SDL2pp::Rect Camera::compute_view_rect(int movable_x, int movable_y, int movable_w,
                                  int movable_h) const {
    int cam_x = movable_x + movable_w / 2 - game_viewport.GetW() / 2;
    int cam_y = movable_y + movable_h / 2 - game_viewport.GetH() / 2;

    if (has_tilemap) {
        const int max_x = std::max(0, map_px_w - game_viewport.GetW());
        const int max_y = std::max(0, map_px_h - game_viewport.GetH());
        cam_x = std::clamp(cam_x, 0, max_x);
        cam_y = std::clamp(cam_y, 0, max_y);
    } else {
        cam_x = std::max(0, cam_x);
        cam_y = std::max(0, cam_y);
    }

    cached_ox = cam_x;
    cached_oy = cam_y;
    return SDL2pp::Rect(cam_x, cam_y, game_viewport.GetW(), game_viewport.GetH());
}

bool Camera::screen_to_world(int screen_x, int screen_y, int& world_x, int& world_y) const {
    const int local_x = screen_x - game_viewport.GetX();
    const int local_y = screen_y - game_viewport.GetY();
    if (local_x < 0 || local_y < 0 || local_x >= game_viewport.GetW() ||
        local_y >= game_viewport.GetH()) {
        return false;
    }
    world_x = local_x + cached_ox;
    world_y = local_y + cached_oy;
    return true;
}
