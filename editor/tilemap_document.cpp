#include "tilemap_document.h"
#include <fstream>
#include <stdexcept>

void TilemapDocument::load(const std::string& path) {
    toml::table root = toml::parse_file(path);
    parse_tilemap_config(root, config_);
    if (config_.mapa.empty()) {
        throw std::runtime_error("Empty map grid in config file");
    }
    path_ = path;
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

void TilemapDocument::save(const std::string& path) const {
    toml::table tilemap_tbl;

    tilemap_tbl.emplace("path", config_.path);
    tilemap_tbl.emplace("tile_size", config_.tile_size);

    toml::array mapa_array;
    for (const auto& row : config_.mapa) {
        toml::array row_array;
        for (const auto& cell : row) {
            row_array.push_back(cell);
        }
        mapa_array.push_back(std::move(row_array));
    }
    tilemap_tbl.emplace("mapa", std::move(mapa_array));

    toml::table tiles_tbl;
    for (const auto& [name, def] : config_.tiles) {
        toml::table tile_def;
        tile_def.emplace("x", def.x);
        tile_def.emplace("y", def.y);
        if (!def.walkable) {
            tile_def.emplace("walkable", false);
        }
        tiles_tbl.emplace(name, std::move(tile_def));
    }
    tilemap_tbl.emplace("tiles", std::move(tiles_tbl));

    toml::table root;
    root.emplace("tilemap", std::move(tilemap_tbl));

    std::ofstream file(path);
    file << root << std::endl;
}
