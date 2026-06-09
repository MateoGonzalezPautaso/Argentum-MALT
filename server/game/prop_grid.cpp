#include "prop_grid.h"

#include <algorithm>
#include <limits>
#include <utility>

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
            bool has_overrides = r < tilemap.prop_transition_overrides.size() &&
                                 c < tilemap.prop_transition_overrides[r].size();

            int prop_x = static_cast<int>(c) * tilemap.tile_size;
            int prop_y = (static_cast<int>(r) + 1) * tilemap.tile_size - prop.height;

            Entry entry;
            entry.name = prop_name;
            entry.row = static_cast<int>(r);
            entry.col = static_cast<int>(c);
            entry.center_x = prop_x + prop.width / 2;
            entry.center_y = prop_y + prop.height / 2;
            entry.hb_left = prop_x + prop.hitbox.x;
            entry.hb_top = prop_y + prop.hitbox.y;
            entry.hb_right = entry.hb_left + prop.hitbox.w;
            entry.hb_bottom = entry.hb_top + prop.hitbox.h;
            entry.hb_w = prop.hitbox.w;
            entry.hb_h = prop.hitbox.h;
            entry.def = &prop_it->second;
            if (has_overrides)
                entry.override = tilemap.prop_transition_overrides[r][c];

            entries_.push_back(std::move(entry));
        }
    }
}

bool PropGrid::is_hitbox_at(int foot_x, int foot_y) const {
    return std::any_of(entries_.begin(), entries_.end(), [foot_x, foot_y](const Entry& e) {
        return e.hb_w > 0 && e.hb_h > 0 && foot_x >= e.hb_left && foot_x < e.hb_right &&
               foot_y >= e.hb_top && foot_y < e.hb_bottom;
    });
}

const PropGrid::Entry* PropGrid::find_transition_at(int foot_x, int foot_y) const {
    for (const auto& e: entries_) {
        if (e.transition_map().empty())
            continue;
        if (foot_x >= e.hb_left && foot_x < e.hb_right && foot_y >= e.hb_top &&
            foot_y < e.hb_bottom)
            return &e;
    }
    return nullptr;
}

const PropGrid::Entry* PropGrid::find_closest(const std::string& prop_name, int px, int py,
                                              int range) const {
    const Entry* closest = nullptr;
    int best = range * range;
    for (const auto& e: entries_) {
        if (e.name != prop_name)
            continue;
        int dx = px - e.center_x;
        int dy = py - e.center_y;
        int dist_sq = dx * dx + dy * dy;
        if (dist_sq < best) {
            best = dist_sq;
            closest = &e;
        }
    }
    return closest;
}

bool PropGrid::is_in_range_of(const std::string& prop_name, int px, int py, int range) const {
    return std::any_of(entries_.begin(), entries_.end(),
                       [&prop_name, px, py, range](const Entry& e) {
                           return e.name == prop_name && std::abs(px - e.center_x) < range &&
                                  std::abs(py - e.center_y) < range;
                       });
}

bool PropGrid::find_first_transition(const std::string& target_map, int& out_cx,
                                     int& out_cy, int& out_hb_left,
                                     int& out_hb_bottom) const {
    for (const auto& e: entries_) {
        if (e.transition_map() == target_map) {
            out_cx = e.center_x;
            out_cy = e.center_y;
            out_hb_left = e.hb_left;
            out_hb_bottom = e.hb_bottom;
            return true;
        }
    }
    return false;
}

bool PropGrid::find_nearest_center(const std::string& prop_name, int px, int py, int& out_cx,
                                   int& out_cy) const {
    int best = std::numeric_limits<int>::max();
    bool found = false;

    for (const auto& e: entries_) {
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
