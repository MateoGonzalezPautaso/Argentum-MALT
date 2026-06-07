#include "editor_controller.h"

#include <QGraphicsScene>
#include <QMessageBox>
#include <QWidget>
#include <stdexcept>
#include <toml++/toml.hpp>

#include "../io/file_manager.h"

EditorController::EditorController(QGraphicsScene* scene):
        renderer_(std::make_unique<MapSceneRenderer>(scene, atlas_loader_)),
        file_manager_(std::make_unique<FileManager>(nullptr)) {}

void EditorController::load_document(const std::string& config_path) {
    doc_.load(config_path);
}

void EditorController::load_default_tile_config(const std::string& path) {
    try {
        toml::table root = toml::parse_file(path);
        parse_tilemap_config(root, default_tile_config_);
        parse_prop_config(root, default_tile_config_);
    } catch (const std::exception& e) {
        qWarning("Failed to load default tile config: %s", e.what());
    }
}

void EditorController::create_new_map(int rows, int cols, MapType map_type) {
    renderer_->clear_all();
    doc_.create_new(rows, cols, default_tile_config_, map_type);
    reload_atlas();
    full_rebuild();
}

void EditorController::reload_atlas() {
    atlas_loader_.clear();
    atlas_loader_.load(doc_.config());
}

void EditorController::full_rebuild() {
    renderer_->clear_all();
    renderer_->render_all(doc_, show_walkable_overlay_);
    renderer_->rebuild_grid(doc_);
    renderer_->rebuild_spawn_overlay(doc_);
}

bool EditorController::validate_city_map() const {
    if (doc_.config().map_type != MapType::CITY) {
        return true;
    }

    QStringList missing;
    for (const auto& npc : kCityRequiredNpcs) {
        bool found = false;
        for (const auto& row : doc_.config().prop_map) {
            for (const auto& cell : row) {
                if (cell == npc) {
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
        if (!found) missing << QString::fromStdString(npc);
    }

    if (missing.isEmpty()) {
        return true;
    }

    QMessageBox::critical(
            nullptr, "Cannot save City map",
            QString("A City map must contain the following NPCs:\n\n"
                    "  - %1\n\n"
                    "Please place them on the map before saving.")
                    .arg(missing.join("\n  - ")));
    return false;
}

bool EditorController::save() {
    if (!validate_city_map())
        return false;
    return file_manager_->save(doc_);
}

bool EditorController::save_as() {
    if (!validate_city_map())
        return false;
    return file_manager_->save_as(doc_);
}

bool EditorController::open() {
    if (!file_manager_->open(doc_))
        return false;
    reload_atlas();
    return true;
}

void EditorController::place_tile_or_prop(int row, int col, const std::string& name) {
    if (doc_.is_prop(name)) {
        doc_.set_prop(row, col, name);
        renderer_->update_prop(row, col, name, doc_);
    } else {
        doc_.set_tile(row, col, name);
        renderer_->update_tile(row, col, name, doc_, show_walkable_overlay_);
    }
}

void EditorController::fill_rect(int r1, int c1, int r2, int c2, const std::string& name) {
    int r_min = std::min(r1, r2);
    int r_max = std::max(r1, r2);
    int c_min = std::min(c1, c2);
    int c_max = std::max(c1, c2);

    for (int r = r_min; r <= r_max; ++r) {
        for (int c = c_min; c <= c_max; ++c) {
            place_tile_or_prop(r, c, name);
        }
    }
}

void EditorController::fill_spawn_zone_rect(int r1, int c1, int r2, int c2) {
    int r_min = std::min(r1, r2);
    int r_max = std::max(r1, r2);
    int c_min = std::min(c1, c2);
    int c_max = std::max(c1, c2);

    for (int r = r_min; r <= r_max; ++r) {
        for (int c = c_min; c <= c_max; ++c) {
            doc_.set_mob_spawn_zone(r, c, true);
            renderer_->update_spawn_overlay_tile(r, c, true, doc_.tile_size());
        }
    }
}

void EditorController::set_map_type(MapType type) {
    doc_.set_map_type(type);
}

void EditorController::resize_map(int cols, int rows) {
    renderer_->clear_all();
    doc_.resize(rows, cols, "");
    renderer_->render_all(doc_, show_walkable_overlay_);
    renderer_->rebuild_grid(doc_);
    renderer_->rebuild_spawn_overlay(doc_);
}

void EditorController::set_tile_walkable(const std::string& name, bool walkable) {
    doc_.set_tile_walkable(name, walkable);
}

void EditorController::toggle_walkable_overlay() {
    show_walkable_overlay_ = !show_walkable_overlay_;
    renderer_->clear_tiles_and_props();
    renderer_->render_all(doc_, show_walkable_overlay_);
}

void EditorController::toggle_spawn_overlay() {
    bool show = !renderer_->show_spawn_overlay();
    renderer_->set_show_spawn_overlay(show);
    renderer_->rebuild_spawn_overlay(doc_);
}

void EditorController::set_spawn_zone(int row, int col, bool enabled) {
    doc_.set_mob_spawn_zone(row, col, enabled);
    renderer_->update_spawn_overlay_tile(row, col, enabled, doc_.tile_size());
}

void EditorController::erase_prop(int row, int col) {
    doc_.set_prop(row, col, "");
    renderer_->update_prop(row, col, "", doc_);
}

void EditorController::erase_tile(int row, int col) {
    doc_.set_tile(row, col, "");
    renderer_->update_tile(row, col, "", doc_, show_walkable_overlay_);
}
