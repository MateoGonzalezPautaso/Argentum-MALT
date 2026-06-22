#ifndef EDITOR_EDITOR_CONTROLLER_H
#define EDITOR_EDITOR_CONTROLLER_H

#include <memory>
#include <string>
#include <vector>

#include "../../common/config.h"
#include "../document/tilemap_document.h"
#include "../io/file_manager.h"
#include "../render/atlas_loader.h"
#include "../render/map_scene_renderer.h"

class EditorController {
public:
    explicit EditorController(QGraphicsScene* scene);

    const TilemapDocument& document() const { return doc_; }
    TilemapDocument& document() { return doc_; }
    const TilemapConfig& default_tile_config() const { return default_tile_config_; }
    const AtlasLoader& atlas_loader() const { return atlas_loader_; }
    AtlasLoader& atlas_loader() { return atlas_loader_; }
    MapSceneRenderer& renderer() { return *renderer_; }
    bool show_walkable_overlay() const { return show_walkable_overlay_; }

    void load_document(const std::string& config_path);
    void load_default_tile_config(const std::string& path);
    void create_new_map(int rows, int cols, MapType map_type);
    void reload_atlas();

    bool save();
    bool save_as();
    bool open();
    bool validate_city_map() const;
    void register_current_map_in_list();

    void place_tile_or_prop(int row, int col, const std::string& name);
    void fill_rect(int r1, int c1, int r2, int c2, const std::string& name);
    void fill_spawn_zone_rect(int r1, int c1, int r2, int c2);
    void set_map_type(MapType type);
    void resize_map(int cols, int rows);
    void set_tile_walkable(const std::string& name, bool walkable);
    void set_prop_transition(const std::string& name, const std::string& transition_map,
                             int transition_x, int transition_y);
    void set_prop_transition_override(int row, int col, const std::string& transition_map,
                                      int transition_x, int transition_y);
    void toggle_walkable_overlay();
    void toggle_spawn_overlay();
    void set_spawn_zone(int row, int col, bool enabled);
    void erase_prop(int row, int col);
    void erase_tile(int row, int col);

    void full_rebuild();

    static inline const std::vector<std::string> kCityRequiredNpcs = {"comerciante", "banquero",
                                                                      "sacerdote"};

private:
    template <typename F>
    static void for_each_cell(int r1, int c1, int r2, int c2, F&& fn) {
        int r_min = std::min(r1, r2);
        int r_max = std::max(r1, r2);
        int c_min = std::min(c1, c2);
        int c_max = std::max(c1, c2);
        for (int r = r_min; r <= r_max; ++r)
            for (int c = c_min; c <= c_max; ++c) fn(r, c);
    }

    TilemapConfig default_tile_config_;
    TilemapDocument doc_;
    AtlasLoader atlas_loader_;
    std::unique_ptr<MapSceneRenderer> renderer_;
    std::unique_ptr<FileManager> file_manager_;
    bool show_walkable_overlay_ = true;
};

#endif
