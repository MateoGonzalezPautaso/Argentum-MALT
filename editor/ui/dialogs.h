#ifndef EDITOR_DIALOGS_H
#define EDITOR_DIALOGS_H

#include <climits>
#include <string>

#include "common/config.h"

class QWidget;

constexpr int kMaxMapDimension = INT_MAX;

struct NewMapResult {
    int width = 20;
    int height = 20;
    MapType map_type = MapType::NONE;
    bool accepted = false;
};

NewMapResult show_new_map_dialog(QWidget* parent);

struct TransitionResult {
    std::string transition_map;
    int transition_x = 0;
    int transition_y = 0;
    bool accepted = false;
};

TransitionResult show_transition_dialog(QWidget* parent,
                                        const std::string& current_map,
                                        int current_x, int current_y);

#endif
