#include "map_scene_renderer.h"

#include <QGraphicsRectItem>
#include <QPainter>
#include <QPen>
#include <utility>


MapSceneRenderer::MapSceneRenderer(QGraphicsScene* scene, const AtlasLoader& atlases):
        scene_(scene), atlases_(&atlases) {}

QPixmap MapSceneRenderer::tile_pixmap(const TilemapDocument& doc, const std::string& name) const {
    const auto& cfg = doc.config();
    auto tile_it = cfg.tiles.find(name);
    if (tile_it == cfg.tiles.end())
        return {};

    const auto& def = tile_it->second;
    std::string atlas_path = def.path.empty() ? cfg.path : def.path;
    const QPixmap* atlas = atlases_->get(atlas_path);
    if (!atlas)
        return {};

    return atlas->copy(QRect(def.x, def.y, doc.tile_size(), doc.tile_size()));
}

QPixmap MapSceneRenderer::prop_pixmap(const TilemapDocument& doc, const std::string& name) const {
    const auto& cfg = doc.config();
    auto prop_it = cfg.props.find(name);
    if (prop_it == cfg.props.end())
        return {};

    const auto& def = prop_it->second;
    if (def.paths.empty())
        return {};

    const QPixmap* atlas = atlases_->get(def.paths[0]);
    if (!atlas)
        return {};

    QPixmap frame = atlas->copy(QRect(def.src_x, def.src_y, def.src_w, def.src_h));

    int tsz = doc.tile_size();
    int display_w = def.width > 0 ? def.width : tsz;
    int display_h = def.height > 0 ? def.height : tsz;

    return frame.scaled(display_w, display_h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

void MapSceneRenderer::add_non_walkable_indicator(QGraphicsPixmapItem* item, int tsz) {
    auto* overlay = new QGraphicsRectItem(0, 0, tsz, tsz, item);
    overlay->setBrush(QColor(255, 0, 0, 60));
    overlay->setPen(QPen(Qt::NoPen));
}

void MapSceneRenderer::apply_prop_pos(QGraphicsPixmapItem* item, int col, int row,
                                      const std::string& prop_name,
                                      const TilemapDocument& doc) const {
    const auto& cfg = doc.config();
    auto prop_it = cfg.props.find(prop_name);
    int tsz = doc.tile_size();
    int display_w =
            (prop_it != cfg.props.end() && prop_it->second.width > 0) ? prop_it->second.width : tsz;
    int display_h = (prop_it != cfg.props.end() && prop_it->second.height > 0) ?
                            prop_it->second.height :
                            tsz;
    int offset_x = (display_w - tsz) / 2;
    int offset_y = (display_h - tsz) / 2;
    item->setPos(col * tsz - offset_x, row * tsz - offset_y);
    item->setZValue(0.5);
}

void MapSceneRenderer::clear_grid(std::vector<std::vector<QGraphicsPixmapItem*>>& grid) {
    for (auto& row: grid) {
        for (auto* item: row) {
            if (item) {
                scene_->removeItem(item);
                delete item;
            }
        }
    }
    grid.clear();
}

void MapSceneRenderer::render_all(const TilemapDocument& doc, bool show_walkable_overlay) {
    clear_tiles_and_props();

    const auto& cfg = doc.config();
    if (cfg.tiles.empty())
        return;

    int tsz = doc.tile_size();
    tile_items_.reserve(doc.height());

    for (int r = 0; r < doc.height(); ++r) {
        std::vector<QGraphicsPixmapItem*> row_items;
        row_items.reserve(doc.width());

        for (int c = 0; c < doc.width(); ++c) {
            const auto& name = doc.tile_name(r, c);
            QPixmap pix = tile_pixmap(doc, name);
            QGraphicsPixmapItem* item = nullptr;

            if (!pix.isNull()) {
                item = scene_->addPixmap(pix);
                item->setPos(c * tsz, r * tsz);
                if (show_walkable_overlay) {
                    auto tile_it = cfg.tiles.find(name);
                    if (tile_it != cfg.tiles.end() && !tile_it->second.walkable) {
                        add_non_walkable_indicator(item, tsz);
                    }
                }
            }
            row_items.push_back(item);
        }
        tile_items_.push_back(std::move(row_items));
    }

    render_props(doc);
}

void MapSceneRenderer::render_props(const TilemapDocument& doc) {
    const auto& cfg = doc.config();
    if (cfg.props.empty())
        return;

    prop_items_.reserve(doc.height());

    for (int r = 0; r < doc.height(); ++r) {
        std::vector<QGraphicsPixmapItem*> row_items;
        row_items.reserve(doc.width());

        for (int c = 0; c < doc.width(); ++c) {
            const auto& name = doc.prop_name(r, c);
            if (name.empty()) {
                row_items.push_back(nullptr);
                continue;
            }

            QPixmap scaled = prop_pixmap(doc, name);
            if (scaled.isNull()) {
                row_items.push_back(nullptr);
                continue;
            }

            auto* item = scene_->addPixmap(scaled);
            apply_prop_pos(item, c, r, name, doc);
            row_items.push_back(item);
        }
        prop_items_.push_back(std::move(row_items));
    }
}

void MapSceneRenderer::rebuild_grid(const TilemapDocument& doc) {
    auto rows = doc.height();
    auto cols = doc.width();
    auto tsz = doc.tile_size();

    int w = cols * tsz;
    int h = rows * tsz;

    QPen grid_pen(QColor(80, 80, 80), 1);

    for (int r = 0; r <= rows; ++r) {
        auto* line = scene_->addLine(0, r * tsz, w, r * tsz, grid_pen);
        line->setZValue(1);
    }
    for (int c = 0; c <= cols; ++c) {
        auto* line = scene_->addLine(c * tsz, 0, c * tsz, h, grid_pen);
        line->setZValue(1);
    }

    scene_->setSceneRect(0, 0, w, h);
}

void MapSceneRenderer::update_tile(int row, int col, const std::string& tile_name,
                                   const TilemapDocument& doc, bool show_walkable_overlay) {
    auto& old_item = tile_items_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
    if (old_item) {
        scene_->removeItem(old_item);
        delete old_item;
        old_item = nullptr;
    }

    if (tile_name.empty())
        return;

    QPixmap pix = tile_pixmap(doc, tile_name);
    if (pix.isNull())
        return;

    int tsz = doc.tile_size();
    auto* item = scene_->addPixmap(pix);
    item->setPos(col * tsz, row * tsz);

    if (show_walkable_overlay) {
        const auto& cfg = doc.config();
        auto tile_it = cfg.tiles.find(tile_name);
        if (tile_it != cfg.tiles.end() && !tile_it->second.walkable) {
            add_non_walkable_indicator(item, tsz);
        }
    }

    tile_items_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = item;
}

void MapSceneRenderer::update_prop(int row, int col, const std::string& prop_name,
                                   const TilemapDocument& doc) {
    auto& old_item = prop_items_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
    if (old_item) {
        scene_->removeItem(old_item);
        delete old_item;
        old_item = nullptr;
    }

    if (prop_name.empty())
        return;

    QPixmap scaled = prop_pixmap(doc, prop_name);
    if (scaled.isNull())
        return;

    auto* item = scene_->addPixmap(scaled);
    apply_prop_pos(item, col, row, prop_name, doc);

    prop_items_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = item;
}

void MapSceneRenderer::clear_tiles_and_props() {
    clear_grid(tile_items_);
    clear_grid(prop_items_);
}

void MapSceneRenderer::clear_all() {
    clear_tiles_and_props();
    scene_->clear();
}
