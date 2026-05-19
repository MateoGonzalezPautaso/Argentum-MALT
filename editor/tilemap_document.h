#ifndef EDITOR_TILEMAP_DOCUMENT_H
#define EDITOR_TILEMAP_DOCUMENT_H

#include <string>
#include "../common/config.h"

class TilemapDocument {
public:
    void load(const std::string& path);
    void save(const std::string& path) const;

    int width() const;
    int height() const;
    int tile_size() const;
    const std::string& tile_name(int row, int col) const;
    void set_tile(int row, int col, const std::string& name);
    void resize(int new_rows, int new_cols, const std::string& default_tile = "");

    const TilemapConfig& config() const { return config_; }
    const std::string& path() const { return path_; }
    void set_path(const std::string& path) { path_ = path; }

private:
    TilemapConfig config_;
    std::string path_;
};

#endif
