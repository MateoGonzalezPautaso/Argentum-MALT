#ifndef EDITOR_MAP_SCENE_RENDERER_H
#define EDITOR_MAP_SCENE_RENDERER_H

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <string>
#include <vector>
#include "atlas_loader.h"
#include "../document/tilemap_document.h"

class MapSceneRenderer {
public:
    MapSceneRenderer(QGraphicsScene* scene, const AtlasLoader& atlases);

    void render_all(const TilemapDocument& doc, bool show_walkable_overlay);
    void rebuild_grid(const TilemapDocument& doc);
    void update_tile(int row, int col, const std::string& tile_name,
                     const TilemapDocument& doc, bool show_walkable_overlay);
    void update_prop(int row, int col, const std::string& prop_name,
                     const TilemapDocument& doc);
    void clear_tiles_and_props();
    void clear_all();

private:
    void render_props(const TilemapDocument& doc);
    QPixmap tile_pixmap(const TilemapDocument& doc, const std::string& name) const;
    QPixmap prop_pixmap(const TilemapDocument& doc, const std::string& name) const;
    void add_walkable_overlay(QGraphicsPixmapItem* item, bool walkable, int tsz);

    QGraphicsScene* scene_;
    const AtlasLoader* atlases_;
    std::vector<std::vector<QGraphicsPixmapItem*>> tile_items_;
    std::vector<std::vector<QGraphicsPixmapItem*>> prop_items_;
};

#endif
