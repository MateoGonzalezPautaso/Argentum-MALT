#ifndef EDITOR_MAP_INTERACTION_H
#define EDITOR_MAP_INTERACTION_H

#include <QGraphicsView>
#include <QMouseEvent>
#include <string>

class MapInteraction {
public:
    struct ClickResult {
        int row = -1;
        int col = -1;
        bool valid = false;
        Qt::MouseButton button = Qt::NoButton;
    };

    static ClickResult resolve_click(QMouseEvent* event, QGraphicsView* view,
                                      int tile_size, int map_height, int map_width);

    void set_selected(const std::string& name) { selected_ = name; }
    const std::string& selected() const { return selected_; }
    bool has_selection() const { return !selected_.empty(); }

private:
    std::string selected_;
};

#endif
