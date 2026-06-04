#include "prop_grid.h"

#include <limits>

PropGrid::PropGrid(const TilemapConfig& tilemap) {
    entries_.reserve(tilemap.prop_map.size() * tilemap.prop_map.front().size());

    for (std::size_t r = 0; r < tilemap.prop_map.size(); ++r) {
        for (std::size_t c = 0; c < tilemap.prop_map[r].size(); ++c) {
            const auto& prop_name = tilemap.prop_map[r][c];
            if (prop_name.empty())
                continue;

            auto prop_it = tilemap.props.find(prop_name);
            if (prop_it == tilemap.props.end())
                continue;

            const PropDef& prop = prop_it->second;

            int prop_x = static_cast<int>(c) * tilemap.tile_size;
            int prop_y = (static_cast<int>(r) + 1) * tilemap.tile_size - prop.height;

            Entry entry;
            entry.name = prop_name;
            entry.center_x = prop_x + prop.width / 2;
            entry.center_y = prop_y + prop.height / 2;
            entry.hb_left = prop_x + prop.hitbox.x;
            entry.hb_top = prop_y + prop.hitbox.y;
            entry.hb_right = entry.hb_left + prop.hitbox.w;
            entry.hb_bottom = entry.hb_top + prop.hitbox.h;
            entry.hb_w = prop.hitbox.w;
            entry.hb_h = prop.hitbox.h;
            entry.def = &prop_it->second;

            entries_.push_back(std::move(entry));
        }
    }
}

bool PropGrid::is_hitbox_at(int foot_x, int foot_y) const {
    for (const auto& e : entries_) {
        if (e.hb_w <= 0 || e.hb_h <= 0)
            continue;
        if (foot_x >= e.hb_left && foot_x < e.hb_right &&
            foot_y >= e.hb_top && foot_y < e.hb_bottom)
            return true;
    }
    return false;
}

const PropDef* PropGrid::find_transition_at(int foot_x, int foot_y) const {
    for (const auto& e : entries_) {
        if (e.def->transition_map.empty())
            continue;
        if (foot_x >= e.hb_left && foot_x < e.hb_right &&
            foot_y >= e.hb_top && foot_y < e.hb_bottom)
            return e.def;
    }
    return nullptr;
}

bool PropGrid::is_in_range_of(const std::string& prop_name, int px, int py, int range) const {
    for (const auto& e : entries_) {
        if (e.name != prop_name)
            continue;
        if (std::abs(px - e.center_x) < range &&
            std::abs(py - e.center_y) < range)
            return true;
    }
    return false;
}

bool PropGrid::find_nearest_center(const std::string& prop_name, int px, int py,
                                   int& out_cx, int& out_cy) const {
    int best = std::numeric_limits<int>::max();
    bool found = false;

    for (const auto& e : entries_) {
        if (e.name != prop_name)
            continue;

        int dx = px - e.center_x;
        int dy = py - e.center_y;
        int dist_sq = dx * dx + dy * dy;

        if (dist_sq < best) {
            best = dist_sq;
            out_cx = e.center_x;
            out_cy = e.center_y;
            found = true;
        }
    }

    return found;
}
