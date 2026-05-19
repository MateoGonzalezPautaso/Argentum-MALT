#include "tilemap_document.h"
#include <stdexcept>

void TilemapDocument::load(const std::string& path) {
    toml::table root = toml::parse_file(path);
    parse_tilemap_config(root, config_);
    if (config_.mapa.empty()) {
        throw std::runtime_error("Empty map grid in config file");
    }
}

int TilemapDocument::width() const {
    if (config_.mapa.empty()) return 0;
    return static_cast<int>(config_.mapa[0].size());
}

int TilemapDocument::height() const {
    return static_cast<int>(config_.mapa.size());
}

int TilemapDocument::tile_size() const {
    return config_.tile_size;
}

const std::string& TilemapDocument::tile_name(int row, int col) const {
    return config_.mapa[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
}
