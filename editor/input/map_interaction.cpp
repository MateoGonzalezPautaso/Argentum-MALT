#include "map_interaction.h"

MapInteraction::ClickResult MapInteraction::resolve_click(QMouseEvent* event, QGraphicsView* view,
                                                          int tile_size, int map_height,
                                                          int map_width) {

    ClickResult result;
    QPointF scene_pos = view->mapToScene(event->pos());

    result.col = static_cast<int>(scene_pos.x()) / tile_size;
    result.row = static_cast<int>(scene_pos.y()) / tile_size;
    result.button = event->button();

    result.valid = (result.row >= 0 && result.row < map_height && result.col >= 0 &&
                    result.col < map_width);

    return result;
}
