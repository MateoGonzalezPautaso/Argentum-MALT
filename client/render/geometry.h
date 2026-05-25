#ifndef CLIENT_RENDER_GEOMETRY_H
#define CLIENT_RENDER_GEOMETRY_H

#include <SDL2pp/SDL2pp.hh>

inline bool point_in_rect(int px, int py, const SDL2pp::Rect& r) {
    return px >= r.GetX() && px <= r.GetX() + r.GetW() && py >= r.GetY() &&
           py <= r.GetY() + r.GetH();
}

#endif  // CLIENT_RENDER_GEOMETRY_H
