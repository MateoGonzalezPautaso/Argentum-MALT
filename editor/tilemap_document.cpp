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

void TilemapDocument::set_tile(int row, int col, const std::string& name) {
    config_.mapa[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = name;
}

void TilemapDocument::resize(int new_rows, int new_cols, const std::string& default_tile) {
    config_.mapa.resize(static_cast<std::size_t>(new_rows));
    for (auto& row : config_.mapa) {
        row.resize(static_cast<std::size_t>(new_cols), default_tile);
    }
}
