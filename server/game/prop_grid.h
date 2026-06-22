#ifndef SERVER_PROP_GRID_H
#define SERVER_PROP_GRID_H

#include <climits>
#include <string>
#include <vector>

#include "../../common/config.h"

class PropGrid {
public:
    explicit PropGrid(const TilemapConfig& tilemap);

    struct Entry {
        std::string name;
        int row;
        int col;
        int center_x;
        int center_y;
        int hb_left;
        int hb_top;
        int hb_right;
        int hb_bottom;
        int hb_w;
        int hb_h;
        const PropDef* def;
        PropTransitionOverride override;

        const std::string& transition_map() const {
            return !override.transition_map.empty() ? override.transition_map : def->transition_map;
        }
        int transition_x() const {
            return override.transition_map.empty() ? def->transition_x : override.transition_x;
        }
        int transition_y() const {
            return override.transition_map.empty() ? def->transition_y : override.transition_y;
        }
    };

    bool is_hitbox_at(int foot_x, int foot_y) const;

    const Entry* find_transition_at(int foot_x, int foot_y) const;

    const Entry* find_closest(const std::string& prop_name, int px, int py, int range) const;

    const Entry* find_closest(const std::string& prop_name, int px, int py) const;

    bool is_in_range_of(const std::string& prop_name, int px, int py, int range) const;

    bool find_first_transition(const std::string& target_map, int& out_cx, int& out_cy,
                               int& out_hb_left, int& out_hb_bottom) const;

private:
    std::vector<Entry> entries_;
};

#endif
