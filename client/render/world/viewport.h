#ifndef CLIENT_VIEWPORT_H
#define CLIENT_VIEWPORT_H

#include <algorithm>
#include <cstdint>

#include <SDL2pp/SDL2pp.hh>

struct VisibleRange {
    int first_col;
    int first_row;
    int last_col;
    int last_row;
};

inline VisibleRange compute_visible_range(const SDL2pp::Rect& cam, int tile_size, int extra = 1) {
    return {std::max(0, cam.GetX() / tile_size - extra),
            std::max(0, cam.GetY() / tile_size - extra),
            (cam.GetX() + cam.GetW() - 1) / tile_size + extra,
            (cam.GetY() + cam.GetH() - 1) / tile_size + extra};
}

#endif
