#ifndef SERVER_PROP_GRID_H
#define SERVER_PROP_GRID_H

#include <string>
#include <vector>

#include "../../common/config.h"

class PropGrid {
public:
    explicit PropGrid(const TilemapConfig& tilemap);

    bool is_hitbox_at(int foot_x, int foot_y) const;

    const PropDef* find_transition_at(int foot_x, int foot_y) const;

    bool is_in_range_of(const std::string& prop_name, int px, int py, int range) const;

private:
    struct Entry {
        std::string name;
        int center_x;
        int center_y;
        int hb_left;
        int hb_top;
        int hb_right;
        int hb_bottom;
        int hb_w;
        int hb_h;
        const PropDef* def;
    };

    std::vector<Entry> entries_;
};

#endif
