#ifndef EDITOR_TILEMAP_DOCUMENT_H
#define EDITOR_TILEMAP_DOCUMENT_H

#include <string>

#include "common/config.h"

class TilemapDocument {
public:
    void load(const std::string& path);
    void save(const std::string& path) const;

    int width() const;
    int height() const;
    int tile_size() const;
    const std::string& tile_name(int row, int col) const;
    void set_tile(int row, int col, const std::string& name);
    const std::string& prop_name(int row, int col) const;
    void set_prop(int row, int col, const std::string& name);
    bool is_mob_spawn_zone(int row, int col) const;
    void set_mob_spawn_zone(int row, int col, bool value);
    void resize(int new_rows, int new_cols, const std::string& default_tile = "");
    void create_new(int rows, int cols, const TilemapConfig& tile_config,
                    MapType map_type = MapType::NONE);

    bool is_prop(const std::string& name) const {
        return config_.props.find(name) != config_.props.end();
    }

    PropTransitionOverride transition_override(int row, int col) const;
    void set_transition_override(int row, int col, const PropTransitionOverride& override);

    const TilemapConfig& config() const { return config_; }
    TilemapConfig& config() { return config_; }

    void set_map_type(MapType t) { config_.map_type = t; }
    void set_tile_walkable(const std::string& name, bool walkable);
    const std::string& path() const { return path_; }
    void set_path(const std::string& path) { path_ = path; }

private:
    template <typename T>
    void resize_grid(std::vector<std::vector<T>>& grid, int new_rows, int new_cols,
                     const T& default_value) {
        grid.resize(static_cast<std::size_t>(new_rows));
        for (auto& row: grid) row.resize(static_cast<std::size_t>(new_cols), default_value);
    }

    TilemapConfig config_;
    std::string path_;
};

#endif
