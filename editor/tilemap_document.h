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

    const TilemapConfig& config() const { return config_; }

private:
    TilemapConfig config_;
};

#endif
