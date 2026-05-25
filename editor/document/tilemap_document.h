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
    void resize(int new_rows, int new_cols, const std::string& default_tile = "");
    void create_new(int rows, int cols, const TilemapConfig& tile_config);

    bool is_prop(const std::string& name) const {
        return config_.props.find(name) != config_.props.end();
    }

    const TilemapConfig& config() const { return config_; }
    TilemapConfig& config() { return config_; }
    const std::string& path() const { return path_; }
    void set_path(const std::string& path) { path_ = path; }

private:
    TilemapConfig config_;
    std::string path_;
};

#endif
