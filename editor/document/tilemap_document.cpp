#include "tilemap_document.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>

void TilemapDocument::load(const std::string& path) {
    toml::table root = toml::parse_file(path);
    parse_tilemap_config(root, config_);
    parse_prop_config(root, config_);
    if (config_.mapa.empty()) {
        throw std::runtime_error("Empty map grid in config file");
    }
    if (config_.prop_map.empty()) {
        config_.prop_map.resize(config_.mapa.size(),
                                std::vector<std::string>(config_.mapa[0].size(), ""));
    }
    path_ = path;
}

int TilemapDocument::width() const {
    if (config_.mapa.empty())
        return 0;
    return static_cast<int>(config_.mapa[0].size());
}

int TilemapDocument::height() const { return static_cast<int>(config_.mapa.size()); }

int TilemapDocument::tile_size() const { return config_.tile_size; }

const std::string& TilemapDocument::tile_name(int row, int col) const {
    return config_.mapa[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
}

void TilemapDocument::set_tile(int row, int col, const std::string& name) {
    config_.mapa[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = name;
}

const std::string& TilemapDocument::prop_name(int row, int col) const {
    return config_.prop_map[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
}

void TilemapDocument::set_prop(int row, int col, const std::string& name) {
    config_.prop_map[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = name;
}

void TilemapDocument::resize(int new_rows, int new_cols, const std::string& default_tile) {
    config_.mapa.resize(static_cast<std::size_t>(new_rows));
    for (auto& row: config_.mapa) {
        row.resize(static_cast<std::size_t>(new_cols), default_tile);
    }
    config_.prop_map.resize(static_cast<std::size_t>(new_rows));
    for (auto& row: config_.prop_map) {
        row.resize(static_cast<std::size_t>(new_cols), "");
    }
}

void TilemapDocument::create_new(int rows, int cols, const TilemapConfig& tile_config) {
    config_.path = tile_config.path;
    config_.tile_size = tile_config.tile_size;
    config_.tiles = tile_config.tiles;
    config_.mapa.assign(static_cast<std::size_t>(rows),
                        std::vector<std::string>(static_cast<std::size_t>(cols), ""));
    config_.props = tile_config.props;
    config_.prop_map.assign(static_cast<std::size_t>(rows),
                            std::vector<std::string>(static_cast<std::size_t>(cols), ""));
    path_.clear();
}

void TilemapDocument::save(const std::string& path) const {
    toml::table tilemap_tbl;

    tilemap_tbl.emplace("path", config_.path);
    tilemap_tbl.emplace("tile_size", config_.tile_size);

    toml::array mapa_array;
    for (const auto& row: config_.mapa) {
        toml::array row_array;
        std::copy(row.begin(), row.end(), std::back_inserter(row_array));
        mapa_array.push_back(std::move(row_array));
    }
    tilemap_tbl.emplace("mapa", std::move(mapa_array));

    toml::table tiles_tbl;
    for (const auto& [name, def]: config_.tiles) {
        toml::table tile_def;
        tile_def.emplace("x", def.x);
        tile_def.emplace("y", def.y);
        if (!def.walkable) {
            tile_def.emplace("walkable", false);
        }
        if (!def.path.empty()) {
            tile_def.emplace("path", def.path);
        }
        tiles_tbl.emplace(name, std::move(tile_def));
    }
    tilemap_tbl.emplace("tiles", std::move(tiles_tbl));

    toml::table root;
    root.emplace("tilemap", std::move(tilemap_tbl));

    if (!config_.props.empty()) {
        toml::table prop_tbl;

        toml::table prop_tiles_tbl;
        for (const auto& [name, def]: config_.props) {
            toml::table prop_def;
            toml::array paths_arr;
            std::copy(def.paths.begin(), def.paths.end(), std::back_inserter(paths_arr));
            prop_def.emplace("paths", std::move(paths_arr));

            toml::table src;
            src.emplace("x", def.src_x);
            src.emplace("y", def.src_y);
            src.emplace("w", def.src_w);
            src.emplace("h", def.src_h);
            prop_def.emplace("src", std::move(src));

            prop_def.emplace("width", def.width);
            prop_def.emplace("height", def.height);
            if (def.frame_ms > 0) {
                prop_def.emplace("frame_ms", static_cast<int64_t>(def.frame_ms));
            }
            if (def.hitbox.w > 0 && def.hitbox.h > 0) {
                toml::table hb;
                hb.emplace("x", def.hitbox.x);
                hb.emplace("y", def.hitbox.y);
                hb.emplace("w", def.hitbox.w);
                hb.emplace("h", def.hitbox.h);
                prop_def.emplace("hitbox", std::move(hb));
            }
            prop_tiles_tbl.emplace(name, std::move(prop_def));
        }
        prop_tbl.emplace("tiles", std::move(prop_tiles_tbl));

        auto row_has_prop = [](const auto& row) {
            return std::any_of(row.begin(), row.end(),
                               [](const auto& cell) { return !cell.empty(); });
        };
        bool has_props =
                std::any_of(config_.prop_map.begin(), config_.prop_map.end(), row_has_prop);

        if (has_props) {
            toml::array prop_grid;
            for (const auto& row: config_.prop_map) {
                toml::array row_array;
                std::copy(row.begin(), row.end(), std::back_inserter(row_array));
                prop_grid.push_back(std::move(row_array));
            }
            toml::table pm;
            pm.emplace("data", std::move(prop_grid));
            prop_tbl.emplace("prop_map", std::move(pm));
        }

        root.emplace("prop", std::move(prop_tbl));
    }

    std::ofstream file(path);
    file << root << std::endl;
}
