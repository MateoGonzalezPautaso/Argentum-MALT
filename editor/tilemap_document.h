#ifndef EDITOR_TILEMAP_DOCUMENT_H
#define EDITOR_TILEMAP_DOCUMENT_H

#include <string>
#include "../common/config.h"

class TilemapDocument {
public:
    void load(const std::string& path);

    int width() const;
    int height() const;
    int tile_size() const;
    const std::string& tile_name(int row, int col) const;
    void set_tile(int row, int col, const std::string& name);
    void resize(int new_rows, int new_cols, const std::string& default_tile = "");

    const TilemapConfig& config() const { return config_; }

private:
    TilemapConfig config_;
};

#endif
